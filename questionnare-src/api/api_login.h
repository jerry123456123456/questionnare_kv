#ifndef _API_LOGIN_H_
#define _API_LOGIN_H_
#include "api_common.h"

#define FILE_PUBLIC_ZSET "TABLE_PUBLIC_ZSET"
#define TABLE_NAME_HASH "TABLE_NAME_HASH"
#define TABLE_USER_COUNT "TABLE_USER_COUNT"

#if API_LOGIN_MUTIL_THREAD  // 该把这个宏定义放到cmakelists.txt才对
int ApiUserLogin(u_int32_t conn_uuid, std::string &url, std::string &post_data);
#else
int ApiUserLogin(string &url, string &post_data, string &str_json);
#endif
#endif // ! _API_LOGIN_H_