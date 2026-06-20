#include"fd_guard.h"

FdGuard::FdGuard(int fd):fd_(fd){}//

FdGuard::operator int() const {
    return fd_;
}

FdGuard::~FdGuard(){
    if(fd_>=0)close(fd_);
}

FdGuard::FdGuard(FdGuard&& other) noexcept{
    this->fd_=other.fd_;
    other.fd_=-1;//获得一个无效的fd文件标识符
}

int FdGuard::release() noexcept{
    int temp=fd_;
    fd_=-1;
    return temp;//RVO
}

FdGuard& FdGuard::operator=(FdGuard&& other)noexcept
{
    if(this!=&other){
        if(fd_>=0)close(fd_);//接管其他文件标识符前先释放自己管的文件，防止内存泄漏

        fd_=other.fd_;
        other.fd_=-1;
    }
    return *this;//链式调用
}

int FdGuard::get()const{
    return fd_;
}



