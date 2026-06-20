#ifndef FD_GUARD
#define FD_GUARD
#include<unistd.h>

class FdGuard{
private:
    int fd_;

public:
    explicit FdGuard(int fd);
    FdGuard(const FdGuard&) =delete;
    FdGuard& operator=(const FdGuard&)=delete;
    operator int() const;
    FdGuard(FdGuard&&) noexcept;//移动构造
    FdGuard& operator=(FdGuard&&)noexcept ;//允许链式调用
    int release() noexcept;
    int get()const;
    ~FdGuard();
};

#endif