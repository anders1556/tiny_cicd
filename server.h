//
// Created by burning on 2024/2/26.
//

#ifndef MD5UTILS_SERVER_H
#define MD5UTILS_SERVER_H


#include <deque>
#include <condition_variable>
#include "http_parser.h"

class CondVarMng {
public:
    std::shared_ptr<std::string> branch() {
        return ptr;
    }

    std::mutex& lock() {
        return m;
    }

    std::condition_variable& cond() {
        return cv;
    }

    bool get_ready() const {
        return ready;
    }

    void set_ready(bool b) {
        ready = b;
    }
public:
    static CondVarMng& Instance() {
        static CondVarMng instance;
        return instance;
    }
    CondVarMng(const CondVarMng&) = delete;
    CondVarMng(CondVarMng&&) = delete;
    CondVarMng& operator=(const CondVarMng&) = delete;
    CondVarMng& operator=(CondVarMng&&) = delete;
private:
    CondVarMng() = default;
    ~CondVarMng() = default;

private:
    std::shared_ptr<std::string> ptr = std::make_shared<std::string>();
    bool ready = false;
    std::mutex m;
    std::condition_variable cv;
};

/***
 * gitlab http回调的数据格式
 */
struct CustomerData {
    int flag;
    char branch[32 + 1];
    char user[32 + 1];
    char commit[32 + 1];
};

/***
 * 创建socket
 * @param port
 * @return
 */
int create_listen_fd(int port);

/***
 * 创建监听文件变化的
 * @param file_path
 * @param notify 出参
 * @param watch
 * @return
 */
int create_notify_fd(const char* file_path, int* notify, int* watch);

/***
 * 处理文件变化事件
 * @param notify
 * @param watch
 */
//epoll can't listen file readable
void handle_file_notify(int notify, int watch);

/***
 * 处理网络事件和文件变化事件
 * @param port 监听端口用来接收gitlab发送的http请求
 * @param file_path 用来监听编译目录文件变化
 */
void event_service(int port, const char* file_path);

/***
 * url回调
 * @param parser
 * @param at
 * @param length
 * @return
 */
int my_url_callback(http_parser* parser, const char* at, size_t length);

/***
 * http body回调
 * @param parser
 * @param at
 * @param length
 * @return
 */
int my_body_callback(http_parser* parser, const char* at, size_t length);

/***
 * 对http内容判断的谓词
 * @param cond
 * @return
 */
bool http_body_pred(const CustomerData& cond);
typedef bool (*MyPred)(const CustomerData&);

/***
 * 处理http事件
 * @param client_fd
 * @param which_branch
 */
void handle_http_event(int client_fd, const std::shared_ptr<std::string>& which_branch, MyPred pred = nullptr);

#endif //MD5UTILS_SERVER_H
