#include <sep/protocol.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <print>

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
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1

    if (int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr)); rv < 0)
    {
        std::println(stderr, "Failed to connect: {}", rv);
        return 1;
    }

    for (auto i = 0; i < 10; ++i)
    {
        auto new_order = create_new_order(i + 1, Instrument_id::rose, Side::buy, 100 + i, 1000 + i * 10);
        send_order(fd, new_order);
    }

    close(fd);

    return 0;
}