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
    int just_so_diff;//ֻ���бȽ�һ��ǰ������so�Ĳ�ͬ
    int just_deploy_once;//����һ�β������
    std::string work_dir;//����Ŀ¼
    std::string sub_so_dir;//work_dir������so��Ŀ¼
    std::string file_path;//�����ļ��仯
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
 * ִ��linux����
 * @param commands linux����
 * @return
 */
int exec_naive_cmd(const std::string& commands);

/***
 * ���������в���
 * @param argc
 * @param argv
 * @param opts
 * @return
 */
int parse_cmd_arg(int argc, char** argv, CmdOpts* opts);

/***
 * ��ȡ��֧����
 * @param branch
 * @return
 */
int pull_project_source(const std::string& branch);


enum{
    COMPILE_OPTION_ADD,//��������
    COMPILE_OPTION_ALL,//ȫ������
};

/***
 * ִ�й��̵ı���ű�(���ݾ��幤�̱���ķ�ʽ)
 * @param compile_option
 * @return
 */
int compile_project(int compile_option = COMPILE_OPTION_ALL);

/***
 * �����������so�ļ�������ָ����ַ(���ݾ���Ĺ��̲���ʽ)
 * @param src_path ����ָ��·��
 * @param user_at_ip ����:root@114.114.114.114
 * @param dest_path Զ��ָ��·��
 * @return
 */
int upload_project_files(const std::string& src_path, const std::string& user_at_ip, const std::string& dest_path);

/***
 * ���𻷾�(���ݾ���Ĺ��̲���ʽ)
 * @param cmdOpts
 * @return
 */
int deploy(const CmdOpts& cmdOpts, DbUtils& dbUtils);

/***
 * �����¼����������𻷾�(���ݾ���Ĺ��̲���ʽ)
 * @param cmdOpts
 * @param dbUtils
 * @return
 */
int deploy_by_notify(const CmdOpts& cmdOpts, DbUtils& dbUtils);

/***
 * sshԶ��Ŀ������ַ
 * @param user_at_ip ����:root@114.114.114.114
 * @param cmd Ҫ��Զ�̻�����ִ�е�����
 * @return
 */
int exec_remote_cmd(const std::string& user_at_ip, const std::string& cmd);

/***
 * ����ִ��Ŀ¼
 * @param work_dir
 * @return
 */
int change_work_dir(const std::string& work_dir);

/***
 * ���е�Ԥ��������
 * @return
 */
int my_project_pre_compile_cmd();
#endif //MD5UTILS_CICD_CMD_H
