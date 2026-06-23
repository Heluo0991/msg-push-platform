#include "reactor.h"

int main(int argc, char *argv[])
{
    DBstore db("./DATA.db");
    int workernum = 4;
    if (argc == 3 && std::string(argv[1]) == "-w")
    {
        workernum = std::atoi(argv[2]);
    }

    MPSCQueue replyqueue;
    ThreadPool workers(workernum);
    Reactor reactor(8080);
    reactor.run(workers, db, replyqueue);
}
