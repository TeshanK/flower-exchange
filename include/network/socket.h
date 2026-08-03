#pragma once

#include <unistd.h>

class Socket {
public:
    // Default constructor, invalid socket
    constexpr Socket() noexcept = default;

    // Constructor takes the file descriptor of the socket
    explicit Socket(int fd) noexcept : fd_(fd) {}

    // Close the socket when the object is destroyed
    ~Socket() noexcept {
        reset();
    }

    // Disable copy
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // Move constructor
    Socket(Socket&& other) noexcept : fd_(other.release()) {}

    // Move assignment
    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    // Getters
    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] bool is_valid() const noexcept { return fd_ >= 0;}

private:

    int release() noexcept {
        int old_fd = fd_;
        fd_ = -1;
        return old_fd;
    }

    void reset(int new_fd = -1) noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = new_fd;
    }

    int fd_{-1};
};