#include "connection.h"

Connection::Connection(int fd) : rbuf_('\n'), fd_(fd), last_active_(std::time(nullptr)) {}

int Connection::get_fd() const
{
    return fd_;
}

Connection::State Connection::get_state() const
{
    return state_;
}

std::string Connection::get_username() const
{
    return username_;
}

bool Connection::send_line(const std::string &raw_msg)
{
    std::string msg = raw_msg + '\n';
    if (send(fd_, msg.data(), msg.size(), 0))
    {
        return true;
    }
    else if (errno != (EAGAIN||EWOULDBLOCK))
    {
        perror("send");
        return false;
    } // errno==EAGAIN通常是内核缓冲区满了，epoll需要重发，这里等处理
    // 默认send只有短json文件，不会丢，很难堆满每个fd隔离的发送缓冲区
    else
    {
        return false;
    }
}

void Connection::touch()
{
    last_active_ = std::time(nullptr); // 当前unix时间戳
}

std::time_t Connection::idle_seconds() const
{
    return std::time(nullptr) - last_active_; // 距离上次使用这个连接过了多久
}