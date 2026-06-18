#ifndef LOCKFREE_QUEUE
#define LOCKFREE_QUEUE
#include<stddef.h>
#include<atomic>
#include<utility>

template<typename T,size_t Capacity>
class LOCKFREEQUEUE
{
private:
    T* buffer_;//环形数组
    alignas(64) std::atomic<size_t> write_idx_;//写指针.独占一行缓存行
    alignas(64) std::atomic<size_t> read_idx_;//读指针.独占一行，防止缓存行被另一个线程用着
    size_t Capacity_mask_;

public:
    explicit LOCKFREEQUEUE():write_idx_(0),read_idx_(0),Capacity_mask_(Capacity-1){
        buffer_ = static_cast<T*>(operator new(sizeof(T)*Capacity));//裸内存
    }//注意这里不能隐式类型转换裸内存变成T*

    bool push(T&& data) noexcept{//移动T实例进队列,这里只允许右值
        if(write_idx_-read_idx_==Capacity)
        {
            return false;
        }//套圈，写满了，待处理
        int index=write_idx_ & Capacity_mask_;//取模64，这里按位与计算
        new(&buffer_[index]) T(std::move(data));//函数内是有名左值，需要强转为右值才能移动
        //注意是裸内存，内部还没构造实例没有移动赋值，只能移动构造

        write_idx_.store(write_idx_.load()+1,std::memory_order_release);//逻辑同步
        //保证这行之前的全部写操作都完成才更新write_idx_
        //保证不被编译器重排顺序，导致worker线程读到未初始化内存.
        return true;
    }

    bool pop(T& data) noexcept//左值传参接收数据
    {
        if(read_idx_==write_idx_)
        {
            return false;
        }//这里需要条件变量等资源，生产消费，等线程池解决
        int index=read_idx_ & Capacity_mask_;
        data=std::move(buffer_[index]);
        buffer_[index].~T();//析构，被移动了。重新变回裸内存
        read_idx_.store(read_idx_.load()+1,std::memory_order_release);
        //保证被移动了数据才移动读指针，防止主线程生产资源写入有数据的内存。
        return true;
    }

    bool empty()const{
        return read_idx_==write_idx_;
    }//队列为空

    bool full()const{
        return write_idx_-read_idx_==Capacity;
    }//写满了，让主线程处理多于数据

    ~LOCKFREEQUEUE()
    {
        for(size_t i=read_idx_;i<write_idx_;i++)
        {
            buffer_[i&Capacity_mask_].~T();//析构队列资源，防止内存泄漏
        }
        operator delete (buffer_);//析构队列
    }
};



#endif