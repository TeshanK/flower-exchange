#pragma once

#include <sep/protocol.h>
#include "buffer.h"
#include <network/tcp_listener.h>
#include <print>
#include "order_codec.h"
#include "systypes.h"
#include "sequencer.h"

void print_order(const Order &order);

template <Buffer<Order> BufferImpl>
class IngressService
{
public:
    explicit IngressService(BufferImpl &input_buffer, Sequencer &sequencer)
        : input_buffer_(input_buffer), sequencer_(sequencer) {}

    void run(uint16_t port)
    {
        auto listener = TcpListener::create(port);
        if (!listener)
        {
            std::println("Ingress Service failed to bind to port {}", port);
            return;
        }

        std::println("Ingress Service running on port {}", port);

        while (running_)
        {
            auto conn = listener->accept();
            if (!conn)
            {
                continue;
            }

            handle_client(*conn);
        }
    }

private:
    void handle_client(const TcpConnection &conn)
    {
        while (running_)
        {
            auto result = OrderCodec::decode_new_order(conn);
            if (!result.has_value())
            {
                break;
            }

            auto order = sequencer_.sequence(result.value());

            // auto validation_result = RiskValidator::validate(order);
            // if (!validation_result)
            // {
            //     Rejection_reason reason = validation_result.error();
            //     std::println("Order #{} REJECTED by Risk Module. Reason code: {}",
            //                  order.raw_order.client_order_id,
            //                  static_cast<uint8_t>(reason));
            //
            //     // TODO: Send execution_report with status=rejected directly to Egress
            //     continue; // Skip pushing to Matching Engine!
            // }

            // print_order(order);
            input_buffer_.push(std::move(order));
        }
    }

    BufferImpl &input_buffer_;
    Sequencer &sequencer_;
    bool running_{true};
};

inline void print_order(const Order &order)
{
    std::println("sequence_number: {}", order.sequence_number);
    std::println("client_order_id: {}", order.client_order_id);
    std::println("instrument_id: {}", static_cast<uint8_t>(order.instrument_id));
    std::println("book_side: {}", static_cast<uint8_t>(order.book_side));
    std::println("quantity: {}", order.quantity);
    std::println("price: {}", order.price);
    std::println("ingress_timestamp: {}", order.ingress_timestamp);
}