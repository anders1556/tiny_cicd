//
// Created by burning on 2024/2/26.
//

#ifndef MD5UTILS_CICD_CMD_H
#define MD5UTILS_CICD_CMD_H

#include <string>
#include <memory>
#include <chrono>
#include <set>
#include "db_utils.h"


struct CmdOpts{
    int port;
    int compile_opt;
    int daemon;
    int just_so_diff;//只运行比较一次前后两次so的不同
    int just_deploy_once;//运行一次部署过程
    std::string work_dir;//工作目录
    std::string sub_so_dir;//work_dir下生成so的目录
    std::string file_path;//检测此文件变化
    std::string db_name;
};

class Statistics{
public:
    Statistics(){
        start = std::chrono::steady_clock::now();
    }

    static inline std::string gm_time_str(){
        std::time_t t = std::time(nullptr);
        struct tm gm = *gmtime(&t);
        char mbstr[100];
        if (std::strftime(mbstr, sizeof(mbstr), "%a, %d %b %Y %H:%M:%S GMT", &gm)) {
            std::cout << mbstr << std::endl;
        }
        return mbstr;
    }

    ~Statistics(){
        end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        std::cout << "Statistics --> elapsed time: " << elapsed_seconds.count() << "s" << std::endl;

        gm_time_str();
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> start, end;
};

/***
 * 执行linux命令
 * @param commands linux命令
 * @return
 */
int exec_naive_cmd(const std::string& commands);

/***
 * 解析命令行参数
 * @param argc
 * @param argv
 * @param opts
 * @return
 */
int parse_cmd_arg(int argc, char** argv, CmdOpts* opts);

/***
 * 拉取分支代码
 * @param branch
 * @return
 */
int pull_project_source(const std::string& branch);


enum{
    COMPILE_OPTION_ADD,//增量编译
    COMPILE_OPTION_ALL,//全量编译
};

/***
 * 执行工程的编译脚本(根据具体工程编译的方式)
 * @param compile_option
 * @return
 */
int compile_project(int compile_option = COMPILE_OPTION_ALL);

/***
 * 将编译出来的so文件拷贝到指定地址(根据具体的工程部署方式)
 * @param src_path 本地指定路径
 * @param user_at_ip 例如:root@114.114.114.114
 * @param dest_path 远程指定路径
 * @return
 */
int upload_project_files(const std::string& src_path, const std::string& user_at_ip, const std::string& dest_path);

/***
 * 部署环境(根据具体的工程部署方式)
 * @param cmdOpts
 * @return
 */
int deploy(const CmdOpts& cmdOpts, DbUtils& dbUtils);

/***
 * 根据事件来触发部署环境(根据具体的工程部署方式)
 * @param cmdOpts
 * @param dbUtils
 * @return
 */
int deploy_by_notify(const CmdOpts& cmdOpts, DbUtils& dbUtils);

/***
 * ssh远程目标服务地址
 * @param user_at_ip 例如:root@114.114.114.114
 * @param cmd 要在远程机器上执行的命令
 * @return
 */
int exec_remote_cmd(const std::string& user_at_ip, const std::string& cmd);

/***
 * 进入执行目录
 * @param work_dir
 * @return
 */
int change_work_dir(const std::string& work_dir);

/***
 * 特有的预编译命令
 * @return
 */
int my_project_pre_compile_cmd();
#endif //MD5UTILS_CICD_CMD_H
