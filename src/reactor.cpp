#include "reactor.h"

Reactor::Reactor(uint16_t port) : listen_fd_(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)), epfd_(epoll_create1(0))
{
    if (listen_fd_ < 0)
    {
        perror("[Reactor] : listen_fd_ socket()");
        return;
    }
    if (epfd_ < 0)
    {
        perror("[Reactor] : epfd_ create");
    }
    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in listen_addr;
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listen_fd_, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0)
    {
        perror("[Reactor] : listen_fd_ bind()");
        return;
    }
    if (listen(listen_fd_, SOMAXCONN) < 0)
    {
        perror("[Reactor] : listen_fd_ listen");
    }
    subscribe(listen_fd_);
}

void Reactor::subscribe(int fd)
{
    epoll_event ev;
    ev.events = EPOLLET | EPOLLIN;
    ev.data.fd = fd;
    epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
}

void Reactor::handle_accept()
{
    while (true)
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd_ = accept4(listen_fd_, (struct sockaddr *)&client_addr, &client_len, SOCK_NONBLOCK);
        if (client_fd_ < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            break;
        }
        else if (client_fd_ < 0)
        {
            perror("[Reactor] : accept");
            continue;
        }
        subscribe(client_fd_);
        auto conn = std::shared_ptr<Connection>(
            pool_.allocate(client_fd_),
            [this](Connection *ptr) { pool_.deallocate(ptr); });

        // 手动 drain 一轮，防止 ET 竞态：accept 和 send 同时到达时 epoll 漏通知
        conn->rbuf_.drain(client_fd_);
        std::string line;
        while (!(line = conn->rbuf_.try_pop()).empty())
        {
            process_line(conn, line);
            conn->touch();
        }

        connections_.emplace(client_fd_, conn);
    }
}

void Reactor::handle_read(std::shared_ptr<Connection> conn)
{
    conn->rbuf_.drain(conn->get_fd());//送入缓冲区
    std::string recv_msg;
    while (true)
    {
        recv_msg = conn->rbuf_.try_pop();//从缓冲区取字符
        if (recv_msg.empty()) break;
        process_line(conn, recv_msg);//发送回
        conn->touch();
    }
}

void Reactor::process_line(std::shared_ptr<Connection> conn, const std::string &raw)
{
    MessageBody json_msg;
    try_parse_line(raw, json_msg);
    std::string resp = "{\"type\":\"chat\",\"from\":\"server\",\"msg\":\""
                     + std::to_string(json_msg.index())
                     + "\"}";
    conn->send_line(resp);
}

void Reactor::close_connection(std::shared_ptr<Connection> conn)
{
    epoll_ctl(epfd_, EPOLL_CTL_DEL, conn->get_fd(), NULL);
    connections_.erase(conn->get_fd());
    conn.reset();
}

void Reactor::run()
{
    epoll_event events[64];
    while (true)
    {
        int events_nums = epoll_wait(epfd_, events, 64, -1);
        if (events_nums < 0)
        {
            perror("[Reactor] : epoll_wait()");
            break;
        }
        for (int i = 0; i < events_nums; i++)
        {
            if (events[i].data.fd == static_cast<int>(listen_fd_))
            {
                handle_accept();
            }
            else
            {
                auto it = connections_.find(events[i].data.fd);
                if (it != connections_.end())
                    handle_read(it->second);
            }
        }
    }
}

Reactor::~Reactor()
{
    // 全程RAII,Reactor析构无资源需要手动释放
}
