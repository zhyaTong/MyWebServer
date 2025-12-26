#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <sys/socket.h>
#include <memory>
#include <unistd.h> //getcwd
#include <cassert>
#include <cstring>
#include <fcntl.h>

#include "epoll.h"
#include "../http/http_connect.h"

class WebServer
{
public:
    WebServer(int port, int trigMode, int timeoutMs, bool OptLinger);
    ~WebServer();
    void Start();

private:
    bool InitSocket_();
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);

    void DealListen_();
    void DealWrite_(HttpConn *client);
    void DealRead_(HttpConn *client);

    void SendError_(int fd, const char *info);
    void ExentTime_(HttpConn *client);
    void CloseConn_(HttpConn *client);

    void onRead_(HttpConn *client);
    void onWrite_(HttpConn *client);
    void onProcess_(HttpConn *client);

    static const int MAX_FD = 65536;

    static int SetFdNonblock(int fd);

    int port_;
    bool openLinger_;
    int timeoutMs_;
    bool isClose_;
    int listenFd_;
    char *srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unordered_map<int, HttpConn> users_;
    std::unique_ptr<Epoll> epoll_;
};

#endif