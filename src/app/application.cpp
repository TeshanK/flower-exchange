#include "app/application.h"

#include "common/thread_utils.h"
#include "common/validator.h"
#include "io/csv_reader.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace {

std::size_t copy_text_len(char* dst, std::size_t dst_size, const char* src) {
    if (!dst || dst_size == 0) {
        return 0;
    }
    dst[0] = '\0';
    if (!src) {
        return 0;
    }

    const std::size_t max_copy = dst_size - 1;
    std::size_t copy_len = 0;
    while (copy_len < max_copy && src[copy_len] != '\0') {
        dst[copy_len] = src[copy_len];
        ++copy_len;
    }
    dst[copy_len] = '\0';
    return copy_len;
}

void copy_text(char* dst, std::size_t dst_size, std::string_view src) {
    if (!dst || dst_size == 0) {
        return;
    }
    std::size_t copy_len = std::min(src.size(), dst_size - 1);
    if (copy_len > 0) {
        std::memcpy(dst, src.data(), copy_len);
    }
    dst[copy_len] = '\0';
}

template <typename TryOp>
void spin_until_success(TryOp&& try_op, int fast_spins) {
    int spin_count = 0;
    while (!try_op()) {
        if (spin_count++ < fast_spins) {
            __builtin_ia32_pause();
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            spin_count = 0;
        }
    }
}

const std::array<std::array<char, 5>, MAX_QUANTITY + 1> kQuantityText = []()
    -> std::array<std::array<char, 5>, MAX_QUANTITY + 1> {
    std::array<std::array<char, 5>, MAX_QUANTITY + 1> out{};
    for (int i = 0; i <= MAX_QUANTITY; ++i) {
        auto result = std::to_chars(out[i].data(), out[i].data() + out[i].size() - 1, i);
        if (result.ec == std::errc()) {
            *result.ptr = '\0';
        } else {
            out[i][0] = '0';
            out[i][1] = '\0';
        }
    }
    return out;
}();

const std::array<uint8_t, MAX_QUANTITY + 1> kQuantityTextLen = []()
    -> std::array<uint8_t, MAX_QUANTITY + 1> {
    std::array<uint8_t, MAX_QUANTITY + 1> out{};
    for (int i = 0; i <= MAX_QUANTITY; ++i) {
        const char* p = kQuantityText[i].data();
        uint8_t len = 0;
        while (p[len] != '\0') {
            ++len;
        }
        out[i] = len;
    }
    return out;
}();

std::size_t write_side_to_buffer(char* out, int side) {
    if (side == static_cast<int>(Side::BUY)) {
        out[0] = '1';
        return 1;
    }
    if (side == static_cast<int>(Side::SELL)) {
        out[0] = '2';
        return 1;
    }
    auto result = std::to_chars(out, out + 16, side);
    if (result.ec == std::errc()) {
        return static_cast<std::size_t>(result.ptr - out);
    }
    out[0] = '0';
    return 1;
}

std::size_t write_quantity_to_buffer(char* out, int quantity) {
    if (quantity >= 0 && quantity <= MAX_QUANTITY) {
        const std::size_t len = kQuantityTextLen[quantity];
        if (len > 0) {
            std::memcpy(out, kQuantityText[quantity].data(), len);
        }
        return len;
    }
    auto result = std::to_chars(out, out + 16, quantity);
    if (result.ec == std::errc()) {
        return static_cast<std::size_t>(result.ptr - out);
    }
    out[0] = '0';
    return 1;
}

void fill_outbound_common_fields(OutboundReportMsg& out,
                                 const ExecutionReport* report,
                                 const char* instrument_text,
                                 int side) {
    out.oid_len = static_cast<uint8_t>(copy_text_len(out.oid, sizeof(out.oid), report->oid));
    out.coid_len = static_cast<uint8_t>(copy_text_len(out.coid, sizeof(out.coid), report->coid));
    out.instrument_len = static_cast<uint8_t>(
        copy_text_len(out.instrument, sizeof(out.instrument), instrument_text));
    out.side = side;
    out.exec_status_len = static_cast<uint8_t>(
        copy_text_len(out.exec_status, sizeof(out.exec_status), exec_status_to_string(report->status)));
    out.quantity = report->quantity;
    out.reason_len = static_cast<uint8_t>(copy_text_len(out.reason, sizeof(out.reason), report->reason));
    out.timestamp_len = static_cast<uint8_t>(
        copy_text_len(out.timestamp, sizeof(out.timestamp), report->timestamp));
}

