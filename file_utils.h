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
 * ���ɴ˴α���so�İ汾��
 * �˰汾��Ϊ���Ӽ����ʱ���
 * @return �汾�ŵ��ַ������硰202402261447��
 */
std::string gen_so_version();

/***
 * �ݹ�����Ŀ¼������ļ�
 * @param path Ҫ������Ŀ¼
 * @param file_name_prefix Ҫ�������ļ�·��
 * @param files �����ļ��Ľ����
 * @return �ɹ�:0; ʧ��:��0
 */
int fetch_dir_files(const std::string& path, const std::string& file_name_prefix, std::set<std::string>& files);

/***
 * �����ļ���md5ֵ
 * @param path �ļ���·��
 * @param md5_str �ļ���md5ֵ
 * @return �ɹ�:0; ʧ��:��0
 */
int calc_file_md5(const std::string& path, char* md5_str);

/***
 * �ݹ����һ��Ŀ¼�����ļ���md5ֵ
 * @param dir
 * @param md5s
 * @return
 */
int calc_dir_file_md5s(const std::string& dir, std::map<std::string, std::string>& md5s);

/***
 * �ҳ�����һ����������֮ǰ��ͬ��so
 * @param dbUtils
 * @param cmdOpts
 * @param diff_file
 * @return
 */
int fetch_new_diff_so(DbUtils& dbUtils, const CmdOpts& cmdOpts, std::set<std::string>& diff_file);

#endif //MD5UTILS_FILE_UTILS_H
