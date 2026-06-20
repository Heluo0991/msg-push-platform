#include"src/fd_guard.h"
#include<cassert>
#include<unistd.h>
#include<fcntl.h>
#include<utility>

int main()
{
    int fd= open("/dev/null",O_WRONLY);
    FdGuard g(fd);
    assert(g.get()==fd);
    FdGuard g2 = std::move(g);//移动构造测试
    assert(g.get()==-1);
    assert(g2.get()==fd);
    int r=g2.release();//释放所有权
    assert(r==fd);
    assert(g2.get()==-1);
    close(r);
    return 0;
}