void append_csv_row(std::string& dst, const OutboundReportMsg& msg) {
    std::array<char, 320> row{};
    char* out = row.data();

    auto append_span = [&](const char* src, std::size_t len) -> void {
        if (len > 0) {
            std::memcpy(out, src, len);
            out += len;
        }
    };

    append_span(msg.oid, msg.oid_len);
    *out++ = ',';
    append_span(msg.coid, msg.coid_len);
    *out++ = ',';
    append_span(msg.instrument, msg.instrument_len);
    *out++ = ',';
    out += write_side_to_buffer(out, msg.side);
    *out++ = ',';
    append_span(msg.exec_status, msg.exec_status_len);
    *out++ = ',';
    out += write_quantity_to_buffer(out, msg.quantity);
    *out++ = ',';
    append_span(msg.price_text, msg.price_text_len);
    *out++ = ',';
    append_span(msg.reason, msg.reason_len);
    *out++ = ',';
    append_span(msg.timestamp, msg.timestamp_len);
    *out++ = '\n';

    dst.append(row.data(), static_cast<std::size_t>(out - row.data()));
}

void format_order_id(char* oid, std::size_t oid_size, uint64_t id) {
    if (!oid || oid_size < 5) {
        return;
    }
    oid[0] = 'o';
    oid[1] = 'r';
    oid[2] = 'd';
    auto result = std::to_chars(oid + 3, oid + oid_size - 1, id);
    if (result.ec == std::errc()) {
        *result.ptr = '\0';
        return;
    }
    oid[0] = '\0';
}

} // namespace

Application::Application()
    : initialized_(false),
      next_oid_(1),
    order_pool_(RuntimeConfig::kOrderPoolInitialCapacity),
    report_pool_(RuntimeConfig::kReportPoolInitialCapacity),
        buy_books_{OrderBook(RuntimeConfig::kOrderBookTickCapacity),
                 OrderBook(RuntimeConfig::kOrderBookTickCapacity),
                 OrderBook(RuntimeConfig::kOrderBookTickCapacity),
                 OrderBook(RuntimeConfig::kOrderBookTickCapacity),
                 OrderBook(RuntimeConfig::kOrderBookTickCapacity)},
        sell_books_{OrderBook(RuntimeConfig::kOrderBookTickCapacity),
                OrderBook(RuntimeConfig::kOrderBookTickCapacity),
                OrderBook(RuntimeConfig::kOrderBookTickCapacity),
                OrderBook(RuntimeConfig::kOrderBookTickCapacity),
                OrderBook(RuntimeConfig::kOrderBookTickCapacity)} {}

Application::~Application() {
    shutdown();
}

void Application::init() {
    if (initialized_) {
        return;
    }

    rebuild_matcher();
    initialized_ = true;
}

void Application::rebuild_matcher() {
    matcher_ = std::make_unique<MatchingEngine>(order_pool_, report_pool_, buy_books_, sell_books_);
}

void Application::shutdown() {
    if (!initialized_) {
        return;
    }
    matcher_.reset();
    initialized_ = false;
}

std::string Application::derive_output_path(const std::string& input_path) {
    std::filesystem::path in(input_path);
    std::string stem = in.stem().string();
    return (std::filesystem::path("output") / (stem + "_reports.csv")).string();
}

void Application::io_producer(const std::string& input_path,
                              std::atomic<bool>& producer_done,
                              std::atomic<uint64_t>& produced_orders,
                              std::atomic<uint64_t>& producer_ns) {
    const auto producer_start = std::chrono::steady_clock::now();
    uint64_t produced = 0;

    std::ifstream file(input_path);
    // Tune input buffer to 256 KB for faster bulk reads
    std::array<char, 262144> file_buffer{};
    file.rdbuf()->pubsetbuf(file_buffer.data(), static_cast<std::streamsize>(file_buffer.size()));
    
    if (!file.is_open()) {
        std::cerr << "Unable to open input file: " << input_path << '\n';
        InboundOrderMsg eos{};
        eos.end_of_stream = true;
        spin_until_success([&]() -> bool { return inbound_queue_.push(eos); }, 100);
        producer_done.store(true);
        return;
    }

    bool first_row = true;
    uint64_t seq = 1;
    for (const CSVRow& row : CSVRange(file)) {
        if (first_row) {
            first_row = false;
            continue;
        }
        if (row.size() < 5) {
            continue;
        }

        InboundOrderMsg msg{};
        msg.end_of_stream = false;
        copy_text(msg.coid, sizeof(msg.coid), row[0]);
        copy_text(msg.instrument, sizeof(msg.instrument), row[1]);
        msg.side = string_view_to_int(row[2]);
        msg.quantity = string_view_to_int(row[3]);
        msg.price = string_view_to_double(row[4]);
        msg.seq = seq++;
        ++produced;

        spin_until_success([&]() -> bool { return inbound_queue_.push(msg); }, 100);
    }

    InboundOrderMsg eos{};
    eos.end_of_stream = true;
    spin_until_success([&]() -> bool { return inbound_queue_.push(eos); }, 100);

    const auto producer_end = std::chrono::steady_clock::now();
    producer_ns.store(static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(producer_end - producer_start).count()));
    produced_orders.store(produced);
    producer_done.store(true);
}

