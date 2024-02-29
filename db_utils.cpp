//
// Created by burning on 2024/2/27.
//

#include <iostream>
#include <sstream>
#include <chrono>
#include "db_utils.h"

long long DbUtils::version;
std::map<std::string, std::string> DbUtils::file_md5s;

DbUtils::DbUtils(const std::string &db_file) :db_file(db_file), is_ok(false){
    int rc = sqlite3_open(db_file.c_str(), &db);
    if (rc){
        std::cerr << "Can't open database" << sqlite3_errmsg(db);
        return;
    }

    is_ok = true;
}

DbUtils::~DbUtils() {
    sqlite3_close(db);
}

int DbUtils::create_table() {
    int rc = sqlite3_exec(db, "create table if not exists md5s(id Integer primary key autoincrement, version Integer, name Char, md5 Char)", NULL, NULL, &errmsg);
    if (rc) {
        std::cerr << "Create table failed, rc:" << rc << ",errmsg:" << errmsg << std::endl;
        sqlite3_free(errmsg);
        return rc;
    }
    return 0;
}

int DbUtils::select_last_version(long long& last_version) {
    int rc = sqlite3_exec(db, "select version from md5s order by version desc limit 1", callback_version, NULL, &errmsg);
    if (rc) {
        std::cerr << "Select version failed, rc:" << rc << ",errmsg:" << errmsg << std::endl;
        sqlite3_free(errmsg);
        return rc;
    }
    last_version = DbUtils::version;
    return 0;
}

int DbUtils::select_last_file_md5s(std::map<std::string, std::string>& output_md5s) {
    std::stringstream query;
    query << "select version, name, md5 from md5s";
    if (version != INVALID_VERSION) {
        query <<  " where version = " << version ;
    }
    int rc = sqlite3_exec(db, query.str().c_str(), callback_file_md5s, NULL, &errmsg);
    if (rc){
        std::cerr << "Select table failed, rc:" << rc << ",errmsg:" << errmsg << std::endl;
        sqlite3_free(errmsg);
        return rc;
    }

    for (auto kv:file_md5s){
        output_md5s.insert(kv);
    }
    return 0;
}

int DbUtils::insert_newest_file_md5s(std::map<std::string, std::string> md5s) {
    std::string insert_sql("insert into md5s (version, name, md5)values ");
    std::string table_version(gen_so_version());

    for (const auto& it:md5s)
    {
        std::stringstream ss;

        ss << insert_sql << "('" << table_version << "','" << it.first << "','" << it.second << "');";
        int rc = sqlite3_exec(db, ss.str().c_str(), NULL, NULL, &errmsg);
        if (rc){
            std::cerr << "Insert table failed, rc:" << rc << ",errmsg:" << errmsg << std::endl;
            sqlite3_free(errmsg);
            return rc;
        }
    }

    return 0;
}

std::string DbUtils::gen_so_version() {
    auto ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buffer[16] = {0};
    std::strftime(buffer, sizeof(buffer),"%Y%m%d%H%M",std::localtime(&ts));
    return std::string(buffer);
}

int callback_version(void *data, int argc, char **argv, char **col_name) {
    if (argc != 1) {
        return 0;
    }

    DbUtils::version = std::atol(argv[0]);
    return 0;
}

int callback_file_md5s(void *data, int argc, char **argv, char **col_name) {
    if (argc < 3) {
        std::cerr << "Invalid field number";
        return 0;
    }
    if (argv[1] && argv[2]){
        DbUtils::file_md5s[argv[1]] = argv[2];
        return 0;
    }
    std::cerr << "Error: ["<< col_name[1] << ":" << argv[1] << "," << col_name[2] <<":" << argv[2] << "]";
    return 0;
}

int fetch_last_file_md5s(DbUtils& dbUtils, std::map<std::string, std::string>& last_md5s){
    long long last_version = INVALID_VERSION;
    if (dbUtils.ok()){
        int rc = dbUtils.create_table();
        if (rc) {
            return rc;
        }


        rc = dbUtils.select_last_version(last_version);
        if (rc) {
            return rc;
        }


        rc = dbUtils.select_last_file_md5s(last_md5s);
        if (rc) {
            return rc;
        }

        return 0;
    }

    return -1;
}