#ifndef MESSAGE
#define MESSAGE
#include <string>
#include <variant>

struct LoginMsg
{
    std::string username;
    std::string password_hash;
};

struct ChatMsg
{
    std::string from;
    std::string content;
};

struct GroupMsg
{
    std::string from;
    std::string cmd;
    std::string group_name;
    std::string content;
};

struct PrivateMsg
{
    std::string from;
    std::string to;
    std::string content;
};

struct ErrorMsg
{
    int code;
    std::string reason;
};

using MessageBody = std::variant<LoginMsg, ChatMsg, GroupMsg, PrivateMsg,  ErrorMsg>;

#endif