void Application::matching_consumer(std::atomic<bool>& producer_done,
                                    std::atomic<bool>& matcher_done,
                                    std::atomic<uint64_t>& consumed_orders,
                                    std::atomic<uint64_t>& matcher_ns) {
    const auto matcher_start = std::chrono::steady_clock::now();
    uint64_t consumed = 0;

    bool got_eos = false;
    int pop_spin = 0;
    while (!got_eos || !producer_done.load()) {
        InboundOrderMsg msg{};
        if (!inbound_queue_.pop(msg)) {
            if (pop_spin++ < 1000) {
                __builtin_ia32_pause();
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                pop_spin = 0;
            }
            continue;
        }
        pop_spin = 0;

        if (msg.end_of_stream) {
            got_eos = true;
            continue;
        }
        ++consumed;

        std::array<char, 32> oid{};
        format_order_id(oid.data(), oid.size(), next_oid_++);

        InstrumentType instrument = InstrumentType::COUNT;
        parse_instrument(msg.instrument, instrument);
        auto validation =
            validate_order(instrument, msg.side, msg.price, msg.quantity);

        if (!validation.first) {
            std::array<char, 20> reject_timestamp{};
            ExecutionReport::fill_current_timestamp(reject_timestamp.data(), reject_timestamp.size());
            ExecutionReport* reject = report_pool_.allocate(
                oid.data(),
                msg.coid,
                InstrumentType::COUNT,
                (msg.side == static_cast<int>(Side::SELL) ? Side::SELL : Side::BUY),
                0,
                static_cast<uint16_t>(msg.quantity > 0 ? msg.quantity : 0),
                ExecStatus::REJECTED,
                validation.second,
                reject_timestamp.data());

            OutboundReportMsg out{};
            fill_outbound_common_fields(out, reject, msg.instrument, msg.side);
            out.price_text_len = static_cast<uint8_t>(
                format_price_to_buffer(msg.price, out.price_text, sizeof(out.price_text)));
            out.seq = msg.seq;

            spin_until_success([&]() -> bool { return outbound_queue_.push(out); }, 100);
            report_pool_.deallocate(reject);
            continue;
        }

        const PriceTick price_ticks = double_to_ticks(msg.price);
        if (price_ticks > MAX_VALID_TICK) {
            std::array<char, 20> reject_timestamp{};
            ExecutionReport::fill_current_timestamp(reject_timestamp.data(), reject_timestamp.size());
            ExecutionReport* reject = report_pool_.allocate(
                oid.data(),
                msg.coid,
                InstrumentType::COUNT,
                (msg.side == static_cast<int>(Side::SELL) ? Side::SELL : Side::BUY),
                0,
                static_cast<uint16_t>(msg.quantity > 0 ? msg.quantity : 0),
                ExecStatus::REJECTED,
                "Invalid price",
                reject_timestamp.data());

            OutboundReportMsg out{};
            fill_outbound_common_fields(out, reject, msg.instrument, msg.side);
            out.price_text_len = static_cast<uint8_t>(
                format_price_to_buffer(msg.price, out.price_text, sizeof(out.price_text)));
            out.seq = msg.seq;

            spin_until_success([&]() -> bool { return outbound_queue_.push(out); }, 100);
            report_pool_.deallocate(reject);
            continue;
        }

        Order* order = order_pool_.allocate(
            oid.data(),
            msg.coid,
            instrument,
            msg.side == static_cast<int>(Side::BUY) ? Side::BUY : Side::SELL,
            price_ticks,
            static_cast<uint16_t>(msg.quantity));

        matcher_->process_order(order, [&](ExecutionReport* report) -> void {
            OutboundReportMsg out{};
            fill_outbound_common_fields(out,
                                        report,
                                        instrument_to_string(report->instrument),
                                        static_cast<int>(report->side));
            out.price_text_len = static_cast<uint8_t>(
                format_ticks_to_buffer(report->price, out.price_text, sizeof(out.price_text)));
            out.seq = msg.seq;

            spin_until_success([&]() -> bool { return outbound_queue_.push(out); }, 100);
            report_pool_.deallocate(report);
        });
    }

    const auto matcher_end = std::chrono::steady_clock::now();
    matcher_ns.store(static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(matcher_end - matcher_start).count()));
    consumed_orders.store(consumed);
    matcher_done.store(true);
}

