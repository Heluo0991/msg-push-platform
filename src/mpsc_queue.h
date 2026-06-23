#ifndef MPSC_QUEUE
#define MPSC_QUEUE
#include <string>
#include <queue>
#include <mutex>
#include "message.h"

struct Reply
{
    int fd_;
    std::string json_ ;//子线程塞入MPSC队列的生字符串，json格式
};

class MPSCQueue
{ // 多 Worker 生产，单 Main 消费
private:
    std::queue<Reply> queue_;
    std::mutex mtx_;

public:
    MPSCQueue() = default;
    MPSCQueue(const MPSCQueue &) = delete;
    MPSCQueue &operator=(const MPSCQueue &) = delete;
    MPSCQueue(MPSCQueue &&) = delete;
    MPSCQueue &operator=(MPSCQueue &&) = delete;

    void push(Reply &&r)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(std::move(r));
    }

    bool pop(Reply &out)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) return false;
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }
};

#endif
