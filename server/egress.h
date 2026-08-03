#pragma once

#include "buffer.h"
#include "systypes.h"
#include <print>

template<Buffer<Report> OutputBuffer>
class EgressService {
public:
    explicit EgressService(OutputBuffer &output_buffer)
        : output_buffer_(output_buffer) {
    }

    std::size_t drain() {
        std::size_t processed_count = 0;

        while (auto report = output_buffer_.pop()) {
            dispatch_report(*report);
            processed_count++;
        }

        return processed_count;
    }

private:
    void dispatch_report(const Report &report) {
        std::println(
            "OrderID:{} | ClientOrderID:{} | InstrumentID:{} | Side:{} | Quantity:{} | Price:{} | Status:{} | Rejection:{} | Timestamp:{}",
            report.order_id,
            report.client_order_id,
            static_cast<uint8_t>(report.instrument_id),
            static_cast<uint8_t>(report.book_side),
            report.quantity,
            report.price,
            static_cast<uint8_t>(report.status),
            static_cast<uint8_t>(report.reject_reason),
            report.execution_timestamp);
    }

    OutputBuffer &output_buffer_;
};
