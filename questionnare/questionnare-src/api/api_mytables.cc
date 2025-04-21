#include "api_mytables.h"
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api_common.h"
#include "json/json.h"
#include "http_conn.h"
#include <sys/time.h>

// 声明外部函数
extern int send_kvstore_command(const char* command, char* response, size_t response_size);

//解析的json包, 登陆token
/*
{
    "user":xxx,
    "token":xxx,
}
*/
int decodeCountJson(string &str_json, string &user_name, string &token) {
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res) {
        LogError("parse reg json failed ");
        return -1;
    }
    int ret = 0;

    // 用户名
    if (root["user"].isNull()) {
        LogError("user null\n");
        return -1;
    }
    user_name = root["user"].asString();

    //密码
    if (root["token"].isNull()) {
        LogError("token null\n");
        return -1;
    }
    token = root["token"].asString();

    return ret;
}

int encodeCountJson(int ret, int total, string &str_json) {
    Json::Value root;
    root["code"] = ret;
    if (ret == 0) {
        root["total"] = total; // 正常返回的时候才写入token
    }
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

// 与kvstore交互的CacheGetCount函数
static int CacheGetCount(const std::string& key, int64_t &count) {
    count = 0;
    char command[1024];
    // 检查命令长度，防止缓冲区溢出
    int command_length = snprintf(command, sizeof(command), "RGET %s\n", key.c_str());
    if (command_length < 0 || command_length >= sizeof(command)) {
        LogError("Command buffer overflow in CacheGetCount for key: %s", key.c_str());
        return -1;
    }
    char response[1024];
    int ret = send_kvstore_command(command, response, sizeof(response));
    if (ret == 0 && response[0] != '\0') {
        // 处理响应中的换行符
        char* newline = strchr(response, '\n');
        if (newline) {
            *newline = '\0';
        }
        count = atoll(response);
        return 0;
    } else {
        LogError("Failed to get count in CacheGetCount for key: %s. Response: %s", key.c_str(), response);
        return -1;
    }
}

// 与kvstore交互的CacheSetCount函数
static int CacheSetCount(const std::string& key, int64_t count) {
    char command[1024];
    // 检查命令长度，防止缓冲区溢出
    int command_length = snprintf(command, sizeof(command), "RSET %s %lld\n", key.c_str(), count);
    if (command_length < 0 || command_length >= sizeof(command)) {
        LogError("Command buffer overflow in CacheSetCount for key: %s, count: %lld", key.c_str(), count);
        return -1;
    }
    char response[1024];
    int ret = send_kvstore_command(command, response, sizeof(response));
    // 处理响应中的换行符
    char* newline = strchr(response, '\n');
    if (newline) {
        *newline = '\0';
    }
    if (ret == 0 && strstr(response, "OK") != nullptr) {
        return 0;
    } else {
        LogError("Failed to set count in CacheSetCount for key: %s, count: %lld. Response: %s", key.c_str(), count, response);
        return -1;
    }
}  

//获取每个用户可以填的表的数量
int getUserTablesCount(CDBConn *db_conn, string &user_name, int &count) {
    int ret = 0;
    int64_t table_count = 0;

    //先查看用户是否存在
    // 1. 先从kvstore里面获取，如果数量为0则从mysql查询确定是否为0
    if (CacheGetCount(TABLE_USER_COUNT + user_name, table_count) < 0) {
        LogWarn("CacheGetCount failed"); // 有可能是因为没有key，不要急于判断为错误
        table_count = 0;
        ret = -1;
    }

    if (table_count == 0) {  //没有key,或者值为0的时候从mysql重新加载在写入
        count = 0;
        if (DBGetUserTableCountByUsername(db_conn, user_name, count) < 0) {
            LogError("DBGetUserTablesCountByUsername failed");
            return -1;
        }
        table_count = (int64_t)count;
        if (CacheSetCount(TABLE_USER_COUNT + user_name, table_count) < 0) {
            LogError("CacheSetCount failed");
            return -1;
        }
    }

    count = table_count;

    return ret;
}

//解析json包,用于处理参数cmd=normal的情况
/*
{
    "user":xxx,
    "token":xxx,
    "title":xxx,
}
*/
int decodeTableslistJson(string &str_json, string &user_name, string &token, string &table_name) {
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res) {
        LogError("parse reg json failed ");
        return -1;
    }
    int ret = 0;

    // 用户名
    if (root["user"].isNull()) {
        LogError("user null\n");
        return -1;
    }
    user_name = root["user"].asString();

    //密码
    if (root["token"].isNull()) {
        LogError("token null\n");
        return -1;
    }
    token = root["token"].asString();

    if (root["title"].isNull()) {
        LogError("table_name null\n");
        return -1;
    }
    table_name = root["title"].asString();

    return ret;
}

