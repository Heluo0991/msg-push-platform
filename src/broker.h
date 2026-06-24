#ifndef BROKER
#define BROKER
#include "connection.h"
#include <unordered_map>
#include "protocol.h"
#include "db_store.h"
#include"mpsc_queue.h"
#include<unordered_set>
#include<unordered_map>

namespace Broker
{
    using Groups = std::unordered_map<std::string,std::unordered_set<int>>;
    using UserMap = std::unordered_map<std::string,int>;

    template <class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;                       // 合并每个lambda的重载
        overloaded(Ts... ts) : Ts(std::move(ts))... {} // 初始化构造lambda实例，交给CTAD
    };
    // 整个逻辑是，模板overloaded结构体继承了全部六种lambda，
    // 然后内部展开了六个传入参数的默认operator()重载，也就是lambda表达式函数体的内容

    // 虽然每个lambda都是不同类型，但是匿名的，无法在参数列表显示指定其类模板，
    // 只能靠类模板参数推导CTAD，所以需要在参数列表内构造lambda实例让编译器推导

    inline void broadcast(std::shared_ptr<Connection> from, const ChatMsg &msg, const std::unordered_map<int, std::shared_ptr<Connection>> &connections,MPSCQueue & replyqueue)
    {
        json resp;
        resp["type"] = "chat";
        resp["from"] = msg.from;
        resp["content"] = msg.content;
        for (const auto &it: connections)
        {
           if(it.second!=from) replyqueue.push(Reply{it.second->get_fd(),resp.dump()}); // 给每个连接都广播，除了自己
        }
    }

    inline void private_msg(std::shared_ptr<Connection> from, const PrivateMsg &msg,MPSCQueue & replyqueue, const std::unordered_map<int, std::shared_ptr<Connection>> &connections, DBstore& db, UserMap& user_to_fd_)
    {
        if(user_to_fd_.find(msg.to)==user_to_fd_.end()){
            json resp;
            resp["type"]="private_failed";
            resp["reason"]="user_not_found";
            replyqueue.push(Reply{from->get_fd(),resp.dump()});
            return;
        }
        json resp;
        resp["type"]="private_msg";
        resp["from"]=msg.from;
        resp["content"]=msg.content;
        replyqueue.push(Reply{user_to_fd_[msg.to],resp.dump()});//发给对方
    }

    
    


    inline void group_msg(std::shared_ptr<Connection> from, const GroupMsg &msg,MPSCQueue & replyqueue, Groups& groups_,const std::unordered_map<int, std::shared_ptr<Connection>> &connections)
    {
        if(msg.cmd=="create"){
            if(groups_.find(msg.group_name)!=groups_.end())
            {
                json resp;
                resp["type"]="create_failed";
                resp["reason"]="dupilicate_group_name";
                replyqueue.push(Reply{from->get_fd(),resp.dump()});
                return;
            }
            std::unordered_set<int> group;
            group.emplace(from->get_fd());
            groups_.insert({msg.group_name,std::move(group)});
        }
        else if (msg.cmd ==  "join")
        {
            if(groups_.find(msg.group_name)==groups_.end()){
                json resp;
                resp["type"]="join_failed";
                resp["reason"]="group_not_found";
                replyqueue.push(Reply{from->get_fd(),resp.dump()});
                return;
            }
            if(groups_[msg.group_name].find(from->get_fd())!=groups_[msg.group_name].end())
            //群组成员已存在
            {
                 json resp;
                resp["type"]="join_failed";
                resp["reason"]="has_joined";
                replyqueue.push(Reply{from->get_fd(),resp.dump()});
                return;
            }
            groups_[msg.group_name].emplace(from->get_fd());
        }//成功就不回报文了，减少服务器负担
        else if(msg.cmd == "leave")
        {
            if(groups_.find(msg.group_name)==groups_.end()){
                json resp;
                resp["type"]="leave_failed";
                resp["reason"]="group_not_found";
                replyqueue.push(Reply{from->get_fd(),resp.dump()});
                return;
            }//小组不存在
            if(groups_[msg.group_name].find(from->get_fd())==groups_[msg.group_name].end())
            //群组成员不存在
            {
                 json resp;
                resp["type"]="leave_failed";
                resp["reason"]="member_not_found";
                replyqueue.push(Reply{from->get_fd(),resp.dump()});
                return;
            }
            groups_[msg.group_name].erase(from->get_fd());
        }
        else if(msg.cmd == "list")
        {
            json resp;
            resp["type"] ="groups_list";
            json arr = json::array();
            for(const auto& [name,members] : groups_){
                json item;
                item["group"]=name;
                item["count"]=members.size();
                arr.push_back(item);
            }
            resp["groups"] = arr;
            replyqueue.push(Reply{from->get_fd(),resp.dump()});
            //显式小组列表
        }
        else if(msg.cmd == "members")
        {
             if(groups_.find(msg.group_name)==groups_.end()){
                json resp;
                resp["type"]="members_list_failed";
                resp["reason"]="group_not_found";
                replyqueue.push(Reply{from->get_fd(),resp.dump()});
                return;
            }//小组不存在
            json resp;
            resp["type"] ="groups_list";
            resp["group"]=msg.group_name;
            json arr = json::array();
            for(int fd : groups_[msg.group_name]){
                auto it = connections.find(fd);
                std::string name = (it!=connections.end())?it->second->get_username():"unknown";
                //从connections里面找用户名，稍微抢一下锁
                json item;
                item["member"]=name;
                arr.push_back(item);
            }
            resp["members"] = arr;
            replyqueue.push(Reply{from->get_fd(),resp.dump()});
        }
        else if(msg.cmd=="msg")
        {
            if(groups_.find(msg.group_name)==groups_.end()){
                json resp;
                resp["type"]="members_list_failed";
                resp["reason"]="group_not_found";
                replyqueue.push(Reply{from->get_fd(),resp.dump()});
                return;
            }//小组不存在
            json resp;
            resp["type"] = "group_msg";
            resp["from"] = msg.from;
            resp["content"] = msg.content;
            for(int fd : groups_[msg.group_name]){
                if(fd!=from->get_fd()) replyqueue.push(Reply{fd,resp.dump()});
            }
        }
        else//非法cmd
        {
            json resp;
            resp["type"] = "error";
            resp["reason"] = "unknown group cmd: "+msg.cmd;
            replyqueue.push(Reply{from->get_fd(),resp.dump()});
            return;
        }
    }

    inline void login_msg(std::shared_ptr<Connection> from, const LoginMsg &msg, DBstore &db,MPSCQueue & replyqueue, UserMap& user_to_fd_)
    {
        // 根据门卫过滤，是非Chat状态连接才能调用这个接口
        //若出现多线程短时间同时处理Login_msg请求，也就是多个非chat连接竞争写state非锁变量
        if(!from->try_claim_login()){//原子更改连接状态和检查，其他人登录请求无法通过这个guard
            json resp;
            resp["type"] = "error";
            resp["reason"] = "frequent_login";
            replyqueue.push(Reply{from->get_fd(),resp.dump()});
            return;
        }//发回一个重复登录的回复

        //正常登录

        if (db.verify_login(msg.username, msg.password_hash))
        {
            // 登录成功，更改连接状态
            from->set_username(msg.username);//由于guard保护，不怕竞态写username

            json resp;
            resp["type"] = "login_ok";
            resp["user"] = msg.username;
            replyqueue.push(Reply{from->get_fd(),resp.dump()}); // 发回一个登录成功的报文
            user_to_fd_.insert({msg.username,from->get_fd()});//登录后加入到映射表
        }
        else
        {
            // 登录失败，可能是未注册，可能是密码错误，先处理未注册
            if (db.register_user(msg.username, msg.password_hash))
            {
                // 注册成功，更改连接状态
                from->set_state(Connection::State::CHAT);
                from->set_username(msg.username);
                json resp;
                resp["type"] = "register&login_ok";
                resp["user"] = msg.username;
                replyqueue.push(Reply{from->get_fd(),resp.dump()}); // 发回一个注册并登录成功的报文
            }
            else
            {
                // 登录失败且注册失败，大概率是密码错误
                json resp;
                resp["type"] = "error";
                resp["code"] = -1;
                resp["reason"] = "login failed";
                replyqueue.push(Reply{from->get_fd(),resp.dump()});
            }
        }
    }

    inline void ack_msg(std::shared_ptr<Connection> from, const AckMsg &msg,MPSCQueue & replyqueue)
    {
    }

    inline void error_msg(std::shared_ptr<Connection> from, const ErrorMsg &msg,MPSCQueue & replyqueue)
    {
        json resp;
        resp["type"] = "error";
        resp["code"] = msg.code;
        resp["reason"] = msg.reason;
        replyqueue.push(Reply{from->get_fd(),resp.dump()}); // 发回客户端一个json格式错误
    }

    inline void dispatch(std::shared_ptr<Connection> from, const MessageBody &msg, const std::unordered_map<int, std::shared_ptr<Connection>> &connections, DBstore &db,MPSCQueue & replyqueue , Groups & groups_, UserMap& user_to_fd_)
    { 

        // 门卫，如果非Chat状态连接只能进行登录请求
        bool is_login = std::holds_alternative<LoginMsg>(msg);
        if (from->get_state() == Connection::State::LOGIN && !is_login)
        {
            json resp;
            resp["type"] = "error";
            resp["code"] = -1;
            resp["reason"] = "Please login first";
            replyqueue.push(Reply{from->get_fd(),resp.dump()});
            return; // 非登录msg但connect未登录，需求他先登录
        }

        std::visit(
            overloaded{
                [&](const ChatMsg &m)
                {
                    broadcast(from, m, connections,replyqueue);
                },
                [&](const LoginMsg &m)
                {
                    login_msg(from, m, db, replyqueue, user_to_fd_);
                },
                [&](const GroupMsg &m)
                {
                    group_msg(from, m,replyqueue,groups_,connections);
                },
                [&](const PrivateMsg &m)
                {
                    private_msg(from, m, replyqueue, connections, db, user_to_fd_);
                },
                [&](const AckMsg &m)
                {
                    ack_msg(from, m,replyqueue);
                },
                [&](const ErrorMsg &m)
                {
                    error_msg(from, m,replyqueue);
                }},
            msg); // 构造overloaded匿名实例，六种lambda参数，有6个operator()重载
                  // lambda 捕获from
                  // visit取对应类型msg引用给匿名overloaded实例，编译期生成对应表，查表直接选择匹配
                  // visit内overloaded实例名为visitor,调用visitor().operator();//调用对应重载
                  // visit实际就是配套variant使用的根据类型选择重载的工具
    }
}

#endif