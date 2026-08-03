#pragma once

#include "socket.h"
#include "tcp_connection.h"
#include <optional>
#include <sys/socket.h>
#include <arpa/inet.h>

class TcpListener
{
public:
    static std::optional<TcpListener> create(int port)
    {
        Socket server_sock{::socket(AF_INET, SOCK_STREAM, 0)};
        if (!server_sock.is_valid())
        {
            return std::nullopt;
        }

        // If not set to 1, cannot bind to the same IP:port it was using after a restart
        int set_reuseaddr = 1;
        ::setsockopt(server_sock.get(), SOL_SOCKET, SO_REUSEADDR, &set_reuseaddr, sizeof(set_reuseaddr));

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(0);

        if (::bind(server_sock.get(), reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr)) < 0)
        {
            return std::nullopt;
        }

        if (::listen(server_sock.get(), SOMAXCONN) < 0)
        {
            return std::nullopt;
        }

        return TcpListener(std::move(server_sock));
    }

    std::optional<TcpConnection> accept() {
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);

        Socket client_sock{::accept(server_socket_.get(), reinterpret_cast<struct sockaddr *>(&client_addr), &addrlen)};
        if (!client_sock.is_valid())
        {
            return std::nullopt;
        }
        return TcpConnection(std::move(client_sock));
    }

private:
    explicit TcpListener(Socket server_socket) : server_socket_(std::move(server_socket)) {}
    Socket server_socket_;
};