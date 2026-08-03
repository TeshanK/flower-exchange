#pragma once

#include "socket.h"
#include <utility>
#include <cassert>
#include <asm-generic/errno-base.h>

#include "socket.h"

class TcpConnection
{
public:
    explicit TcpConnection(Socket socket) : socket_(std::move(socket)) {}

    // Read exactly n bytes into the buffer
    int read_exact(void *buf, size_t n) const
    {
        auto *ptr = static_cast<char *>(buf);
        while (n > 0)
        {
            ssize_t rv = ::read(socket_.get(), ptr, n);
            if (rv <= 0)
            {
                if (rv == EINTR) {
                    continue;
                }
                return -1;
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
