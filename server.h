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
 * gitlab http�ص������ݸ�ʽ
 */
struct CustomerData {
    int flag;
    char branch[32 + 1];
    char user[32 + 1];
    char commit[32 + 1];
};

/***
 * ����socket
 * @param port
 * @return
 */
int create_listen_fd(int port);

/***
 * ���������ļ��仯��
 * @param file_path
 * @param notify ����
 * @param watch
 * @return
 */
int create_notify_fd(const char* file_path, int* notify, int* watch);

/***
 * �����ļ��仯�¼�
 * @param notify
 * @param watch
 */
//epoll can't listen file readable
void handle_file_notify(int notify, int watch);

/***
 * ���������¼����ļ��仯�¼�
 * @param port �����˿���������gitlab���͵�http����
 * @param file_path ������������Ŀ¼�ļ��仯
 */
void event_service(int port, const char* file_path);

/***
 * url�ص�
 * @param parser
 * @param at
 * @param length
 * @return
 */
int my_url_callback(http_parser* parser, const char* at, size_t length);

/***
 * http body�ص�
 * @param parser
 * @param at
 * @param length
 * @return
 */
int my_body_callback(http_parser* parser, const char* at, size_t length);

/***
 * ��http�����жϵ�ν��
 * @param cond
 * @return
 */
bool http_body_pred(const CustomerData& cond);
typedef bool (*MyPred)(const CustomerData&);

/***
 * ����http�¼�
 * @param client_fd
 * @param which_branch
 */
void handle_http_event(int client_fd, const std::shared_ptr<std::string>& which_branch, MyPred pred = nullptr);

#endif //MD5UTILS_SERVER_H
