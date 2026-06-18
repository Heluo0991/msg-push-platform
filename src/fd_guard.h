#ifndef FD_GUARD
#define FD_GUARD
#include<unistd.h>

class FDGUARD{
private:
    int fd_;

public:
    explicit FDGUARD(int fd);
    FDGUARD(const FDGUARD&) =delete;
    FDGUARD& operator=(const FDGUARD&)=delete;
    operator int() const;
    FDGUARD(FDGUARD&&) noexcept;//移动构造
    FDGUARD& operator=(FDGUARD&&)noexcept ;//允许链式调用
    int release() noexcept;
    int get()const;
    ~FDGUARD();
};

#endif