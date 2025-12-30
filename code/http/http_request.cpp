#include "http_request.h"
#include <cassert>
#include <algorithm>
#include <regex>

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

std::string HttpRequest::path() const
{
    return path_;
}

std::string &HttpRequest::path()
{
    return path_;
}

std::string HttpRequest::method() const
{
    return method_;
}

std::string HttpRequest::version() const
{
    return version_;
}

std::string HttpRequest::GetPost(const std::string &key) const
{
    assert(key != "");
    if (post_.count(key) == 1)
    {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char *key) const
{
    assert(key != nullptr);
    if (post_.count(key) == 1)
    {
        return post_.find(key)->second;
    }
    return "";
}

void HttpRequest::Init()
{
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const
{
    if (version_ == "1.1") {
        auto it = header_.find("Connection");
        return it == header_.end() || it->second != "close";
    }
    if (version_ == "1.0") {
        auto it = header_.find("Connection");
        return it != header_.end() && it->second == "keep-alive";
    }
    return false;
}

bool HttpRequest::parse(Buffer &buff)
{
    const char CRLF[] = "\r\n";
    if (buff.ReadableBytes() <= 0)
        return false;
    while (buff.ReadableBytes() && state_ != FINISH)
    {
        const char *lineEnd = std::search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2); // 找\r\n的位置
        std::string line(buff.Peek(), lineEnd);
        switch (state_)
        {
        case REQUEST_LINE:
            if (!ParseRequestLine_(line))
            {
                return false;
            }
            ParsePath_();
            break;
        case HEADERS:
            ParseHeader_(line);
            if (buff.ReadableBytes() <= 2)
            {
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        // 如果一行没读完就停，等待下一次读，更新已经读完的位置
        if (lineEnd == buff.BeginWrite())
        {
            if (method_ == "POST" && state_ == FINISH)
            {
                buff.RetrieveUntil(lineEnd);
            }
            break;
        }
        buff.RetrieveUntil(lineEnd + 2);
    }
    return true;
}

void HttpRequest::ParsePath_()
{
    if (path_ == "/")
    {
        path_ = "/index.html";
    }
    else
    {
        for (auto &item : DEFAULT_HTML)
        {
            if (item == path_)
            {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine_(const std::string &line)
{
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten))
    {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    return false;
}

void HttpRequest::ParseHeader_(const std::string &line)
{
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten))
    {
        header_[subMatch[1]] = subMatch[2];
    }
    else
    {
        state_ = BODY;
    }
}

void HttpRequest::ParseBody_(const std::string &line)
{
    body_ = line;
    ParsePost_();
    state_ = FINISH;
}

//16转10
int HttpRequest::ConverHex(char c)
{
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return c;
}

//登陆 注册
void HttpRequest::ParsePost_()
{
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded")
    {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_))
        {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            if (tag == 0 || tag == 1)
            {
                bool isLogin = (tag == 1);
            }
        }
    }
}

//解析请求体的内容放入post_
void HttpRequest::ParseFromUrlencoded_()
{
    if (body_.size() == 0)
        return;

    std::string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;
    for (; i < n; i++)
    {
        char c = body_[i];
        switch (c)
        {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            // LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }

    assert(j <= i);
    if (post_.count(key) == 0 && j < i)
    {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}