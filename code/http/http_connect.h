#ifndef HTTP_CONNECT
#define HTTP_CONNECT

#include "http_request.h"
#include "http_response.h"
#include "../log/log.h"
#include <arpa/inet.h>

class HttpConn
{
public:
    HttpConn();
    ~HttpConn();

    void Init(int sockFd, const sockaddr_in &addr);
    ssize_t read(int *saveErrno);
    ssize_t write(int *saveErrno);

    void Close();

    int GetFd() const;
    int GetPort() const;

    const char *GetIp() const;
    sockaddr_in GetAddr() const;

    bool process();

    int ToWriteBytes();

    bool IsKeepAlive() const;

    static bool isET;
    static const char *srcDir;
    static std::atomic<int> userCount;

private:
    int fd_;
    struct sockaddr_in addr_;

    bool isClose_;

    int iovCnt_{};

    struct iovec iov_[2]{};

    Buffer readBuff_;
    Buffer writeBuff_;

    HttpRequest request_;
    HttpResponse response_;
};

#endif