#include "epoll.h"
#include <unistd.h>
#include <cassert>

Epoll::Epoll(int maxEvent):events_(maxEvent),epollFd_(epoll_create(512)){
    assert(epollFd_>=0 && events_.size()>0);
}

Epoll::~Epoll(){
    close(epollFd_);
}

bool Epoll::AddFd(int fd,uint32_t events){
    if(fd<0)    return false;
    epoll_event ev={0};
    ev.data.fd=fd;
    ev.events=events;
    return epoll_ctl(epollFd_,EPOLL_CTL_ADD,fd,&ev)==0;
}

bool Epoll::ModFd(int fd,uint32_t events){
    if(fd<0)    return false;
    epoll_event ev={0};
    ev.data.fd=fd;
    ev.events=events;
    return epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd,&ev)==0;
}

bool Epoll::DelFd(int fd){
    if(fd<0)    return false;
    epoll_event ev={0};
    return epoll_ctl(epollFd_,EPOLL_CTL_DEL,fd,&ev)==0;
}

int Epoll::Wait(int timeoutMs){
    return epoll_wait(epollFd_,&events_[0],static_cast<int>(events_.size()),timeoutMs);
}

int Epoll::GetEventFd(std::size_t i) const{
    assert(i<events_.size() && i>=0);
    return events_[i].data.fd;
}

uint32_t Epoll::GetEvents(std::size_t i) const{
    assert(i<events_.size() && i>=0);
    return events_[i].events;
}