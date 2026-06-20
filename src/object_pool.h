#ifndef OBJECT_POOL
#define OBJECT_POOL
#include <cstddef>
#include<utility>


template <typename T, size_t Poolsize>
class ObjectPool
{
private:
    struct Slot
    {
        union
        {
            T obj;
            Slot *next; // 该内存结点被摘下来时用obj使用这个slot结点的内存，空闲时指向下一个slot结点
        };
    };
    void *pool_;      // 内存池的头地址
    Slot *free_node_; // 当前空闲结点地址
    bool *used_;

public:
    explicit ObjectPool()
    {
        pool_ = operator new(sizeof(Slot) * Poolsize); // 裸内存分配，不调用Slot中T的构造函数的构造函数,减少内核消耗
        Slot *Slots = static_cast<Slot *>(pool_);      // 不能直接遍历void*的指针
        for (size_t i = 0; i < Poolsize - 1; i++)
        {
            Slots[i].next = &(Slots[i + 1]); // 链表连接
        }
        Slots[Poolsize - 1].next = nullptr; // 尾结点
        free_node_ = Slots;

        used_ = new bool[Poolsize];
        for (size_t i = 0; i < Poolsize; i++)
            used_[i] = false;
        // 标志内存池对应位置是否被使用,初始化为false
    }

    void deallocate(T *delptr)
    {
        delptr->~T(); // 析构存储对象实例
        Slot *backslot = reinterpret_cast<Slot *>(delptr);//归还的结点
        {
            size_t i = backslot - static_cast<Slot *>(pool_);
            used_[i] = false;
        }
        backslot->next = free_node_;
        free_node_ = backslot;
    }

    template <typename... Args>
    T *allocate(Args &&...args)
    {
        if (free_node_ != nullptr)
        {
            Slot *allo_node_ = free_node_;
            free_node_ = free_node_->next;
            new (&allo_node_->obj) T(std::forward<Args>(args)...);
            {
                size_t i = allo_node_ - static_cast<Slot *>(pool_); // 计算当前使用结点位置
                used_[i] = true;
            }
            return &allo_node_->obj; // allo_node_的指针值与联合体成员的指针值一致
        }
        return nullptr; // 链表无空间
    } // 裸内存上没有真正构造T实例，需要原地构造
      // 内部用完美转发，所以双引用
      // 参数列表用于传构造T实例所需的参数
      // allocate摘出了链表结构，但内存仍然可通过数组下标访问到。经过重组后，下标准确，链表访问顺序改变

    ~ObjectPool()
    {
        Slot *Slots = static_cast<Slot *>(pool_);
        for (size_t i = 0; i < Poolsize; i++)
        {
            if (used_[i])
                Slots[i].obj.~T();
        }
        operator delete(pool_);
        delete[] used_;
    }
};

// 模板类的实现都写在.h里
#endif