//
// Created by burning on 2024/2/26.
//

#include <iostream>
#include <getopt.h>
#include <sstream>
#include <chrono>
#include <mutex>
#include <zconf.h>
#include <cstring>
#include "cicd_cmd.h"
#include "server.h"
#include "file_utils.h"

int exec_naive_cmd(const std::string &commands) {
    FILE* fp = popen(commands.c_str(), "r");
    char buffer[1024] = { 0 };
    while (fgets(buffer, sizeof(buffer), fp) != nullptr)
    {
        std::cout << buffer << std::endl;
    }
    pclose(fp);
    return 0;
}

int compile_project(int compile_option) {
    char curr_path[2048] = {0};
    getcwd(curr_path, sizeof(curr_path));
    std::cout << "curr path:" << curr_path << ";  cd into script dir" << std::endl;

    chdir("/root");//compile script dir

    std::string commands;
    if (compile_option)
    {
        commands += "bash ./make_all.sh;";//
    } else{
        commands += "bash ./make_all_add.sh;";//
    }

    FILE *fp = popen(commands.c_str(), "r");
    char buffer[8192] = {0};
    while(fgets(buffer, sizeof(buffer), fp) != nullptr)
    {
        char* find_error = strstr(buffer, "Error 1");
        if (find_error != nullptr)
        {
            std::cout << find_error << std::endl;
        }
    }
    pclose(fp);

    chdir(curr_path);
    std::cout << "**********************compile over***********************" << std::endl;
    return 0;
}

int parse_cmd_arg(int argc, char **argv, CmdOpts* opts) {
    int c;

    opts->port = 12345;
    opts->compile_opt = COMPILE_OPTION_ADD;

    while((c = getopt(argc, argv, "hvcuxp:s:w:W:f:d:")) != -1){
        switch (c) {
            case 'h':
            case '?':
                std::cout << "**************************************************" << std::endl;
                std::cout << "simple cicd server" << std::endl;
                std::cout << "-h help" << std::endl;
                std::cout << "-v version" << std::endl;
                std::cout << "-p [port] default 12345" << std::endl;
                std::cout << "-s [0/1] make_all_add/make_all default 0" << std::endl;
                std::cout << "-f [watch file] " << std::endl;
                std::cout << "-d [db file]" << std::endl;
                std::cout << "-w [workspace dir]" << std::endl;
                std::cout << "-W [so filepath]" << std::endl;
                std::cout << "-f [watch log file]" << std::endl;
                std::cout << "-c [diff last and new so difference]";
                std::cout << "-u [compile and deploy once]";
                std::cout << "-x [run as daemon server]";
                std::cout << "**************************************************" << std::endl;
                exit(0);
            case 'v':
                std::cout << "**************************************************" << std::endl;
                std::cout << "2024-2-27" << std::endl;
                std::cout << "**************************************************" << std::endl;
                exit(0);
            case 'p':
                std::cout << "port:" << optarg << std::endl;
                opts->port = std::strtol(optarg, nullptr, 10);
                break;
            case 's':
                if (std::atoi(optarg)) {
                    opts->compile_opt = COMPILE_OPTION_ALL;
                    std::cout << "compile all" << std::endl;
                }else {
                    opts->compile_opt = COMPILE_OPTION_ADD;
                    std::cout << "compile add" << std::endl;
                }
                break;
            case 'w':
                opts->work_dir.assign(optarg);
                std::cout << "work base dir: " << optarg << std::endl;
                break;
            case 'W':
                opts->sub_so_dir.assign(optarg);
                std::cout << "so dir: " << optarg << std::endl;
                break;
            case 'f':
                opts->file_path.assign(optarg);
                std::cout << "watch log dir: " << optarg << std::endl;
                break;
            case 'd':
                opts->db_name.assign(optarg);
                std::cout << "db filename: " << optarg << std::endl;
                break;
            case 'c':
                opts->just_so_diff = 1;
                std::cout << "just get the new different so "<< std::endl;
                break;
            case 'u':
                opts->just_deploy_once = 1;
                std::cout << "just get deploy once "<< std::endl;
                break;
            case 'x':
                opts->daemon = 1;
                std::cout << "run cicd as daemon program"<< std::endl;
                break;

            default:
                std::cout << "Wrong arguments given!!!" << std::endl;
                exit(EXIT_FAILURE);
        }
    }
    return 0;
}

//关闭服务
//上传文件
//启动服务
int deploy(const CmdOpts& cmdOpts, DbUtils& dbUtils) {
    Statistics statistics;

    //change_work_dir(cmdOpts.work_dir);
    my_project_pre_compile_cmd();
    pull_project_source(CondVarMng::Instance().branch()->c_str());
    compile_project(cmdOpts.compile_opt);
    std::set<std::string> upload_files;
    fetch_new_diff_so(dbUtils, cmdOpts, upload_files);


    exec_remote_cmd("root@10.10.10.10", "cd /root/;bash ./stop.sh;");
    for (const auto& file:upload_files){
        upload_project_files(file, "root@10.10.10.10", "~/");
    }
    exec_remote_cmd("root@10.10.10.10", "cd /root/;bash ./start.sh;");

    sleep(3);
    return 0;
}

int deploy_by_notify(const CmdOpts &cmdOpts, DbUtils &dbUtils) {
    std::unique_lock<std::mutex> lk(CondVarMng::Instance().lock());
    CondVarMng::Instance().cond().wait(lk, [] {return CondVarMng::Instance().get_ready(); });
    auto begin = std::chrono::steady_clock::now();
    CondVarMng::Instance().set_ready(false);
    lk.unlock();

    deploy(cmdOpts, dbUtils);
    return 0;
}

int exec_remote_cmd(const std::string &user_at_ip, const std::string &cmd) {
    std::stringstream commands;
    commands << "ssh -t " << user_at_ip <<" \"" ;
    commands << "source ~/.bash_profile;";
    commands << cmd << "sleep 5; exit;\"" ;
    exec_naive_cmd(commands.str());
    std::cout << "*************************ssh over***********************" << std::endl;
    return 0;
}

int upload_project_files(const std::string &src_path, const std::string &user_at_ip, const std::string &dest_path) {
    std::stringstream commands;
    commands << "scp -r " << src_path << user_at_ip << ":" << dest_path;
    exec_naive_cmd(commands.str());
    std::cout << "**********************scp over*********************" << std::endl;
    return 0;
}

int change_work_dir(const std::string &work_dir) {
    char curr_path[2048] = {0};
    getcwd(curr_path, sizeof(curr_path));

    std::cout << "curr path:" << curr_path << std::endl;
    chdir(work_dir.c_str());
    std::cout << "change dir:" << work_dir << std::endl;
    std::cout << "***********************cd over*************************" << std::endl;
    return 0;
}

int my_project_pre_compile_cmd() {
    std::cout << "**********pre compile command**********" << std::endl;

    static int once = -1;
    if (once < 0) {
        once++;
        std::string commands;
        commands += "ls -t";

        exec_naive_cmd(commands);
    }

    std::cout << "***********pre compile command*************" << std::endl;
    return 0;
}


int pull_project_source(const std::string &branch) {
    std::string commands;
    commands += "git checkout ";
    commands += branch;
    exec_naive_cmd(commands);
    std::cout << "******************git checkout over********************" << std::endl;

    commands.clear();
    commands += "git pull";
    exec_naive_cmd(commands);
    std::cout << "******************git pull over************************" << std::endl;
    return 0;
}