void Application::process_file(const std::string& input_path) {
    const auto total_start = std::chrono::steady_clock::now();
    next_oid_ = 1;

    while (!inbound_queue_.empty()) {
        InboundOrderMsg tmp{};
        inbound_queue_.pop(tmp);
    }
    while (!outbound_queue_.empty()) {
        OutboundReportMsg tmp{};
        outbound_queue_.pop(tmp);
    }

    std::fill(buy_books_.begin(),
              buy_books_.end(),
              OrderBook(RuntimeConfig::kOrderBookTickCapacity));
    std::fill(sell_books_.begin(),
              sell_books_.end(),
              OrderBook(RuntimeConfig::kOrderBookTickCapacity));
    rebuild_matcher();

    std::filesystem::create_directories("output");
    const std::string output_path = derive_output_path(input_path);

    std::ofstream out_file(output_path);
    std::array<char, 1 << 20> out_file_buffer{};
    out_file.rdbuf()->pubsetbuf(out_file_buffer.data(), static_cast<std::streamsize>(out_file_buffer.size()));
    if (!out_file.is_open()) {
        std::cerr << "Unable to open output file: " << output_path << '\n';
        return;
    }

    std::atomic<bool> producer_done{false};
    std::atomic<bool> matcher_done{false};
    std::atomic<uint64_t> produced_orders{0};
    std::atomic<uint64_t> consumed_orders{0};
    std::atomic<uint64_t> producer_ns{0};
    std::atomic<uint64_t> matcher_ns{0};

    std::thread* producer = createAndStartThread(
        0,
        "io-producer",
        &Application::io_producer,
        this,
        input_path,
        std::ref(producer_done),
        std::ref(produced_orders),
        std::ref(producer_ns));

    std::thread* matcher = createAndStartThread(
        1,
        "matcher",
        &Application::matching_consumer,
        this,
        std::ref(producer_done),
        std::ref(matcher_done),
        std::ref(consumed_orders),
        std::ref(matcher_ns));

    const auto write_start = std::chrono::steady_clock::now();
    std::string write_batch;
    write_batch.reserve(4 * 1024 * 1024);
    write_batch.append("Order ID,Client Order Id,Instrument,Side,Exec Status,Quantity,Price,Reason,Timestamp\n");

    int write_spin = 0;
    while (!matcher_done.load() || !outbound_queue_.empty()) {
        OutboundReportMsg msg{};
        if (!outbound_queue_.pop(msg)) {
            if (write_spin++ < 200) {
                __builtin_ia32_pause();
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                write_spin = 0;
            }
            continue;
        }
        write_spin = 0;

        append_csv_row(write_batch, msg);

        if (write_batch.size() >= 4 * 1024 * 1024) {
            out_file.write(write_batch.data(), static_cast<std::streamsize>(write_batch.size()));
            write_batch.clear();
        }
    }

    producer->join();
    matcher->join();
    delete producer;
    delete matcher;

    if (!write_batch.empty()) {
        out_file.write(write_batch.data(), static_cast<std::streamsize>(write_batch.size()));
    }

    const auto write_end = std::chrono::steady_clock::now();
    const double total_sec =
        std::chrono::duration_cast<std::chrono::duration<double>>(write_end - total_start).count();
    const double producer_sec = static_cast<double>(producer_ns.load()) / 1'000'000'000.0;
    const double matcher_sec = static_cast<double>(matcher_ns.load()) / 1'000'000'000.0;
    const double write_sec =
        std::chrono::duration_cast<std::chrono::duration<double>>(write_end - write_start).count();
    const uint64_t orders = produced_orders.load();
    const uint64_t matched = consumed_orders.load();
    const double orders_per_sec = (total_sec > 0.0) ? (static_cast<double>(orders) / total_sec) : 0.0;

    std::cout << "Wrote reports to " << output_path << '\n';
    std::cout << "PERF orders=" << orders
              << " consumed=" << matched
              << " total_sec=" << total_sec
              << " orders_per_sec=" << orders_per_sec
              << " producer_sec=" << producer_sec
              << " matcher_sec=" << matcher_sec
              << " write_sec=" << write_sec
              << '\n';
}

void Application::run() {
    init();

    std::string user_input;
    while (true) {
        std::getline(std::cin, user_input);
        if (user_input == "QUIT") {
            break;
        }

        std::stringstream ss(user_input);
        std::string cmd;
        std::string path;
        ss >> cmd >> path;

        if (cmd == "PROCESS" && !path.empty()) {
            process_file(path);
        }
    }
}