template <typename... Args>
std::string FormatString(const std::string &format, Args... args) {
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) +
                1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                       buf.get() + size - 1); // We don't want the '\0' inside
}

//很显然，获取整个用户可以填的所有表
//回发的json格式
/*
{
    "code":xxx,
    "total":xxx,
    {
        "table_name":xxx
        "deadline":xxx,
        "is_filled":xxx,
    }
    {
        "table_name":xxx
        "deadline":xxx,
        "is_filled":xxx,
    }
    ...
}
*/
int getUserTableList(std::string &user_name, std::string &str_json) {
    LogInfo("getUserTableList info");
    int ret = 0;
    int total = 0;
    std::string str_sql;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    ret = getUserTablesCount(db_conn, user_name, total);
    if (ret < 0) {
        LogError("getUserFilesCount failed");
        return -1;
    } else {  // root用户还没有发布任何表格
        if (total == 0) {
            Json::Value root;
            root["code"] = 0;
            root["total"] = 0;
            Json::FastWriter writer;
            str_json = writer.write(root);
            LogWarn("getUserTablesCount = 0");
            return 0;
        }
    }

    // 到这里就意味着有表格可以填，紧接着从数据库中依次查找表格名字和截止日期，组织成json回发

    // 先从Users表中根据user_name查询user_id
    int user_id;
    str_sql = FormatString("select user_id from Users where user_name = '%s'", user_name.c_str());
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set && result_set->Next()) {
        user_id = result_set->GetInt("user_id");
    } else {
        LogError("未能根据用户名 {} 获取到user_id", user_name);
        delete result_set;
        return -1;
    }
    delete result_set;

    // 从Surveys表中查询所有这个user_id对应的title和deadline
    std::vector<std::string> table_titles;
    std::vector<std::string> table_deadlines;
    std::vector<int> table_filled;
    std::vector<int> table_dead;
    str_sql = FormatString("select title, deadline, is_filled, is_dead from Surveys where user_id = %d", user_id);
    result_set = db_conn->ExecuteQuery(str_sql.c_str());
    while (result_set && result_set->Next()) {
        table_titles.push_back(result_set->GetString("title"));
        table_deadlines.push_back(result_set->GetString("deadline"));
        table_filled.push_back(result_set->GetInt("is_filled"));
        table_dead.push_back(result_set->GetInt("is_dead"));
    }
    delete result_set;

    // 组织成指定的JSON格式
    Json::Value root;
    root["code"] = 0;
    root["total"] = static_cast<int>(table_titles.size());
    Json::Value tables;
    for (size_t i = 0; i < table_titles.size(); ++i) {
        if (table_dead[i] == 0) {   //如果没到截止时间
            Json::Value table;
            table["table_name"] = table_titles[i];
            table["deadline"] = table_deadlines[i];
            table["is_filled"] = table_filled[i];
            tables.append(table);
        }
    }
    root["tables"] = tables;

    // 使用Json::FastWriter将JSON对象转换为字符串
    Json::FastWriter writer;
    str_json = writer.write(root);

    return 0;
}

