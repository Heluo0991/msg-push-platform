#include "reactor.h"
#include "lockfree_queue.h"
#include "rawmessage.h"
#include"mpmc_queue.h"


int main(int argc, char *argv[])
{
    std::vector<std::unique_ptr<LockFreeQueue<RawMessage, 1024>>> Workers;
    int workernum = 4;//无参数时默认SPSC队列数量
    if (argc == 3 && std::string(argv[1]) == "-w")
    {                                   // 传了三个值，包括"-w"和线程数argv[2]
        workernum = std::atoi(argv[2]); // 有参数就改workernum
    }

    for (int i = 0; i < workernum; i++)
    {
        Workers.emplace_back(std::make_unique<LockFreeQueue<RawMessage, 1024>>());
        // std::make_unique有noexcept，vector扩容不会退化成拷贝构造(保证可回退)
        // 临时匿名右值unique_ptr直接把他申请的堆内存交给workers的实例管理
    }
}
