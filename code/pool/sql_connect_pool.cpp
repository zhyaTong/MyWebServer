#include "sql_connect_pool.h"

SqlConnPool::~SqlConnPool()
{
    ClosePool();
}

SqlConnPool *SqlConnPool::Instance()
{ // 懒汉
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char *host, int port,
                       const char *user, const char *pwd,
                       const char *dbName, int connSize = 10)
{
    assert(connSize > 0);
    for (int i = 0; i < connSize; i++)
    {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql)
        {
            std::cout << "mysql init error!" << std::endl;
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
        if (!sql)
        {
            std::cout << "mysql connect error!" << std::endl;
        }
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId, 0, MAX_CONN_); // 初始化信号量
}

MYSQL *SqlConnPool::GetConn()
{
    MYSQL *sql = nullptr;
    if (connQue_.empty())
    {
        std::cout << "SqlConnPool is empty!" << std::endl;
        return nullptr;
    }
    sem_wait(&semId); // 等待信号量
    {
        std::lock_guard<std::mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL *sql)
{
    assert(sql);
    std::lock_guard<std::mutex> locker(mtx_);
    connQue_.push(sql);
    sem_post(&semId); // 释放信号量
}

void SqlConnPool::ClosePool()
{
    std::lock_guard<std::mutex> locker(mtx_);
    while (!connQue_.empty())
    {
        auto sql = connQue_.front();
        connQue_.pop();
        mysql_close(sql);
    }
    mysql_library_end();
}