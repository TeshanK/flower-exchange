#include <sep/protocol.h>
#include <sep/sep_socket.h>
#include <sep/tcp_listener.h>
#include <sep/tcp_connection.h>
#include <sep/order_codec.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <cassert>
#include <print>
#include <expected>

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

int run_ingress()
{
    std::println("Starting Ingress Process...");

    auto listener = TcpListener::create(1234);

    while (true)
    {
        // TODO: handle nullopt case
        auto conn = listener->accept().value();

        while (true)
        {
            auto result = OrderCodec::decode_new_order(conn);
            if (!result.has_value())
            {
                break;
            }
            print_order(result.value());
        }
    }
    return 0;
}

int main()
{
    std::println("Starting Server...");

    if (int rv = run_ingress(); rv != 0)
    {
        std::println(stderr, "run_ingress() error: {}", rv);
        return 1;
    }

    return 0;
}