#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <functional>
#include <memory>
#include <cassert>

class ThreadPool
{
private:
    explicit ThreadPool(size_t threadCount) : pool_(std::make_shared<Pool>())
    {
        assert(threadCount>0);
        for (size_t i = 0; i < threadCount; i++)
        {
            std::thread([pool = pool_]
                        {
                std::unique_lock<std::mutex> locker(pool->mtx);
                while(true){
                    if(!pool->tasks.empty()){
                        auto task=std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }else if(pool->isClose){
                        break;
                    }else{
                        pool->cv.wait(locker);
                    }
                } })
                .detach();
        }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool &&) = default;

    ~ThreadPool()
    {
        if (static_cast<bool>(pool_))
        {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClose = true;
            }
            pool_->cv.notify_all();
        }
    }

    template <class T>
    void AddTask(T &&task)
    {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<T>(task));
        }
        pool_->cv.notify_one();
    }

private:
    struct Pool
    {
        std::mutex mtx;
        std::condition_variable cv;
        std::queue<std::function<void()>> tasks;
        bool isClose = false;
    };

    std::shared_ptr<Pool> pool_;
};

#endif