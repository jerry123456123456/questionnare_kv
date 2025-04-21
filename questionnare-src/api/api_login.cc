#include "api_login.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "http_conn.h"

// 声明外部函数
extern int send_kvstore_command(const char* command, char* response, size_t response_size);

#define LOGIN_RET_OK 0   // 成功
#define LOGIN_RET_FAIL 1 // 失败

/*
{
    "user":xxx,
    "pwd":xxx,
}
*/
// 解析登录信息
int decodeLoginJson(const std::string &str_json, string &user_name,
                    string &pwd) {
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res) {
        LogError("parse reg json failed ");
        return -1;
    }
    // 用户名
    if (root["user"].isNull()) {
        LogError("user null");
        return -1;
    }
    user_name = root["user"].asString();

    //密码
    if (root["pwd"].isNull()) {
        LogError("pwd null");
        return -1;
    }
    pwd = root["pwd"].asString();

    return 0;
}

/*
{
    "code":xxx,
    "token":xxx,
}
*/
// 封装登录结果的json
int encodeLoginJson(int ret, string &token, string &str_json) {
    Json::Value root;
    root["code"] = ret;
    if (ret == 0) {
        root["token"] = token; // 正常返回的时候才写入token
    }
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

template <typename... Args>
std::string formatString1(const std::string &format, Args... args) {
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) +
                1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                       buf.get() + size - 1); // We don't want the '\0' inside
}

/* -------------------------------------------*/
/**
 * @brief  判断用户登陆情况
 *
 * @param username 		用户名
 * @param pwd 		密码
 *
 * @returns
 *      成功: 0
 *      失败：-1
 */
/* -------------------------------------------*/
int verifyUserPassword(string &user_name,string &pwd){
    int ret = 0;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);
    
    // 先查看用户是否存在
    string strSql;

    strSql =
        formatString1("select password from Users where user_name='%s'",
                      user_name.c_str());

    CResultSet *result_set = db_conn->ExecuteQuery(strSql.c_str());
    if (result_set && result_set->Next()) {
        // 存在在返回
        string password = result_set->GetString("password");
        LogInfo("mysql-pwd: {}, user-pwd: {}", password, pwd);
        if (result_set->GetString("password") == pwd)
            ret = 0;
        else
            ret = -1;
    } else { // 如果不存在则注册
        ret = -1;
    }

    delete result_set;

    return ret;
}

std::string removeSpecialChar(const std::string& str);

// 与kvstore交互的RSET函数（严格校验命令格式，避免参数解析错误）
static int setTokenInKvstore(const std::string& key, int expire_time, const std::string& value) {
    // 1. 严格按协议格式生成命令（RSET#key#value#），确保参数用 # 正确分隔
    char command[1024];
    // 2. 检查命令长度，防止缓冲区溢出
    int command_length = snprintf(command, sizeof(command), "RSET#%s%s\n", key.c_str(), value.c_str());
    if (command_length < 0 || command_length >= sizeof(command)) {
        LogError("【setTokenInKvstore】命令缓冲区溢出！Key: %s, Value: %s", key.c_str(), value.c_str());
        return -1;
    }

    char response[1024];
    int ret = send_kvstore_command(command, response, sizeof(response));

    // 3. 处理响应中的换行符，确保正确解析返回值
    // if (response[0] != '\0') {
    //     char* newline_pos = strchr(response, '\n');
    //     if (newline_pos) *newline_pos = '\0'; // 移除换行符，避免影响字符串匹配
    // }

    std::string trimmedResponse = removeSpecialChar(std::string(response));

    // 4. 添加调试日志，打印关键参数和响应内容
    LogError("【setTokenInKvstore】发送命令: %s，响应: %s", command, trimmedResponse.c_str());

    // 5. 校验响应是否符合预期（例如包含"OK"）
    if (ret == 0 && trimmedResponse == "OK\r\n") {
        return 0;
    } else {
        LogError("【setTokenInKvstore】操作失败！Key: %s, Value: %s，错误响应: %s", 
                 key.c_str(), value.c_str(), trimmedResponse.c_str());
        return -1;
    }
}

/* -------------------------------------------*/
/**
 * @brief  生成token字符串, 保存kvstore数据库
 *
 * @param username 		用户名
 * @param token     生成的token字符串
 *
 * @returns
 *      成功: 0
 *      失败：-1
 */
