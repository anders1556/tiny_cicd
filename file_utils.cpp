//
// Created by burning on 2024/2/26.
//

#include <chrono>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include "file_utils.h"
#include "md5.h"


int fetch_dir_files(const std::string &path, const std::string &file_name_prefix, std::set<std::string> &files) {
    DIR* dp = opendir(path.c_str());
    if (!dp) {
        std::cerr << "Can't open dir:" << path << std::endl;
        return -1;
    }

    struct dirent* dirp;
    while ((dirp = readdir(dp)) != nullptr){
        if (dirp->d_type == DT_REG) {
            files.insert(file_name_prefix + "/" + dirp->d_name);
        }

        if (dirp->d_type == DT_DIR) {
            std::string dir_name(dirp->d_name);
            if (dir_name == "." || dir_name == ".."){
                continue;
            }

            std::string new_path(path);
            new_path += "/";
            new_path += dir_name;

            std::string new_prefix(file_name_prefix);
            if (new_prefix.empty()){
                new_prefix.assign(dirp->d_name);
            }else{
                new_prefix += "/";
                new_prefix += dir_name;
            }

            fetch_dir_files(new_path, new_prefix, files);
        }
    }

    return 0;
}

int calc_file_md5(const std::string &path, char *md5_str) {
    std::ifstream fin;
    fin.open(path, std::ios::in);
    if (!fin.is_open()){
        std::cerr << "can not open " << path << std::endl;
        return -1;
    }

    MD5_CTX md5Ctx;
    MD5Init(&md5Ctx);
    unsigned char buffer[1024] = {0};
    unsigned char md5_value[MD5_SIZE];

    while (true){
        auto size = fin.readsome((char*)buffer, sizeof(buffer));
        MD5Update(&md5Ctx, buffer, size);

        if (size < sizeof(buffer)){
            break;
        }
    }

    MD5Final(&md5Ctx, md5_value);
    for (int i=0; i<MD5_SIZE; i++){
        snprintf(md5_str + i*2, 2+1, "%02x", md5_value[i]);
    }
    md5_str[MD5_STR_LEN] = '\0'; // add end
    return 0;
}

int calc_dir_file_md5s(const std::string &dir, std::map<std::string, std::string> &md5s) {
    md5s.clear();

    std::set<std::string> files;
    std::string work_dir(dir);

    if (work_dir.back() == '/') {
        work_dir.pop_back();
    }

    fetch_dir_files(work_dir, work_dir, files);
    for (const auto& file : files) {
        char md5[MD5_STR_LEN+1] = {0};
        calc_file_md5(file, md5);
        md5s.insert(std::make_pair(file, md5));
    }

    return 0;
}

int fetch_new_diff_so(DbUtils& dbUtils, const CmdOpts& cmdOpts, std::set<std::string>& diff_file){
    std::map<std::string, std::string> last_md5s;
    std::map<std::string, std::string> new_md5s;

    change_work_dir(cmdOpts.work_dir);
    calc_dir_file_md5s(cmdOpts.sub_so_dir, new_md5s);
    fetch_last_file_md5s(dbUtils, last_md5s);
    dbUtils.insert_newest_file_md5s(new_md5s);

    for (const auto& it:new_md5s) {
        if (last_md5s.count(it.first) == 0){
            diff_file.insert(it.first);
        }
        if (last_md5s[it.first] != new_md5s[it.first]){
            diff_file.insert(it.first);
        }
    }
}
