#include "log.h"

Log::Log()
{
    lineCount_ = 0;
    isAsync_ = false;
    writeThread_ = nullptr;
    deque_ = nullptr;
    toDay_ = 0;
    fp_ = nullptr;
}

Log::~Log()
{
    if (writeThread_ && writeThread_->joinable())
    {
        while (!deque_->empty())
        {
            deque_->flush();    
        }
        deque_->Close();    //pop()返回false,线程退出循环
        writeThread_->join();   //当前线程（主线程）阻塞等待日志写线程执行完毕
    }
    if (fp_)
    {
        std::lock_guard<std::mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

int Log::GetLevel()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level)
{
    std::lock_guard<std::mutex> locker(mtx_);
    level_ = level;
}

void Log::init(int level = 1, const char *path, const char *suffix, int maxDequeSize)
{
    isOpen_ = true;
    level_ = level;
    if (maxDequeSize > 0)
    {
        isAsync_ = true;
        if (!deque_)
        {
            std::unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            deque_ = std::move(newDeque);

            std::unique_ptr<std::thread> newThread(new std::thread(FlushLogThread));
            writeThread_ = std::move(newThread);
        }
    }
    else
    {
        isAsync_ = false;
    }

    lineCount_ = 0;

    time_t timer = time(nullptr);   //获得当前时间戳
    struct tm *sysTime = localtime(&timer); //转换为本地时间结构
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
             path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    toDay_ = t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(mtx_);
        buff_.RetrieveAll();
        if (fp_)
        {
            flush();
            fclose(fp_);
        }
        fp_ = fopen(fileName, "a"); //追加
        if (fp_ == nullptr)
        {
            mkdir(path_, 0777); //所有用户都可读/写/执行
            fp_ = fopen(fileName, "a");
        }
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char *format, ...)
{
    struct timeval now = {0, 0};    //{秒，微秒}
    gettimeofday(&now, nullptr);    //获取当前时间
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);  //localtime() 只能处理 time_t（秒级时间戳）
    struct tm t = *sysTime;
    va_list vaList;

    //如果日志日期不是当天或者日志超过最大行数，创建一个新的日志文件
    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0)))
    {
        std::unique_lock<std::mutex> locker(mtx_);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (toDay_ != t.tm_mday)
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }
        else
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_ / MAX_LINES), suffix_);
        }

        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }

    {
        std::unique_lock<std::mutex> locker(mtx_);
        lineCount_++;

        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                         t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buff_.HasWritten(n);

        AppendLogLevelTitle_(level);

        va_start(vaList, format);   //初始化vaList
        int m = vsnprintf(buff_.BeginWrite(), buff_.WriteableBytes(), format, vaList);  //将vaList中的参数以format格式写入缓冲区
        va_end(vaList); //释放vaList相关资源

        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);

        if (isAsync_ && deque_ && !deque_->full())  //异步
        {
            deque_->push_back(buff_.RetrieveAllToStr());
            return;
        }
        else    //同步
        {
            fputs(buff_.Peek(), fp_);   //将缓冲区内容追加写入文件，遇到\0停止，先写到 FILE 缓冲区（用户态），不保证写到磁盘
        }
        buff_.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle_(int level)
{
    switch (level)
    {
    case 0:
        buff_.Append("[debug]: ", 9);
        break;
    case 1:
        buff_.Append("[info]: ", 8);
        break;
    case 2:
        buff_.Append("[warn]: ", 8);
        break;
    case 3:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info]: ", 8);
        break;
    }
}

void Log::flush()
{
    if (isAsync_)
    {
        deque_->flush();
    }
    fflush(fp_);    //强制把用户态缓冲区中的日志数据立即写入到文件（内核态）
}

void Log::AsyncWrite_()
{
    std::string str = "";
    while (deque_->pop(str))
    {
        std::lock_guard<std::mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}

Log *Log::Instance()
{
    static Log log;
    return &log;
}

void Log::FlushLogThread()
{
    Log::Instance()->AsyncWrite_();
}