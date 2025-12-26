#ifndef EPOLL_H
#define EPOLL_H

#include <vector>
#include <sys/epoll.h>

class Epoll{
public:
    explicit Epoll(int maxEvent=1024);
    ~Epoll();

    bool AddFd(int fd,uint32_t events); //注册事件
    bool ModFd(int fd,uint32_t events); //修改监听事件
    bool DelFd(int fd); //删除fd

    int Wait(int timeoutMs=-1); // 将就绪的事件从内核事件表中复制到它的第二个参数 events 指向的数组

    int GetEventFd(std::size_t i) const;


    uint32_t GetEvents(size_t i) const;

private:
    int epollFd_;
    std::vector<struct epoll_event> events_;
};

#endif