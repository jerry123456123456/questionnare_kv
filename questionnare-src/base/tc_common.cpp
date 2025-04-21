#include "tc_common.h"

#include <cstdio>
#include <ctype.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 声明外部函数
extern int send_kvstore_command(const char* command, char* response, size_t response_size);

//去掉一个字符串两边的空白字符
int TrimSpace(char *inbuf) {
    int i = 0;
    int j = strlen(inbuf) - 1;

    char *str = inbuf;

    int count = 0;

    if (str == NULL) {
        return -1;
    }

    while (isspace(str[i]) && str[i] != '\0') {
        i++;
    }

    while (isspace(str[j]) && j > i) {
        j--;
    }

    count = j - i + 1;

    strncpy(inbuf, str + i, count);

    inbuf[count] = '\0';

    return 0;
}

//传入一个key,得到相应的value,第一个参数是字符串
int QueryParseKeyValue(const char *query, const char *key, char *value,
                       int *value_len_p) {
    char *temp = NULL;
    char *end = NULL;
    int value_len = 0;

    //找到是否有key
    temp = (char *)strstr(query, key);
    if (temp == NULL) {
        return -1;
    }

    temp += strlen(key); //=
    temp++;              // value

    // get value
    end = temp;

    while ('\0' != *end && '#' != *end && '&' != *end) {
        end++;
    }

    value_len = end - temp;

    strncpy(value, temp, value_len);
    value[value_len] = '\0';

    if (value_len_p != NULL) {
        *value_len_p = value_len;
    }

    return 0;
}

//通过文件名file_name， 得到文件后缀字符串, 保存在suffix
//如果非法文件后缀,返回"null"
int GetFileSuffix(const char *file_name, char *suffix) {
    const char *p = file_name;
    int len = 0;
    const char *q = NULL;
    const char *k = NULL;

    if (p == NULL) {
        return -1;
    }

    q = p;

    // mike.doc.png
    //              ↑

    while (*q != '\0') {
        q++;
    }

    k = q;
    while (*k != '.' && k != p) {
        k--;
    }

    if (*k == '.') {
        k++;
        len = q - k;

        if (len != 0) {
            strncpy(suffix, k, len);
            suffix[len] = '\0';
        } else {
            strncpy(suffix, "null", 5);
        }
    } else {
        strncpy(suffix, "null", 5);
    }

    return 0;
}

// 与kvstore交互的Get函数
extern std::string GetFromKvstore(const std::string& key);

// 去除字符串末尾的空白字符
extern std::string trimRight(const std::string& str);

//验证登陆token，成功返回0，失败-1
int VerifyToken(std::string &user_name, std::string &token) {
    // 添加调试信息
    LogDebug("Verifying token for user: " + user_name + ", token: " + token);
    int ret = 0;
    string user_name1 = trimRight(user_name);

    std::string tmp_token = GetFromKvstore(user_name);

    // 去除末尾的空白字符
    tmp_token = trimRight(tmp_token);
    token = trimRight(token);

    // 打印去除空白字符后的字符串
    printf("Trimmed tmp_token: %s\n", tmp_token.c_str());
    printf("Trimmed token: %s\n", token.c_str());

    if (tmp_token == token) {
        LogDebug("Token verification succeeded for user: " + user_name);
        ret = 0;
    } else {
        LogDebug("Token verification failed for user: " + user_name);
        ret = -1;
    }
    return ret;
}

//优化了源码中的伪随机数问题
std::string RandomString(const int len) {
    std::string str;
    char c;
    int idx;
    
    srand(time(0)); // 设置种子值为当前时间

    for (idx = 0; idx < len; idx++) {
        c = 'a' + rand() % 26;
        str.push_back(c);
    }
    
    return str;
}

//这段代码定义了一个函数 `FormatString`，用于格式化字符串。函数接受一个格式化字符串 `format` 和可变数量的参数 `args`，然后使用这些参数填充格式化字符串并返回最终的格式化结果
template<typename... Args>
std::string FormatString(const std::string &format, Args... args) {
    //这里将s和maxlen都设为0，意义是单纯计算长度，snprintf接收可变参数列表
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
    //这个操作也很精彩，自动释放
    std::unique_ptr<char[]> buf(new char[size]);
    //这里才是拷贝
    //这里buf.get()是为了获得智能指针管理下的原始指针
    std::snprintf(buf.get(), size, format.c_str(), args...);
    //使用string类的构造函数，排除结尾空格
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

//处理数据库查询结果，结果集保存在count，如果要读取count值则设置为0，如果设置为-1则不读取
//返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
int GetResultOneCount(CDBConn *db_conn, char *sql_cmd, int &count) {
    int ret = -1;
    CResultSet *result_set = db_conn->ExecuteQuery(sql_cmd);

    if (!result_set) {
        ret = -1;
    }

    if (count == 0) {
        // 读取
        if (result_set->Next()) {
            ret = 0;
            // 存在在返回
            count = result_set->GetInt("count");
            LogDebug("count: {}", count);
        } else {
            ret = 1; // 没有记录
        }
    } else {
        if (result_set->Next()) {
            ret = 2;
        } else {
            ret = 1; // 没有记录
        }
    }

    delete result_set;

    return ret;
}

//查看数据库中是否有这条记录
int CheckwhetherHaveRecord(CDBConn *db_conn, char *sql_cmd) {
    int ret = -1;
    CResultSet *result_set = db_conn->ExecuteQuery(sql_cmd);

    if (!result_set) {
        ret = -1;
    } else if (result_set && result_set->Next()) {
        ret = 1;
    } else {
        ret = 0;
    }

    delete result_set;

    return ret;
}

//这个函数的作用是执行给定的 SQL 查询语句，获取数据库中的某一条记录的状态，并将状态值存储在 `shared_status` 参数中
int GetResultOneStatus(CDBConn *db_conn, char *sql_cmd, int &shared_status) {
    int ret = 0;
    CResultSet *result_set = db_conn->ExecuteQuery(sql_cmd);

    if (!result_set) {
        LogError("result_set is NULL");
        ret = -1;
    }

    if (result_set->Next()) {
        ret = 0;
        // 存在在返回
        shared_status = result_set->GetInt("shared_status");
        LogInfo("shared_status: {}", shared_status);
    } else {
        LogError("result_set->Next() is NULL");
        ret = -1;
    }

    delete result_set;

    return ret;
}