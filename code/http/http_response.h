#ifndef HTTP_RESPONSE
#define HTTP_RESPONSE

#include "../buffer/buffer.h"
#include <sys/stat.h> //stat
#include <unordered_map>
#include "../log/log.h"

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1); // 初始化
    void MaskResponse(Buffer &buff);                                                                  // 返回响应
    void UnmapFile();                                                                                 // 关闭内存映射
    char *File();
    std::size_t FileLen() const;
    void ErrorContent(Buffer &buff, std::string message) const; // 错误响应体
    int Code() const { return code_; }

private:
    void AddStateLine_(Buffer &buff); // 添加响应行
    void AddHeader_(Buffer &buff);    // 添加响应头
    void AddContent_(Buffer &buff);   // 添加响应体

    void ErrorHtml_();          // 响应返回到错误页
    std::string GetFileType_(); // 判断文件类型

    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    char *mmFile_;
    struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE; // 后缀类型
    static const std::unordered_map<int, std::string> CODE_STATUS;         // 响应码对应状态
    static const std::unordered_map<int, std::string> CODE_PATH;           // 响应码对应路径
};
#endif