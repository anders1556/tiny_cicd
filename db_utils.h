//
// Created by burning on 2024/2/27.
//

#ifndef MD5UTILS_DB_UTILS_H
#define MD5UTILS_DB_UTILS_H
#include <sqlite3.h>
#include <string>
#include <map>

enum {
    INVALID_VERSION = -1,
};

int callback_version(void*data, int argc, char**argv, char** col_name);
int callback_file_md5s(void* data, int argc, char**argv, char** col_name);

class DbUtils{
private:
    sqlite3* db;
    char* errmsg;
    std::string db_file;
    bool is_ok;
public:
    static long long version;
    static std::map<std::string, std::string> file_md5s;
public:
    DbUtils(const std::string& db_file);
    ~DbUtils();

    bool ok() {return is_ok;}
    std::string gen_so_version();
    int create_table();
    int select_last_version(long long& last_version);
    int select_last_file_md5s(std::map<std::string, std::string>& output_md5s);
    int insert_newest_file_md5s(std::map<std::string, std::string> md5s);

};

/***
 * 从数据库查询最近一次的md5值
 * @param dbUtils
 * @param last_md5s
 * @return
 */
int fetch_last_file_md5s(DbUtils& dbUtils, std::map<std::string, std::string>& last_md5s);

#endif //MD5UTILS_DB_UTILS_H
