#ifndef REACTOR
#define REACTOR
#include "fd_guard.h"
#include "connection.h"
#include "object_pool.h"
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>
#include "protocol.h"
#include <iostream>
#include "lockfree_queue.h"
#include "rawmessage.h"
#include <mutex>
class Reactor
{
private:
    FdGuard listen_fd_;                                                // 监听fd
    FdGuard epfd_;                                                     // epoll的fd
    ObjectPool<Connection, 256> pool_;                                 // 256个connection实例大小的裸内存池
    std::mutex connections_mtx_;                               // 单独给connections的锁，因为子线程需要读他
    std::unordered_map<int, std::shared_ptr<Connection>> connections_; // 客户端fd到连接的映射表
    // 反向析构，pool_实例先死会导致，connections_死了之后share_ptr计数归零，调用pool_.deallocate()未定义行为
public:
    explicit Reactor(uint16_t); // 启动服务器引擎
    Reactor(const Reactor &) = delete;
    Reactor &operator=(const Reactor &) = delete;
    Reactor(Reactor &&) = delete;
    Reactor &operator=(Reactor &&) = delete;
    void run(std::vector<std::unique_ptr<LockFreeQueue<RawMessage, 1024>>> &);
    void handle_accept();
    void handle_read(std::shared_ptr<Connection>, std::vector<std::unique_ptr<LockFreeQueue<RawMessage, 1024>>> &);
    void process_line(std::shared_ptr<Connection>, const std::string &);
    void subscribe(int);                                // 注册客户端fd到红黑树
    void close_connection(std::shared_ptr<Connection>); // 从connections_中删除这个连接，从内存池取回内存
    std::unordered_map<int, std::shared_ptr<Connection>>& get_connections();
    std::mutex& get_connections_mutex();
    ~Reactor();
};

// Connection实例的共享指针全部由unordered_map管理，离开作用域自动析构share_ptr，然后自动析构Connection
// fd文件本身由FdGuard管理，离开作用域自动close(fd)
// Connection拿到fd之后就把这个fd给FdGuard处理了，只要connection死掉，fd也会死
// ObjectPool接管了全部连接的内存存放地址，全程不需要对Connection实例new堆内存，减少内核态切换
// 而share_ptr的use_count计数归零之后，析构Connection的时候调用的是ObjectPool的删除器，没有释放内存，只是归还内存
// 对于msg的收发，完成了ET触发epoll_wait然后处理msg，处理的时候保证哪怕没有收到终止符
// 也能针对Connection暂存缓冲区信息(环形缓冲区ring_buffer)，不会因为ET触发取了一半就发现没取完就丢了的情况
#endif