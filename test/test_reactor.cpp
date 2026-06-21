#include "src/reactor.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main()
{
    // 用非标准端口避免冲突
    const uint16_t test_port = 18080;

    // 在一个线程里跑 Reactor，主线程做客户端
    std::thread server_thread([&] {
        Reactor reactor(test_port);
        reactor.run();
    });
    server_thread.detach();

    // 等 Reactor 启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // test1: 客户端连上发消息收 Echo
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        assert(fd >= 0);

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(test_port);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        int ret = connect(fd, (sockaddr*)&addr, sizeof(addr));
        assert(ret == 0);

        // 发消息
        const char* msg = "{\"type\":\"chat\",\"from\":\"alice\",\"content\":\"hello\"}\n";
        send(fd, msg, strlen(msg), 0);

        // 收 Echo
        char buf[256] = {};
        // 等服务器处理完成，用 poll 或直接阻塞 recv
        ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
        assert(n > 0);
        std::string resp(buf, n);
        assert(!resp.empty());
        assert(resp.find("\"type\":\"chat\"") != std::string::npos);
        std::cout << "test1 pass: client recv echo: " << resp << "\n";

        close(fd);
    }

    // test2: 两个客户端同时连
    {
        int fd1 = socket(AF_INET, SOCK_STREAM, 0);
        int fd2 = socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(test_port);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        connect(fd1, (sockaddr*)&addr, sizeof(addr));
        connect(fd2, (sockaddr*)&addr, sizeof(addr));

        send(fd1, "{\"type\":\"chat\",\"from\":\"a\",\"content\":\"one\"}\n", 46, 0);
        send(fd2, "{\"type\":\"chat\",\"from\":\"b\",\"content\":\"two\"}\n", 46, 0);

        char buf[256] = {};
        int n1 = recv(fd1, buf, sizeof(buf) - 1, 0);
        assert(n1 > 0);

        memset(buf, 0, sizeof(buf));
        int n2 = recv(fd2, buf, sizeof(buf) - 1, 0);
        assert(n2 > 0);

        std::cout << "test2 pass: 2 clients echoed\n";

        close(fd1);
        close(fd2);
    }

    // test3: reactor 构造 bind 成功
    {
        // 已经在 test1 test2 中隐式验证了
        std::cout << "test3 pass: reactor bind/accept verified\n";
    }

    std::cout << "\nall test pass" << std::endl;
    return 0;
}
