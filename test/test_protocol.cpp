#include "protocol.h"
#include <cassert>
#include <iostream>
#include <variant>

int main()
{
    // test1 解析 chat 消息
    {
        std::string raw = R"({"type":"chat","from":"alice","content":"hello"})";
        MessageBody out;
        try_parse_line(raw, out);

        assert(std::holds_alternative<ChatMsg>(out));
        auto &msg = std::get<ChatMsg>(out);
        assert(msg.from == "alice");
        assert(msg.content == "hello");

        std::cout << "test1 pass\n";
    }

    // test2 解析 login 消息
    {
        std::string raw = R"({"type":"login","username":"bob","password_hash":"abc123"})";
        MessageBody out;
        try_parse_line(raw, out);

        assert(std::holds_alternative<LoginMsg>(out));
        auto &msg = std::get<LoginMsg>(out);
        assert(msg.username == "bob");
        assert(msg.password_hash == "abc123");

        std::cout << "test2 pass\n";
    }

    // test3 解析 group 消息
    {
        std::string raw = R"({"type":"group","from":"alice","group_name":"cpp-club","content":"hi all"})";
        MessageBody out;
        try_parse_line(raw, out);

        assert(std::holds_alternative<GroupMsg>(out));
        auto &msg = std::get<GroupMsg>(out);
        assert(msg.from == "alice");
        assert(msg.group_name == "cpp-club");
        assert(msg.content == "hi all");

        std::cout << "test3 pass\n";
    }

    // test4 解析 private 消息
    {
        std::string raw = R"({"type":"private","from":"alice","to":"bob","content":"secret"})";
        MessageBody out;
        try_parse_line(raw, out);

        assert(std::holds_alternative<PrivateMsg>(out));
        auto &msg = std::get<PrivateMsg>(out);
        assert(msg.from == "alice");
        assert(msg.to == "bob");
        assert(msg.content == "secret");

        std::cout << "test4 pass\n";
    }

    // test5 解析 ack 消息
    {
        std::string raw = R"({"type":"ack","msg_id":"msg-001"})";
        MessageBody out;
        try_parse_line(raw, out);

        assert(std::holds_alternative<AckMsg>(out));
        auto &msg = std::get<AckMsg>(out);
        assert(msg.msg_id == "msg-001");

        std::cout << "test5 pass\n";
    }

    // test6 未知消息类型 → ErrorMsg
    {
        std::string raw = R"({"type":"unknown_xyz","foo":"bar"})";
        MessageBody out;
        try_parse_line(raw, out);

        assert(std::holds_alternative<ErrorMsg>(out));
        auto &msg = std::get<ErrorMsg>(out);
        assert(msg.code == -1);
        // reason 中包含类型名
        assert(msg.reason.find("unknown_xyz") != std::string::npos);

        std::cout << "test6 pass\n";
    }

    // test7 连续解析不同类型不互相污染
    {
        MessageBody out;

        try_parse_line(R"({"type":"chat","from":"a","content":"c"})", out);
        assert(std::holds_alternative<ChatMsg>(out));

        try_parse_line(R"({"type":"login","username":"u","password_hash":"p"})", out);
        assert(std::holds_alternative<LoginMsg>(out));

        try_parse_line(R"({"type":"ack","msg_id":"m1"})", out);
        assert(std::holds_alternative<AckMsg>(out));

        std::cout << "test7 pass\n";
    }

    // test8 中文内容不乱码
    {
        std::string raw = R"({"type":"chat","from":"张三","content":"你好世界！"})";
        MessageBody out;
        try_parse_line(raw, out);

        assert(std::holds_alternative<ChatMsg>(out));
        auto &msg = std::get<ChatMsg>(out);
        assert(msg.from == "张三");
        assert(msg.content == "你好世界！");

        std::cout << "test8 pass\n";
    }

    std::cout << "all tests passed\n";
    return 0;
}
