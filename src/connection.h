#ifndef CONNECTION
#define CONNECTION
#include <memory>
#include "fd_guard.h"
#include "ring_buffer.h"
#include <string>
#include <ctime>

class Connection : public std::enable_shared_from_this<Connection>
{ // 可在类内获取同一控制块的shared_ptr
public:
    enum class State
    {
        LOGIN,
        CHAT
    };
    RingBuffer rbuf_; // 接收环形缓冲区，这个接口需要暴露，外部需要控制
private:
    FdGuard fd_;      // RAII管理文件
    State state_ = State::LOGIN; // 连接状态
    std::string username_;    // 用户名
    std::time_t last_active_; // 心跳
    int msg_id_counter_;      // 生成ACK用的消息序号
public:
    Connection(int);
    Connection(const Connection &) = delete;
    Connection &operator=(const Connection &) = delete;
    Connection(Connection &&) = delete;
    Connection &operator=(Connection &&) = delete;
    int get_fd() const;
    State get_state() const;
    std::string get_username() const;
    bool send_line(const std::string &); // 调用send(),返回是否发送成功
    void touch();                        // 更新心跳
    std::time_t idle_seconds() const;    // 距离上次touch的时间
};

#endif