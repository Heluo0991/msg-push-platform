#include"fd_guard.h"

FDGUARD::FDGUARD(int fd):fd_(fd){}//

FDGUARD::operator int() const {
    return fd_;
}

FDGUARD::~FDGUARD(){
    if(fd_>=0)close(fd_);
}

FDGUARD::FDGUARD(FDGUARD&& other) noexcept{
    this->fd_=other.fd_;
    other.fd_=-1;//获得一个无效的fd文件标识符
}

int FDGUARD::release() noexcept{
    int temp=fd_;
    fd_=-1;
    return temp;//RVO
}

FDGUARD& FDGUARD::operator=(FDGUARD&& other)noexcept
{
    if(this!=&other){
        if(fd_>=0)close(fd_);//接管其他文件标识符前先释放自己管的文件，防止内存泄漏

        fd_=other.fd_;
        other.fd_=-1;
    }
    return *this;//链式调用
}

int FDGUARD::get()const{
    return fd_;
}



