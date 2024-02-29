//
// Created by burning on 2024/2/26.
//

#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <cstring>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <condition_variable>
#include <memory>
#include <sstream>
#include "server.h"
#include "json11.hpp"
#include "cicd_cmd.h"


void event_service(int port, const char* file_path)
{
    int listen_fd = create_listen_fd(port);
    int notify, watch;
    create_notify_fd(file_path, &notify, &watch);

    int epoll_fd = epoll_create(128 + 1);
    if (epoll_fd == -1)
    {
        std::cerr << "epoll create failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev {};

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
    {
        std::cerr << "epoll add listen fd failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = notify;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, notify, &ev) == -1)
    {
        std::cerr << "epoll add listen fd failed" << std::endl;
        exit(EXIT_FAILURE);
    }


    struct epoll_event evs[128];
    memset(evs, 0x0, sizeof(evs));
    for (;;)
    {
        int nfds = epoll_wait(epoll_fd, evs, 128, 3600 * 1000);
        if (nfds == 0) {
            std::cout << "one hour" << std::endl;
            continue;
        }

        for (int i = 0; i < nfds; i++)
        {
            if ((evs[i].events & EPOLLERR) || (evs[i].events & EPOLLHUP) || !(evs[i].events & EPOLLIN))
            {
                close(evs[i].data.fd);
                continue;
            }

            if (evs[i].data.fd == listen_fd) {
                struct sockaddr_in in {};
                socklen_t sz = sizeof(in);
                int accept_fd = accept(listen_fd, (struct sockaddr*)&in, &sz);
                if (accept_fd == -1)
                {
                    std::cerr << "accept fd failed" << std::endl;
                    exit(EXIT_FAILURE);
                }
                memset(&ev, 0, sizeof(ev));
                ev.events = EPOLLIN;
                ev.data.fd = accept_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, accept_fd, &ev) == -1)
                {
                    std::cerr << "epoll add accept fd failed" << std::endl;
                    exit(EXIT_FAILURE);
                }
                continue;
            }

            if (evs[i].data.fd == notify) {
                handle_file_notify(notify, watch);
                continue;
            }

            ///accept fd
            int accept_client_fd = evs[i].data.fd;
            handle_http_event(accept_client_fd, CondVarMng::Instance().branch(), http_body_pred);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, accept_client_fd, &ev);
            close(accept_client_fd);
        }
    }

    close(notify);
    close(listen_fd);
}

int create_listen_fd(int port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        std::cout << "open socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in  sin{};
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if (bind(listen_fd, (struct  sockaddr *)&sin, sizeof(sin)) == -1)
    {
        std::cout << "bind port:" << port << "failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, 128) == -1)
    {
        std::cout << "listen port:" << port <<  "failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    return listen_fd;
}

int create_notify_fd(const char *file_path, int *notify, int *watch) {
    int notify_fd = inotify_init1(IN_NONBLOCK);
    if (notify_fd == -1)
    {
        perror("inotify_inti1");
        exit(EXIT_FAILURE);
    }

    int watch_fd = inotify_add_watch(notify_fd, file_path, IN_DELETE|IN_MODIFY);
    if (watch_fd == -1)
    {
        std::cerr << "Cannot watch " << file_path << ":" << strerror(errno);
        exit(EXIT_FAILURE);
    }
    *notify = notify_fd;
    *watch = watch_fd;
    return notify_fd;
}

void handle_file_notify(int notify, int watch) {
    char buf[4096]  __attribute__((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event* event;
    ssize_t len;

    for (;;) {
        len = read(notify, buf, sizeof(buf));
        if (len == -1 && errno != EAGAIN) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        if (len <= 0) {
            break;
        }

        for (char* ptr = buf; ptr < buf + len;
             ptr += sizeof(struct inotify_event) + event->len) {

            event = (const struct inotify_event*)ptr;

            if (event->mask & IN_MODIFY) {
                std::lock_guard<std::mutex> lk(CondVarMng::Instance().lock());
                CondVarMng::Instance().set_ready(true);
                CondVarMng::Instance().cond().notify_one();
            }
        }
    }
}


int my_url_callback(http_parser* parser, const char* at, size_t length) {
    return 0;
}

int my_body_callback(http_parser* parser, const char* at, size_t length) {
    auto* data = (CustomerData*)(parser->data);
    data->flag = 1;

    std::string err;
    std::string json(at, length);
    auto res = json11::Json::parse(json, err);
    if (!err.empty()) {
        std::cerr << "invalid json" << std::endl;
        data->flag = 0;
        return -1;
    }
    const auto& ref = res["ref"].string_value();
    auto const pos = ref.find_last_of('/');
    const auto branch = ref.substr(pos + 1);

    const auto& user_username = res["user_username"].string_value();

    auto& commits = res["commits"].array_items();
    auto& commit0 = commits[0];
    const auto& message = commit0["message"].string_value();

    strncpy(data->branch, branch.c_str(), 32);
    strncpy(data->user, user_username.c_str(), 32);
    strncpy(data->commit, message.c_str(), 32);
    return 0;
}

void handle_http_event(int client_fd, const std::shared_ptr<std::string>& which_branch, MyPred pred) {
    http_parser_settings settings;
    memset(&settings, 0x0, sizeof(settings));
    settings.on_url = my_url_callback;
    settings.on_body = my_body_callback;

    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);

    static constexpr int BUF_LEN = 80 * 1024;
    char buf[BUF_LEN] = { 0 };

    ssize_t recved = recv(client_fd, buf, BUF_LEN, 0);
    if (recved < 0) {
        std::cerr << "read client req failed" << std::endl;
        return;
    }

    CustomerData data{};
    memset(&data, 0, sizeof(data));
    parser.data = &data;
    size_t nparsed = http_parser_execute(&parser, &settings, buf, recved);

    if (nparsed != recved || data.flag == 0) {
        write(client_fd, "HTTP/1.0 400 Bad Request\n", 25);
        std::cerr << "invalid http request" << std::endl;
        return;
    }


    std::stringstream result;
    result << "HTTP/1.1 200 OK\n";
    result << "Data: " << Statistics::gm_time_str() << "\n";
    result << "Content-Type: text/plain;charset=ISO-8859-1\n";
    result << "Content-Length: 5\n";
    result << "\n";
    result << "pong\n";

    write(client_fd, result.str().c_str(), result.str().length());

    if (pred && pred(data)) {
        std::lock_guard<std::mutex> lk(CondVarMng::Instance().lock());
        *which_branch = data.branch;
        CondVarMng::Instance().set_ready(true);
        CondVarMng::Instance().cond().notify_one();
    }
}

bool http_body_pred(const CustomerData &cond) {
    std::string branch(cond.branch);
    std::string user(cond.user);
    std::string commit(cond.commit);

    ////根据自己的需求来控制
    return branch == "dev"
           || user == "root"
           || commit.compare(0, strlen("compile"), "compile") == 0;
}

