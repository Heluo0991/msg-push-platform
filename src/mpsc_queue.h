#ifndef MPMCQUEUE
#define MPMCQUEUE
#include<string>
#include<queue>
#include<mutex>
#include"connection.h"
#include"protocol.h"
#include<memory>
struct Reply{
    int fd_;
    MessageBody json_;//处理过的json字符串
};

class MPSCQueue{//多工作线程生产者，单主线程消费者
private:
    std::queue<Reply> queue_;
    std::mutex mtx_;
public:
    MPSCQueue(const MPSCQueue&)=delete;
    MPSCQueue& operator=(const MPSCQueue&)=delete;
    MPSCQueue(MPSCQueue&&)=delete;
    MPSCQueue& operator=(MPSCQueue&)=delete;
    MPSCQueue();
    void push(std::shared_ptr<Connection>);
    bool pop(Reply&);
    ~MPSCQueue();
};


#endif  