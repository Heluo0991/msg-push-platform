#include <csignal>
#include "reactor.h"
#include "mpsc_queue.h"
#include "thread_pool.h"
#include "db_store.h"
#include "broker.h"

static Reactor* g_reactor = nullptr; // 信号处理器拿不到 this，用全局指针桥接

static void handle_signal(int) {
    if (g_reactor) g_reactor->stop();
}

int main(int argc, char *argv[])
{
    DBstore db("./DATA.db");
    std::vector<std::unique_ptr<LockFreeQueue<RawMessage, 1024>>> to_workers;
    int workernum = 4; // 无参数时默认SPSC队列数量
    if (argc == 3 && std::string(argv[1]) == "-w")
    {                                   // 传了三个值，包括"-w"和线程数argv[2]
        workernum = std::atoi(argv[2]); // 有参数就改workernum
    }

    for (int i = 0; i < workernum; i++)
    {
        to_workers.emplace_back(std::make_unique<LockFreeQueue<RawMessage, 1024>>());
    }

    std::atomic<bool> stop_flag{false};

    Reactor reactor(8080);
    g_reactor = &reactor;
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);
    ThreadPool workers(workernum);
    for (int i = 0; i < workernum; i++)
    {
        workers.submit([&, i] {
            while (!stop_flag.load(std::memory_order_relaxed))
            {
                RawMessage Raw;
                if (!to_workers[i]->pop(Raw))
                    continue; // 队列空，等一下再试

                auto conn = Raw.wp_.lock();
                if (!conn) continue;

                MessageBody json;
                try_parse_line(Raw.raw, json);
                {
                    std::lock_guard<std::mutex> lock(reactor.get_connections_mutex());
                    Broker::dispatch(conn, json, reactor.get_connections(), db);
                }
            }
        });
    }

    reactor.run(to_workers);                           // 阻塞直到 Ctrl+C → stop() → 退出循环
    stop_flag.store(true);                             // 通知 Worker lambda 退出 while 循环
}
