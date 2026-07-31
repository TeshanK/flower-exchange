#pragma once

#include <sep/sep_socket.h>
#include <utility>
#include <cassert>

class TcpConnection
{
public:
    explicit TcpConnection(Socket socket) : socket_(std::move(socket)) {}

    // Read exactly n bytes into the buffer
    int read_exact(void *buf, size_t n) const
    {
        char *ptr = static_cast<char *>(buf);
        while (n > 0)
        {
            ssize_t rv = ::read(socket_.get(), ptr, n);
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

private:
    Socket socket_;
};