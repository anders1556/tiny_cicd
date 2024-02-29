//
// Created by burning on 2024/2/26.
//

#ifndef MD5UTILS_FILE_UTILS_H
#define MD5UTILS_FILE_UTILS_H

#include <string>
#include <set>
#include <map>
#include "cicd_cmd.h"
#include "db_utils.h"

#define MD5_SIZE		16
#define MD5_STR_LEN		(MD5_SIZE * 2)

/***
 * 生成此次编译so的版本号
 * 此版本号为分钟级别的时间戳
 * @return 版本号的字符串，如“202402261447”
 */
std::string gen_so_version();

/***
 * 递归搜索目录下面的文件
 * @param path 要搜索的目录
 * @param file_name_prefix 要附带的文件路径
 * @param files 所有文件的结果集
 * @return 成功:0; 失败:非0
 */
int fetch_dir_files(const std::string& path, const std::string& file_name_prefix, std::set<std::string>& files);

/***
 * 计算文件的md5值
 * @param path 文件的路径
 * @param md5_str 文件的md5值
 * @return 成功:0; 失败:非0
 */
int calc_file_md5(const std::string& path, char* md5_str);

/***
 * 递归计算一个目录下面文件的md5值
 * @param dir
 * @param md5s
 * @return
 */
int calc_dir_file_md5s(const std::string& dir, std::map<std::string, std::string>& md5s);

/***
 * 找出最新一次生成且与之前不同的so
 * @param dbUtils
 * @param cmdOpts
 * @param diff_file
 * @return
 */
int fetch_new_diff_so(DbUtils& dbUtils, const CmdOpts& cmdOpts, std::set<std::string>& diff_file);

#endif //MD5UTILS_FILE_UTILS_H
