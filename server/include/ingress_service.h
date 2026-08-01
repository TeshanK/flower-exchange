#pragma once

#include <risk_validator.h>
#include <sep/protocol.h>
#include <buffer.h>
#include <sep/tcp_listener.h>
#include <print>
#include <sep/order_codec.h>
#include <types.h>
#include <sequencer.h>

void print_order(const Order &order);

template <BufferFor<Order> BufferImpl>
class IngressService
{
public:
    explicit IngressService(BufferImpl &input_buffer, Sequencer &sequencer)
        : input_buffer_(input_buffer), sequencer_(sequencer) {}

    void run(uint16_t port)
    {
        auto listener = TcpListener::create(1234);

        std::println("Ingress Service running on port {}", port);

        while (running_)
        {
            // TODO: handle nullopt case
            auto conn = listener->accept().value();

            handle_client(conn);
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

            auto validation_result = RiskValidator::validate(order);
            if (!validation_result)
            {
                Rejection_reason reason = validation_result.error();
                std::println("Order #{} REJECTED by Risk Module. Reason code: {}",
                             order.raw_order.client_order_id,
                             static_cast<uint8_t>(reason));

                // TODO: Send execution_report with status=rejected directly to Egress
                continue; // Skip pushing to Matching Engine!
            }

            print_order(order);
            input_buffer_.push(std::move(order));
        }
    }

    BufferImpl &input_buffer_;
    Sequencer &sequencer_;
    bool running_{true};
};

void print_order(const Order &order)
{
    std::println("version: {}", order.raw_order.header.version);
    std::println("message_type: {}", static_cast<uint8_t>(order.raw_order.header.message_type));
    std::println("body_length: {}", order.raw_order.header.body_length);
    std::println("client_order_id: {}", order.raw_order.client_order_id);
    std::println("instrument_id: {}", static_cast<uint8_t>(order.raw_order.instrument_id));
    std::println("book_side: {}", static_cast<uint8_t>(order.raw_order.book_side));
    std::println("quantity: {}", order.raw_order.quantity);
    std::println("price: {}", order.raw_order.price);
    std::println("sequence_number: {}", order.sequence_number);
    std::println("ingress_timestamp: {}", order.ingress_timestamp);
}