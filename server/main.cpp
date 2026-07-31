#include <sep/protocol.h>
#include <sep/sep_socket.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <cassert>
#include <print>
#include <expected>

int read_full(int fd, void *buf, size_t n)
{
    char *ptr = static_cast<char *>(buf);
    while (n > 0)
    {
        ssize_t rv = read(fd, ptr, n);
        if (rv <= 0)
        {
            return -1; // Connection closed or socket error
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        ptr += rv;
    }
    return 0;
}

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

auto get_order(int connfd) -> std::expected<New_order, int>
{
    New_order new_order = {};

    if (read_full(connfd, &new_order, sizeof(New_order)) < 0)
    {
        return std::unexpected(1);
    }

    if (new_order.header.message_type != Message_type::new_order)
    {
        std::println(stderr, "Protocol error: Unexpected message type {}",
                     static_cast<uint8_t>(new_order.header.message_type));
        return std::unexpected(2);
    }

    return new_order;
}

int run_ingress()
{
    std::println("Starting Ingress Process...");

    Socket server_sock{::socket(AF_INET, SOCK_STREAM, 0)};
    if (!server_sock.is_valid())
    {
        std::println(stderr, "Failed to create socket");
        return 1;
    }

    // If not set to 1, cannot bind to the same IP:port it was using after a restart
    int set_reuseaddr = 1;
    ::setsockopt(server_sock.get(), SOL_SOCKET, SO_REUSEADDR, &set_reuseaddr, sizeof(set_reuseaddr));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);

    if (::bind(server_sock.get(), reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        std::println(stderr, "Failed to bind");
        return 1;
    }

    if (::listen(server_sock.get(), SOMAXCONN) < 0)
    {
        std::println(stderr, "Failed to listen");
        return 1;
    }

    while (true)
    {
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);

        Socket client_sock{::accept(server_sock.get(), reinterpret_cast<struct sockaddr *>(&client_addr), &addrlen)};
        if (!client_sock.is_valid())
        {
            continue;
        }
        
        while (true)
        {
            auto order = get_order(client_sock.get());
            if (!order.has_value())
            {
                break;
            }

            print_order(order.value());
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