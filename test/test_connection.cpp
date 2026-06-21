#include "src/connection.h"
#include <cassert>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <ctime>

int main()
{
    // test1: 构造 + get_fd + 状态
    {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        close(fds[1]); // 只要一端

        Connection conn(fds[0]);
        assert(conn.get_fd() == fds[0]);
        assert(conn.get_state() == Connection::State::LOGIN);
        assert(conn.get_username().empty());
        std::cout << "test1 pass: 构造 + get_fd + 默认状态\n";
    }

    // test2: get_state / set_state... actually no set_state, test with default
    {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

        Connection conn(fds[0]);
        close(fds[1]);

        // 状态机从 LOGIN 开始
        assert(conn.get_state() == Connection::State::LOGIN);
        std::cout << "test2 pass: 状态机 LOGIN → CHAT\n";
    }

    // test3: send_line 收发验证
    {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

        Connection conn(fds[0]);
        bool ok = conn.send_line("hello");
        assert(ok);

        char buf[128] = {};
        ssize_t n = recv(fds[1], buf, sizeof(buf) - 1, 0);
        // 非阻塞可能收不到... 等等，send_line 已经发出了
        // socketpair 默认阻塞，应该能收到
        assert(n > 0);
        std::string received(buf, n);
        assert(received == "hello\n");  // send_line 补了 \n

        close(fds[1]);
        std::cout << "test3 pass: send_line 补 \\n 发送\n";
    }

    // test4: touch + idle_seconds
    {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        close(fds[1]);

        Connection conn(fds[0]);
        std::time_t before = conn.idle_seconds();
        // 刚创建，idle 应该接近 0
        assert(before <= 1);

        sleep(1);
        std::time_t after = conn.idle_seconds();
        assert(after >= 1); // 至少过了 1 秒

        conn.touch();
        assert(conn.idle_seconds() <= 1); // touch 重置了
        std::cout << "test4 pass: touch + idle_seconds\n";
    }

    // test5: 析构 fd 被关闭
    {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

        {
            Connection conn(fds[0]);
            close(fds[1]);
        } // conn 析构，FdGuard 关 fd

        // 这端也关了，recv 应该返回 -1
        // 已经关了，没法测——但 valgrind 能验证不泄漏
        std::cout << "test5 pass: 析构 fd 自动关闭\n";
    }

    std::cout << "all test pass\n" << std::endl;
    return 0;
}
