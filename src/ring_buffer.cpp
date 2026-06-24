#include "ring_buffer.h"

RingBuffer::RingBuffer(const char delimiter) : delimiter_(delimiter) {}

size_t RingBuffer::available() const
{
    return Capacity - (drain_ptr_ - pop_ptr_); // 容量
}

void RingBuffer::drain(int fd)
{
    if (!available())
        return;     // 没容量返回，但由于ET触发，可能会有数据残留在内核缓存区，之后处理
    char tmp[4096]; // 从缓存区取4096个字符，分割后续处理
    while (true)
    {
        int read_volume = std::min(sizeof(tmp), available()); // 最多取到容量
        ssize_t n = recv(fd, tmp, read_volume, 0);            // 取字符
        if (n > 0)
        {
            for (ssize_t i = 0; i < n; i++)
            {
                buf_[drain_ptr_ & (Capacity - 1)] = tmp[i]; // 对应位置对应塞，不会越界，n和index都有保证
                drain_ptr_++;
            }
        }
        else if (n == 0)
        {
            disconnected_=true;
            break; // 对端关闭
        }
        else if (n < 0) // 返回-1
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; // 代表内核缓冲区读完，正常结束
            }
            else
            {
                perror("未知读缓冲区错误");
                break;
            }
        }
    }
}

std::string RingBuffer::try_pop()
{
    if (available() == Capacity)
    {
        return ""; // 缓冲区无字符
    }

    // 扫描找 \n，用绝对值不掩码
    size_t scan = pop_ptr_;
    bool found = false;
    while (scan < drain_ptr_)
    {
        if (buf_[scan & (Capacity - 1)] == '\n')
        {
            found = true;
            break;
        }
        scan++;
    }

    if (!found)
    {
        return ""; // 无完整行，不动 pop_ptr_，数据留着等下次 drain 续上
    }

    // 找到了，scan 停在 \n 位置。把 [pop_ptr_, scan] 全部切出（含 \n）
    std::string pop_str;
    for (size_t i = pop_ptr_; i <= scan; i++)
    {
        pop_str += buf_[i & (Capacity - 1)];
    }
    pop_ptr_ = scan + 1; // 跳过 \n
    return pop_str;
}


bool RingBuffer::empty() const{
    return available()==Capacity;//可用量为容量
}

bool RingBuffer::is_disconnected()const{
    return disconnected_;
}


void RingBuffer::clear()
{
    pop_ptr_=0;
    drain_ptr_=0;
}//纯栈实例，直接指针置零就行

RingBuffer::~RingBuffer(){};//没有需要释放的对象