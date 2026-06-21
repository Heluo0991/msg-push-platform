#include "src/ring_buffer.h"
#include <cassert>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

static int make_pair(int fds[2])
{
    return socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
}

int main()
{
    // test1: drain + try_pop 正常一行
    {
        int fds[2];
        make_pair(fds);

        RingBuffer rb('\n');
        send(fds[1], "hello\n", 6, 0);
        close(fds[1]);

        rb.drain(fds[0]);
        close(fds[0]);

        std::string line = rb.try_pop();
        assert(line == "hello\n");
        assert(rb.empty());
        std::cout << "test1 pass: drain + try_pop 正常一行\n";
    }

    // test2: 不完整行蹲等
    {
        int fds[2];
        make_pair(fds);

        RingBuffer rb('\n');
        send(fds[1], "hello wor", 9, 0);
        close(fds[1]);

        rb.drain(fds[0]);
        close(fds[0]);

        std::string line = rb.try_pop();
        assert(line.empty());
        assert(!rb.empty());
        std::cout << "test2 pass: 不完整行蹲等不消费\n";
    }

    // test3: 多行粘包
    {
        int fds[2];
        make_pair(fds);

        RingBuffer rb('\n');
        send(fds[1], "aaa\nbbb\nccc\n", 12, 0);
        close(fds[1]);

        rb.drain(fds[0]);
        close(fds[0]);

        assert(rb.try_pop() == "aaa\n");
        assert(rb.try_pop() == "bbb\n");
        assert(rb.try_pop() == "ccc\n");
        assert(rb.empty());
        std::cout << "test3 pass: 多行粘包逐行切\n";
    }

    // test4: 只有换行符
    {
        int fds[2];
        make_pair(fds);

        RingBuffer rb('\n');
        send(fds[1], "\n", 1, 0);
        close(fds[1]);

        rb.drain(fds[0]);
        close(fds[0]);

        assert(rb.try_pop() == "\n");
        assert(rb.empty());
        std::cout << "test4 pass: 只有换行符\n";
    }

    // test5: empty / clear
    {
        RingBuffer rb('\n');
        assert(rb.empty());

        int fds[2];
        make_pair(fds);
        send(fds[1], "data\n", 5, 0);
        close(fds[1]);
        rb.drain(fds[0]);
        close(fds[0]);

        assert(!rb.empty());
        rb.clear();
        assert(rb.empty());
        std::cout << "test5 pass: empty / clear\n";
    }

    // test6: drain EAGAIN 不崩溃（非阻塞 fd 无数据）
    {
        int fds[2];
        make_pair(fds);
        // 设非阻塞，drain 里 recv 立即返回 EAGAIN
        int flags = fcntl(fds[0], F_GETFL, 0);
        fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);

        RingBuffer rb('\n');
        rb.drain(fds[0]);
        close(fds[0]);
        close(fds[1]);
        assert(rb.try_pop().empty());
        std::cout << "test6 pass: 空 drain 不崩溃\n";
    }

    std::cout << "all tests pass\n" << std::endl;
    return 0;
}
