#include "reactor.h"
#include"mpsc_queue.h"
#include"thread_pool.h"
#include"db_store.h"
#include"broker.h"

int main(int argc, char *argv[])
{
    DBstore db("./DATA.db");
    std::vector<std::unique_ptr<LockFreeQueue<RawMessage, 1024>>> to_workers;
    int workernum = 4;//无参数时默认SPSC队列数量
    if (argc == 3 && std::string(argv[1]) == "-w")
    {                                   // 传了三个值，包括"-w"和线程数argv[2]
        workernum = std::atoi(argv[2]); // 有参数就改workernum
    }

    for (int i = 0; i < workernum; i++)
    {
        to_workers.emplace_back(std::make_unique<LockFreeQueue<RawMessage, 1024>>());
        // std::make_unique有noexcept，vector扩容不会退化成拷贝构造(保证可回退)
        // 临时匿名右值unique_ptr直接把他申请的堆内存交给workers的实例管理
    }
//思考一下，主线程从epoll中获取完美的缓冲区完整生字符串，处理handle_read
//handle_read读到生字符串之后，调度任意一个spsc队列作为管道输送这段带弱指针的生字符串
//外部线程池让子线程通过spsc队列获取这段数据去加工

//完成之后可能发一个Ack，让to_main, MPSC队列处理
    Reactor reactor(8080);
    reactor.run(to_workers);
    ThreadPool workers(workernum);//工作子线程池
    for(int i =0 ;i<workernum;i++){
        workers.submit([&,i]{
            while(true){
                RawMessage Raw;
                if(to_workers[i]->pop(Raw)){
                    MessageBody json;
                    try_parse_line(Raw.raw,json);
                    Broker::dispatch(Raw.wp_.lock(),json,reactor.)
                }//从环形缓冲区取字符串
            }
        }
        );//i单独一个循环拷贝快照值使用，其他正常引用，目的是让每个线程绑定自己的spsc队列
    }
}
