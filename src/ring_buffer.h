#ifndef RING_BUFFER
#define RING_BUFFER
#include<string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class RingBuffer
{
private:
    static constexpr size_t Capacity =4096;//每个环形缓冲区4kb内存
    char buf_[Capacity];//字符串环形缓冲区
    size_t drain_ptr_ = 0;
    size_t pop_ptr_ = 0;
    char delimiter_;//分割符号
public:
    explicit RingBuffer(const char);
    RingBuffer& operator=(const RingBuffer&)=delete;
    RingBuffer(const RingBuffer&)=delete;
    RingBuffer()=delete;
    void drain(int);
    std::string try_pop() ;
    bool empty() const;
    void clear();
    size_t available() const;
    ~RingBuffer();
};



//思考一下，
//只有主线程使用这个环形缓冲区，环形数组指针不需要atomic
//用size_t。如果主线程运行过久，size_t溢出，指针报废，环形缓冲区也报废了
//似乎不会，无符号整数溢出是标准行为，保持差值，完全可用
#endif