//而这个函数用来获取单个表
//返回的json格式
/*
{
    "code": 0, // 0表示成功获取调查问卷问题，可根据实际情况设置不同的状态码
    "survey_title": "调查问卷标题示例", // 调查问卷的标题
    "total_questions": 5, // 调查问卷中的题目总数
    "questions": [
        {
            "question_text": "你最喜欢的颜色是什么？",
            "question_type": "单选",
            "options": [
                {
                    "option_text": "红色"
                },
                {
                    "option_text": "蓝色"
                },
                {
                    "option_text": "绿色"
                }
            ]
        },
        {
            "question_text": "你平时的兴趣爱好有哪些？",
            "question_type": "多选",
            "options": [
                {
                    "option_text": "阅读"
                },
                {
                    "option_text": "运动"
                },
                {
                    "option_text": "绘画"
                },
                {
                    "option_text": "音乐"
                }
            ]
        },
        {
            "question_text": "请填写你所在的城市",
            "question_type": "填空",
            "options": [] // 填空题没有选项，这里为空数组
        },
        {
            "question_text": "你每天平均花多少时间在手机上？",
            "question_type": "填空",
            "options": []
        },
        {
            "question_text": "你是否喜欢旅行？（是/否）",
            "question_type": "单选",
            "options": [
                {
                    "option_text": "是"
                },
                {
                    "option_text": "否"
                }
            ]
        }
    ]
}
*/
int getUserTable(string &user_name, string &table_name, string &str_json) {
    LogInfo("getUserTable info");
    int ret = 0;
    int total = 0;
    string str_sql;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    // 先根据user_name在Users表中查user_id
    int user_id;
    str_sql = FormatString("select user_id from Users where user_name = '%s'", user_name.c_str());
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set && result_set->Next()) {
        user_id = result_set->GetInt("user_id");
    } else {
        LogError("未能根据用户名 {} 获取到user_id", user_name);
        delete result_set;
        return -1;
    }
    delete result_set;

    // 再从Surveys表中根据user_id和title(函数传递的参数是table_name)查survey_id
    int survey_id;
    str_sql = FormatString("select survey_id from Surveys where user_id = %d and title = '%s'", user_id, table_name.c_str());
    result_set = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set && result_set->Next()) {
        survey_id = result_set->GetInt("survey_id");
        printf("survey_id:%d\n", survey_id);
    } else {
        LogError("未能根据用户ID {} 和表名 {} 获取到survey_id", user_id, table_name);
        delete result_set;
        return -1;
    }
    delete result_set;

    // 从Questions表中查询所有这个survey_id的question_id，question_text和question_type
    Json::Value root;
    root["code"] = 0;
    root["survey_title"] = table_name;
    Json::Value questions;
    str_sql = FormatString("select question_id, question_text, question_type from Questions where survey_id = %d", survey_id);
    result_set = db_conn->ExecuteQuery(str_sql.c_str());
    while (result_set && result_set->Next()) {    //本身保证了向下遍历
        int question_id = result_set->GetInt("question_id");
        std::string question_text = result_set->GetString("question_text");
        printf("question_text:%s\n", question_text.c_str());
        std::string question_type = result_set->GetString("question_type");
        printf("question_type:%s\n", question_type.c_str());

        Json::Value question;
        //question["question_id"] = question_id;
        question["question_text"] = question_text;
        question["question_type"] = question_type;

        // 从Options表中查询每一个question_id对应的所有option_text(填空题没有）
        if (question_type != "填空") {
            Json::Value options;
            str_sql = FormatString("select option_text from Options where question_id = %d;", question_id);
            printf("sql_cmd:%s\n", str_sql.c_str());
            CResultSet *option_result_set = db_conn->ExecuteQuery(str_sql.c_str());
            while (option_result_set && option_result_set->Next()) {
                std::string option_text = option_result_set->GetString("option_text");

                Json::Value option;
                //option["option_id"] = option_result_set->GetInt("option_id");
                option["option_text"] = option_text;
                printf("option_text:%s\n", option_text.c_str());

                options.append(option);
            }
            delete option_result_set;

            question["options"] = options;
        }

        questions.append(question);
    }
    delete result_set;

    root["total_questions"] = questions.size();
    root["questions"] = questions;

    // 将JSON对象转换为字符串
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

int ApiMyTables(string &url, string &post_data, string &str_json) {
    //url这个参数没有用
    UNUSED(url);
    char cmd[20];
    string user_name;
    int ret = 0;
    int count = 0;
    string token;
    string table_name;

    //解析命令 解析url获取自定义参数
    QueryParseKeyValue(url.c_str(), "cmd", cmd, NULL);
    LogInfo("url: {}, cmd: {} ", url, cmd);

    if (strcmp(cmd, "count") == 0) {   //第一次返回表格数量和名字
        // 解析json
        if (decodeCountJson(post_data, user_name, token) < 0) {
            encodeCountJson(1, 0, str_json);
            LogError("decodeCountJson failed");
            return -1;
        }
        printf("%s\n", user_name.c_str());
        printf("%s\n", token.c_str());
        //验证登陆token，成功返回0，失败-1
        ret = VerifyToken(user_name, token); // util_cgi.h
        if (ret == 0) {
            //获取表格数量
            if (getUserTableList(user_name, str_json) < 0) {
                ret = -1;
                return -1;
            }
        } else {
            LogError("VerifyToken failed");
            encodeCountJson(1, 0, str_json);
            return -1;
        }
    } else {
        if (strcmp(cmd, "normal") != 0) {
            LogError("unknow cmd: {}", cmd);
            encodeCountJson(1, 0, str_json);
            return -1;
        }
        printf("%s\n", post_data.c_str());

        ret = decodeTableslistJson(post_data, user_name, token, table_name);
        LogInfo("user_name: {}, token:{}, table_name: {}", user_name, token, table_name);

        if (ret == 0) {
            //验证登陆token，成功返回0，失败-1
            ret = VerifyToken(user_name, token); // util_cgi.h
            if (ret == 0) {
                if (getUserTable(user_name, table_name, str_json) < 0) { //获取用户文件列表
                    LogError("getUserTableList failed");
                    encodeCountJson(1, 0, str_json);
                    return -1;
                }
            } else {
                LogError("VerifyToken failed");
                encodeCountJson(1, 0, str_json);
                return -1;
            }
        } else {
            LogError("decodeTableslistJson failed");
            encodeCountJson(1, 0, str_json);
            return -1;
        }
    }

    return 0;
}