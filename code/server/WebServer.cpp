#include "WebServer.h"

WebServer::WebServer(int port, int trigMode, int timeoutMs, bool OptLinger)
    : port_(port), openLinger_(OptLinger), timeoutMs_(timeoutMs), isClose_(false),
      epoll_(new Epoll())
{
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;

    InitEventMode_(trigMode);
    if (!InitSocket_())
    {
        isClose_ = true;
    }
}

WebServer::~WebServer()
{
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
}

void WebServer::InitEventMode_(int trigMode)
{
    listenEvent_ = EPOLLRDHUP;
    connEvent_ - EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        connEvent_ |= EPOLLET;
        listenEvent_ |= EPOLLET;
        break;
    default:
        connEvent_ |= EPOLLET;
        listenEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::Start()
{
    int timeMs = -1; // epoll_wait无事件将阻塞
    while (!isClose_)
    {
        int eventCnt = epoll_->Wait(timeMs);
        for (int i = 0; i < eventCnt; i++)
        {
            int fd = epoll_->GetEventFd(i);
            uint32_t events = epoll_->GetEvents(i);
            if (fd == listenFd_)
            {
                DealListen_();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if (events & EPOLLIN)
            {
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            }
            else if (events & EPOLLOUT)
            {
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            }
            else
            {
                std::cout << "Unexpected event" << std::endl;
            }
        }
    }
}

void WebServer::SendError_(int fd, const char *info)
{
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    close(fd);
}

void WebServer::CloseConn_(HttpConn *client)
{
    assert(client);
    epoll_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr)
{
    assert(fd > 0);
    users_[fd].Init(fd, addr);
    epoll_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonblock(fd);
}

void WebServer::DealListen_()
{
    struct sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    do
    {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if (fd < 0)
        {
            return;
        }
        else if (HttpConn::userCount >= MAX_FD)
        {
            SendError_(fd, "Server busy!");
            return;
        }
        AddClient_(fd, addr);
    } while (listenEvent_ & EPOLLET);
}

void WebServer::DealRead_(HttpConn *client)
{
    assert(client);
    onRead_(client);
}

void WebServer::DealWrite_(HttpConn *client)
{
    assert(client);
    onWrite_(client);
}

void WebServer::onRead_(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN)    //出现错误        ret==0 对端关闭 
    {
        CloseConn_(client);
        return;
    }
    onProcess_(client);
}

void WebServer::onProcess_(HttpConn *client)
{
    if (client->process())
    {
        epoll_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    }
    else
    {
        epoll_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::onWrite_(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if (client->ToWriteBytes() == 0)
    {//传输完成
        if (client->IsKeepAlive())//是否保持连接
        {
            onProcess_(client);
            return;
        }
    }
    else if (ret < 0)
    {
        if (writeErrno == EAGAIN)   //写缓冲区满
        {//等下次可写，继续传输
            epoll_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

bool WebServer::InitSocket_()
{
    int ret;
    struct sockaddr_in addr{};
    if (port_ > 65535 || port_ < 1024)
    {
        return false;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    struct linger optLinger = {0};
    if (openLinger_)
    { // 优雅关闭：直到所有数据发送完毕或超时
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0)
        return false;

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0)
    {
        close(listenFd_);
        return false;
    }

    int optval = 1; // 端口复用
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(optval));
    if (ret == -1)
    {
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if (ret < 0)
    {
        close(listenFd_);
        return false;
    }
    ret = epoll_->AddFd(listenFd_, listenEvent_ | EPOLLIN);
    if (ret == 0)
    {
        close(listenFd_);
        return false;
    }
    SetFdNonblock(listenFd_);
    return true;
}

int WebServer::SetFdNonblock(int fd)
{
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}