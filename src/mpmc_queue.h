#ifndef MPMCQUEUE
#define MPMCQUEUE
#include<string>
#include<queue>
#include<mutex>
#include"connection.h"
#include"protocol.h"
#include<memory>
struct Reply{
    int fd_;
    MessageBody json_;//处理过的json字符串
};

class MPSCQueue{//多工作线程生产者，单主线程消费者
private:
    std::queue<Reply> queue_;
    std::mutex mtx_;
public:
    MPSCQueue(const MPSCQueue&)=delete;
    MPSCQueue& operator=(const MPSCQueue&)=delete;
    MPSCQueue(MPSCQueue&&)=delete;
    MPSCQueue& operator=(MPSCQueue&)=delete;
    inline void push(std::shared_ptr<Connection> conn){
        //从这个子线程的连接中获取生数据然后处理压入队列
        std::vector<std::string> raws;
        while(true){
            std::string raw= conn->rbuf_.try_pop();
            if(!raw.empty()){
                raws.push_back(raw);//这里会有移动构造
            }
        }
        std::lock_guard<std::mutex> lock(mtx_);
        for(auto it : raws){
            MessageBody json;
            try_parse_line(it ,json);
            queue_.emplace(conn->get_fd(),json);
        }
    }//子线程把生字符串从环形缓冲区中取出变成json结构体，压入MPSC队列交给主线程消费

    inline bool pop(Reply& out){//消费队列中的reply
        std::lock_guard<std::mutex> lock(mtx_);
        if(!queue_.empty()){
            out=std::move(queue_.front());
            queue_.pop();
            return true;
        }else{
            return false;
        }
    }

};


#endif