//
// 负责把 Buffer 中的原始 HTTP 请求字节流，解析成“结构化的请求对象”
//
#ifndef HTTP_REQUEST
#define HTTP_REQUEST

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mysql/mysql.h>
#include "../buffer/buffer.h"
#include "../pool/sql_connect_RAII.h"

class HttpRequest
{
public:
    enum PARSE_STATE // 解析状态机
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE // 解析结果
    {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    void Init();              // 初始化
    bool parse(Buffer &buff); // 解析数据并更新状态机

    std::string path() const;
    std::string &path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string &key) const; // 从 POST 表单数据中获取参数值
    std::string GetPost(const char *key) const;

    bool IsKeepAlive() const; // 判断是否保持长连接

private:
    bool ParseRequestLine_(const std::string &line);
    void ParseHeader_(const std::string &line);
    void ParseBody_(const std::string &line);

    void ParsePath_(); // 处理默认路径映射
    void ParsePost_(); // 判断是否是 POST 请求，并调用表单解析
    void ParseFromUrlencoded_();

    static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_; // 所有 HTTP 头字段
    std::unordered_map<std::string, std::string> post_;   // POST 表单键值对

    static const std::unordered_set<std::string> DEFAULT_HTML;          // 页面集合
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG; // 页面类型标记
    static int ConverHex(char c);                                       // 十六进制字符转数字
};

#endif