/* -------------------------------------------*/
extern char SPECIAL_CHAR = '#';

// 为字符串添加特殊字符
extern std::string addSpecialChar(const std::string& str);

// 去除字符串末尾的特殊字符
extern std::string removeSpecialChar(const std::string& str);

int setToken(std::string &user_name, std::string &token) {
    int ret = 0;
    token = RandomString(32); // 随机32个字母

    std::string keyWithSpecial = addSpecialChar(user_name);
    std::string valueWithSpecial = addSpecialChar(token);

    if (setTokenInKvstore(keyWithSpecial, 86400, valueWithSpecial) != 0) {
        ret = -1;
    }

    return ret;
}

// 与kvstore交互的CacheSetCount函数（严格处理命令格式与响应解析）
extern int CacheSetCount(const std::string key, int64_t count);

//同步个人用户的表到kvstore
int loadMyTablesCount(string &user_name){
    int64_t kvstore_table_count = 0;
    int mysql_table_count = 0;

    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    if(DBGetUserTableCountByUsername(db_conn,user_name,mysql_table_count)<0){
        LogError("DBGetUserTablesCountByUsername failed");
        return -1;
    }

    kvstore_table_count = (int64_t)mysql_table_count;
    //将从mysql获取到的用户表的数量set到kvstore里
    if(CacheSetCount(TABLE_USER_COUNT + user_name,kvstore_table_count)<0){
         LogError("DBGetUserTablesCountByUsername failed");
        return -1;
    }
    LogInfo("TABLE_USER_COUNT: {}", kvstore_table_count);

    return 0;
}

#if API_LOGIN_MUTIL_THREAD
int ApiUserLogin(u_int32_t conn_uuid, std::string &url, std::string &post_data)
{
    UNUSED(url);
    string user_name;
    string pwd;
    string token;
    string str_json;
    // 判断数据是否为空
    if (post_data.empty()) {
        encodeLoginJson(1, token, str_json);
        goto END;
    }
    // 解析json
    if (decodeLoginJson(post_data, user_name, pwd) < 0) {
        LogError("decodeRegisterJson failed");
        encodeLoginJson(1, token, str_json);
        goto END;
    }

    // 验证账号和密码是否匹配
    if (verifyUserPassword(user_name, pwd) < 0) {
        LogError("verifyUserPassword failed");
        encodeLoginJson(1, token, str_json);
        goto END;
    }

    // 生成token
    if (setToken(user_name, token) < 0) {
        LogError("setToken failed");
        encodeLoginJson(1, token, str_json);
        goto END;
    }

    // 加载 我的表格数量 
    if (loadMyTablesCount(user_name) < 0) {
        LogError("loadMyTablesCount failed");
        encodeLoginJson(1, token, str_json);
        goto END;
    }
    encodeLoginJson(0, token, str_json);
END:
    char *str_content = new char[HTTP_RESPONSE_HTML_MAX];
    uint32_t ulen = str_json.length();
    snprintf(str_content, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, ulen,
             str_json.c_str());
    str_json = str_content;
    CHttpConn::AddResponseData(conn_uuid, str_json);
    delete[] str_content;
    
    return 0;
}
#else
int ApiUserLogin(string &url, string &post_data, string &str_json) {
    UNUSED(url);
    string user_name;
    string pwd;
    string token;

    // 判断数据是否为空
    if (post_data.empty()) {
        return -1;
    }
    // 解析json
    if (decodeLoginJson(post_data, user_name, pwd) < 0) {
        LogError("decodeRegisterJson failed");
        encodeLoginJson(1, token, str_json);
        return -1;
    }

    // 验证账号和密码是否匹配
    if (verifyUserPassword(user_name, pwd) < 0) {
        LogError("verifyUserPassword failed");
        encodeLoginJson(1, token, str_json);
        return -1;
    }

    // 生成token
    if (setToken(user_name, token) < 0) {
        LogError("setToken failed");
        encodeLoginJson(1, token, str_json);
        return -1;
    }

    // 加载 我的文件数量  我的分享图片数量

    if (loadMyTablesCount(user_name) < 0) {
        LogError("loadMyTablesCount failed");
        encodeLoginJson(1, token, str_json);
        return -1;
    }
    // 注册账号
    // ret = registerUser(user_name, nick_name, pwd, phone, email);
    encodeLoginJson(0, token, str_json);
    return 0;
}
#endif