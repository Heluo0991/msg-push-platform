#include "reactor.h"

int main(int argc, char *argv[])
{
    DBstore db("./DATA.db");
    int workernum = 4;
    if (argc == 3 && std::string(argv[1]) == "-w")
    {
        workernum = std::atoi(argv[2]);
    }

    Broker::Groups groups_;//生命周期为进程级的群组映射表
    Broker::UserMap user_to_fd;//生命周期为进程级的
    MPSCQueue replyqueue;
    ThreadPool workers(workernum);
    Reactor reactor(8080);
    reactor.run(workers, db, replyqueue, groups_);
}
