#ifndef _API_COMMON_H_
#define _API_COMMON_H_

#include"db_pool.h"
#include"tc_common.h"
#include"dlog.h"
#include"json/json.h"
#include<string>

#define FILE_PUBLIC_ZSET "TABLE_PUBLIC_ZSET"
#define TABLE_NAME_HASH "TABLE_NAME_HASH"
#define TABLE_USER_COUNT "TABLE_USER_COUNT"

#define HTTP_RESPONSE_HTML_MAX 4096
#define HTTP_RESPONSE_HTML                                                     \
    "HTTP/1.1 200 OK\r\n"                                                      \
    "Connection:close\r\n"                                                     \
    "Content-Length:%d\r\n"                                                    \
    "Content-Type:application/json;charset=utf-8\r\n\r\n%s"

// 开启多线程
#define API_MYFILES_MUTIL_THREAD  1
#define API_LOGIN_MUTIL_THREAD  1

extern string s_dfs_path_client;
extern string s_storage_web_server_ip;
extern string s_storage_web_server_port;
extern string s_shorturl_server_address;
extern string s_shorturl_server_access_token;
using std::string;
static int ApiInit();

static int CacheSetCount(void*, string key, int64_t count);
static int CacheGetCount(void*, string key, int64_t &count) ;
static int CacheIncrCount(void*, string key);
static int CacheDecrCount(void*, string key);

// 声明外部函数和变量
extern int kvstore_sockfd;
extern bool connect_to_kvstore();
extern int send_kvstore_command(const char* command, char* response, size_t response_size);
extern void close_kvstore_connection();

int DBGetUserTableCountByUsername(CDBConn *db_conn, string user_name,
                                  int &count);
int DBGetAllTablesCount(CDBConn *db_conn,int &count);

#endif  