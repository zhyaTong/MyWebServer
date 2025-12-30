#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <ctime>
#include <cassert>
#include <functional>
#include <chrono>
#include <vector>
#include <unordered_map>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock; // 表示实现提供的拥有最小计次周期的时钟
typedef std::chrono::milliseconds MS;             // 毫秒
typedef Clock::time_point TimeStamp;              // 表示一个时间点

struct TimerNode
{
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode &t) const
    {
        return expires < t.expires;
    }
};

class HeapTimer
{
public:
    HeapTimer() { heap_.reserve(64); }
    ~HeapTimer() { clear(); }

    void adjust(int id, int newExpires);
    void add(int id, int timeOut, const TimeoutCallBack &cb);
    void clear();
    void tick();
    void pop();
    int GetNextTick();

private:
    void del_(size_t i);                    // 删除
    void siftup_(size_t i);                 // 上虑
    bool siftdown_(size_t index, size_t n); // 下虑
    void SwapNode_(size_t i, size_t j);     // 交换

    std::vector<TimerNode> heap_;         // 存放定时节点
    std::unordered_map<int, size_t> ref_; // 存放节点在数组中的位置
};

#endif