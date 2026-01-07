#ifndef SQL_CONNECT_POOL_H
#define SQL_CONNECT_POOL_H

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <cassert>
#include <iostream>

class SqlConnPool
{
public:
    static SqlConnPool *Instance();

    MYSQL *GetConn();
    void FreeConn(MYSQL *sql);

    void Init(const char *host, int port,
              const char *user, const char *pwd,
              const char *dbName, int connSize);
    void ClosePool();

    SqlConnPool(const SqlConnPool &) = delete;
    SqlConnPool &operator=(const SqlConnPool &) = delete;

private:
    SqlConnPool() = default;
    ~SqlConnPool();

    int MAX_CONN_;

    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId;
};

#endif