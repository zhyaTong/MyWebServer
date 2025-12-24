//
// 封装实现自动增长的缓冲区
//
#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <atomic>
#include <vector>
#include <string>

class Buffer
{
public:
    Buffer(int initBufferSize = 1024);
    ~Buffer() = default;

    std::size_t WriteableBytes() const;   //  缓冲区中可写的字节数
    std::size_t ReadableBytes() const;    //  缓冲区中未读的字节数
    std::size_t PrependableBytes() const; //  缓冲区中已经读过的字节数

    const char *Peek() const;              //  返回要取出的数据的起始地址
    void EnsureWriteable(std::size_t len); // 判断缓冲区是否够用，不够用调用MakeSpace
    void HasWritten(std::size_t len);      // 写入len长度的数据，更新writePos_

    void Retrieve(std::size_t len);      // 取出len长度的未读数据，更新readPos_
    void RetrieveUntil(const char *end); // 取出到指定位置之间的未读数据，更新readPos_
    void RetrieveAll();                  // 清空缓冲区
    std::string RetrieveAllToStr();      // 将未读数据转为字符串返回，清空缓冲区

    const char *BeginWriteConst() const; // 返回要写入数据的起始地址
    char *BeginWrite();

    void Append(const std::string &str);
    void Append(const char *str, std::size_t len); // 向指定位置写入数据
    void Append(const void *data, std::size_t len);
    void Append(const Buffer &buff);

    ssize_t ReadFd(int fd, int *Errno);  // 向缓冲区中读入数据
    ssize_t WriteFd(int fd, int *Errno); // 从缓冲区中取出数据

private:
    char *BeginPtr_(); // 缓冲区起始地址
    const char *BeginPtr_() const;
    void MakeSpace_(std::size_t len); // 如果可写+已读空间不够直接resize，否则将未读取数据移动到起始地址

    std::atomic<std::size_t> readPos_;  // 已经取出数据的末尾
    std::atomic<std::size_t> writePos_; // 已经写入数据的末尾
    std::vector<char> buffer_;          // 缓冲区
};

#endif