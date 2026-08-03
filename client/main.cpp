#include <sep/protocol.h>
#include <common/types.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <print>
#include <vector>

int write_full(int fd, const void *buf, size_t n)
{
    const char *ptr = static_cast<const char *>(buf);
    while (n > 0)
    {
        ssize_t rv = write(fd, ptr, n);
        if (rv <= 0)
        {
            return -1; // Write error or closed connection
        }
        n -= static_cast<size_t>(rv);
        ptr += rv;
    }
    return 0;
}

int send_order(int fd, const New_order &new_order)
{
    if (write_full(fd, &new_order, sizeof(new_order)) < 0)
    {
        std::println(stderr, "write() error");
        return -1;
    }
    return 0;
}

New_order create_new_order(uint32_t client_order_id, Instrument_id instrument_id, Side book_side, uint16_t quantity, Price_ticks price)
{
    New_order new_order = {};
    new_order.header.version = 1;
    new_order.header.message_type = Message_type::new_order;
    new_order.header.body_length = sizeof(New_order) - sizeof(Message_header);
    new_order.client_order_id = client_order_id;
    new_order.instrument_id = instrument_id;
    new_order.book_side = book_side;
    new_order.quantity = quantity;
    new_order.price = price;

    return new_order;
}

struct TestOrder {
    uint32_t id;
    Instrument_id instrument;
    Side side;
    uint16_t qty;
    Price_ticks price;
    const char* scenario_desc;
};

int main()
{
    std::println("Starting Client...");

    auto fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        std::println(stderr, "Failed to create socket: {}", fd);
        return 1;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

    if (int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr)); rv < 0)
    {
        std::println(stderr, "Failed to connect: {}", rv);
        return 1;
    }

    // --- Execution Test Suite ---
    std::vector<TestOrder> scenario = {
        // 1. Establish initial spread (No execution yet)
        {1, Instrument_id::rose, Side::buy,  100, 1000, "[Setup] Buy 100 @ 1000 (Rests on book)"},
        {2, Instrument_id::rose, Side::buy,   50,  990, "[Setup] Buy  50 @  990 (Rests on book)"},
        {3, Instrument_id::rose, Side::sell, 100, 1050, "[Setup] Sell 100 @ 1050 (Rests on book)"},
        {4, Instrument_id::rose, Side::sell,  50, 1060, "[Setup] Sell  50 @ 1060 (Rests on book)"},

        // 2. Exact match / Partial fill on resting order
        {5, Instrument_id::rose, Side::buy,   40, 1050, "[Partial Fill] Buy 40 @ 1050 -> Fills 40 against Sell #3 @ 1050 (60 remaining)"},

        // 3. Complete resting order fill
        {6, Instrument_id::rose, Side::buy,   60, 1050, "[Full Fill] Buy 60 @ 1050 -> Fills remaining 60 of Sell #3 @ 1050"},

        // 4. Multi-level sweep (Aggressor crosses multiple price levels)
        {7, Instrument_id::rose, Side::sell,  80, 1070, "[Setup] Sell 80 @ 1070 (Rests on book)"},
        {8, Instrument_id::rose, Side::buy,  100, 1070, "[Level Sweep] Buy 100 @ 1070 -> Sweeps 50 @ 1060 and 50 @ 1070 (30 sell left @ 1070)"},

        // 5. Cross and Rest (Aggressor consumes liquidity, remaining rests on bid side)
        {9, Instrument_id::rose, Side::buy,   50, 1080, "[Cross & Rest] Buy 50 @ 1080 -> Consumes 30 @ 1070, remaining 20 rests @ 1080"},

        // 6. Clearing the bid side (Aggressive sell sweeping bids down to 990)
        {10, Instrument_id::rose, Side::sell, 200, 990, "[Sell Sweep] Sell 200 @ 990 -> Sweeps 20 @ 1080, 100 @ 1000, 50 @ 990"}
    };

    for (const auto &o : scenario)
    {
        std::println("Sending Order #{}: {}", o.id, o.scenario_desc);
        auto new_order = create_new_order(o.id, o.instrument, o.side, o.qty, o.price);
        if (send_order(fd, new_order) < 0)
        {
            std::println(stderr, "Failed to send order #{}", o.id);
            break;
        }

        // Small delay so logs appear clearly on the server
        usleep(50000);
    }

    close(fd);
    return 0;
}