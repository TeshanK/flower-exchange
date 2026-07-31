#pragma once

#include <sep/protocol.h>
#include <buffer.h>
#include <sep/tcp_listener.h>
#include <print>
#include <sep/order_codec.h>


void print_order(const New_order &new_order);

template <BufferFor<New_order> BufferImpl>
class IngressService
{
public:
    explicit IngressService(BufferImpl &input_buffer) : input_buffer_(input_buffer) {}

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

    void handle_client(const TcpConnection &conn) {
        while (running_)
        {
            auto result = OrderCodec::decode_new_order(conn);
            if (!result.has_value())
            {
                break;
            }
            print_order(result.value());
            input_buffer_.push(result.value());
        }
    }
    BufferImpl &input_buffer_;
    bool running_{true};
};

void print_order(const New_order &new_order)
{
    std::println("version: {}", new_order.header.version);
    std::println("message_type: {}", static_cast<uint8_t>(new_order.header.message_type));
    std::println("body_length: {}", new_order.header.body_length);
    std::println("client_order_id: {}", new_order.client_order_id);
    std::println("instrument_id: {}", static_cast<uint8_t>(new_order.instrument_id));
    std::println("book_side: {}", static_cast<uint8_t>(new_order.book_side));
    std::println("quantity: {}", new_order.quantity);
    std::println("price: {}", new_order.price);
}