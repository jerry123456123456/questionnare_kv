#include "api_root_tables.h"
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api_common.h"
#include "json/json.h"
#include "http_conn.h"
#include <sys/time.h>

//root用户不用验证token了

int encodeCountJson1(int ret, int total, string &str_json) {
    Json::Value root;
    root["code"] = ret;
    if (ret == 0) {
        root["total"] = total; // 正常返回的时候才写入token
    }
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

//解析json包，处理参数cmd=normal的情况
/*
{
    "title":xxx
}
*/
int decodeRootTablelistJson(string &str_json,string &table_name){
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res) {
        LogError("parse reg json failed ");
        return -1;
    }
    int ret = 0;

    if(root["title"].isNull()){
        LogError("table_name null\n");
        return -1;
    }
    table_name = root["title"].asString();

    return ret;
}


//获取所有创建的调查问卷个数
int getTablesCount(CDBConn *db_conn,int &count){
    int ret = 0;
    int64_t table_count = 0;

    if(DBGetUserTableCountByUsername(db_conn,"root",count)){
        LogError("DBGetRootTablesCountByRootname failed");
        return -1;
    }

    return 0;
}

// url="http://192.168.186.138/api/root/tables?cmd=count"
template <typename... Args>
std::string FormatString3(const std::string &format, Args... args) {
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) +
                1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                      buf.get() + size - 1); // We don't want the '\0' inside
}

//回发的格式
/*
{
    "code":xxx,
    "total":xxx,
    {
        "table_name":xxx
        "deadline":xxx,
    }
    {
        "table_name":xxx
        "deadline":xxx,
    }
    ...
}
*/
//// url="http://192.168.186.138/api/root/tables?cmd=count"
int getRootTableList(std::string &str_json){
    LogInfo("getRootTableList info");
    int ret = 0;
    int total = 0;
    std::string str_sql;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    ret = getTablesCount(db_conn, total);
    if (ret < 0) {
        LogError("getRootFilesCount failed");
        return -1;
    } else {  // root用户还没有发布任何表格
        if (total == 0) {
            Json::Value root;
            root["code"] = 0;
            root["total"] = 0;
            Json::FastWriter writer;
            str_json = writer.write(root);
            LogWarn("getRootTablesCount = 0");
            return 0;
        }
    }

    //到这里就意味着创建了表格，所有的表格在root里都有，但是root用户不负责填写，所以其他表没有root信息，除了Surveys

    //root用户id是1
    // 从Surveys表中查询所有这个user_id对应的title和deadline
    std::vector<std::string> table_titles;
    std::vector<std::string> table_deadlines;
    str_sql = FormatString3("select title, deadline from Surveys where user_id = 1");
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());
    while (result_set && result_set->Next()) {
        table_titles.push_back(result_set->GetString("title"));
        table_deadlines.push_back(result_set->GetString("deadline"));
    }
    delete result_set;

    // 组织成指定的JSON格式
    Json::Value root;
    root["code"] = 0;
    root["total"] = static_cast<int>(table_titles.size());
    Json::Value tables;
    for (size_t i = 0; i < table_titles.size(); ++i) {
        Json::Value table;
        table["table_name"] = table_titles[i];
        table["deadline"] = table_deadlines[i];
        tables.append(table);
    }
    root["tables"] = tables;

    // 使用Json::FastWriter将JSON对象转换为字符串
    Json::FastWriter writer;
    str_json = writer.write(root);

    return 0;
}

// url="http://192.168.186.138/api/root/tables?cmd=normal"
/*
当root管理员点击特定的表格后，接下来就要获取普通用户填写的结果了
*/
/*
{
    "code": 0,
    "questions": [
        {
            "options": [
                {
                    "option_count": 0,
                    "option_text": "选项A（问题2）"
                },
                {
                    "option_count": 0,
                    "option_text": "选项B（问题2）"
                },
                {
                    "option_count": 2,
                    "option_text": "选项C（问题2）"
                }
            ],
            "question_text": "问题1，这是一个单选题",
            "question_type": "single_choice"
        },
        {
            "options": [
                {
                    "option_count": 2,
                    "option_text": "选项A（问题2）"
                },
                {
                    "option_count": 2,
                    "option_text": "选项B（问题2）"
                },
                {
                    "option_count": 2,
                    "option_text": "选项C（问题2）"
                }
            ],
            "question_text": "问题2，这是一个多选题",
            "question_type": "multiple_choice"
        }
    ],
    "survey_title": "通用问卷标题",
}
*/
int getRootTable(std::string &table_name, std::string &str_json) {
    LogInfo("getRootTable info");
    int ret = 0;
    int total = 0;
    std::string str_sql;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    // 先根据 title 在 Surveys 表中查询 survey_id 和 user_id
    int survey_id = 0;
    str_sql = FormatString3("SELECT survey_id FROM Surveys WHERE title = '%s' AND user_id = 1", table_name.c_str());
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set && result_set->Next()) {
        survey_id = result_set->GetInt("survey_id");
    } else {
        return -1;  // 没有找到相应的 survey_id
    }
    delete result_set;

    Json::Value root;
    root["code"] = 0;
    root["survey_title"] = table_name;

    Json::Value questions(Json::arrayValue);  // 存储所有问题

    // 查询 Questions 表，获取该 survey_id 下的所有问题
    str_sql = FormatString3("SELECT question_id, question_text, question_type FROM Questions WHERE survey_id = %d", survey_id);
    result_set = db_conn->ExecuteQuery(str_sql.c_str());
    while (result_set->Next()) {
        int question_id = result_set->GetInt("question_id");
        std::string question_text = result_set->GetString("question_text");
        std::string question_type = result_set->GetString("question_type");

        Json::Value question;
        question["question_text"] = question_text;
        question["question_type"] = question_type;

        if (question_type!= "fill_in_blank") {  // 选择题
            // 查询选项
            str_sql = FormatString3("SELECT option_id, option_text FROM Options WHERE question_id = %d", question_id);
            CResultSet *options_result = db_conn->ExecuteQuery(str_sql.c_str());
            Json::Value options(Json::arrayValue);
            std::map<int, int> option_count_map;  // 用于统计每个选项的选中数量

            // 初始化选项计数
            while (options_result->Next()) {
                std::string option_text = options_result->GetString("option_text");
                int option_id = options_result->GetInt("option_id");
                Json::Value option;
                option["option_text"] = option_text;
                options.append(option);

                // 初始化选项计数
                option_count_map[option_id] = 0;

                // 查询 Responses 表，统计选项的选中数量
                str_sql = FormatString3("SELECT option_id FROM Responses WHERE answer = '%s'", option_text.c_str());
                CResultSet *responses_result = db_conn->ExecuteQuery(str_sql.c_str());
                while (responses_result->Next()) {
                    // 这是用户的
                    int option_id = responses_result->GetInt("option_id");
                    str_sql = FormatString3("SELECT root_option_id FROM Options WHERE option_id = '%d'", option_id);
                    CResultSet *responses_result1 = db_conn->ExecuteQuery(str_sql.c_str());
                    // 只会有一条结果
                    if (responses_result1->Next()) {
                        int root_option_id = responses_result1->GetInt("root_option_id");
                        // 只统计选项计数
                        if (option_count_map.find(root_option_id)!= option_count_map.end()) {
                            option_count_map[root_option_id]++;
                        }
                    }
                    delete responses_result1;
                }
                delete responses_result;
            }
            delete options_result;

            // 更新选项的计数信息
            for (auto& option : options) {
                std::string option_text = option["option_text"].asString();
                int option_id = 0;  // 获取 option_id
                str_sql = FormatString3("SELECT option_id FROM Options WHERE option_text = '%s' AND question_id = %d", option_text.c_str(), question_id);
                CResultSet *id_result = db_conn->ExecuteQuery(str_sql.c_str());
                if (id_result && id_result->Next()) {
                    option_id = id_result->GetInt("option_id");
                }
                option["option_count"] = option_count_map[option_id];
                delete id_result;
            }
            question["options"] = options;

        } else {  // 填空题
            // 初始化 answers 为一个空数组，确保可以累积添加答案
            question["answers"] = Json::Value(Json::arrayValue);
            str_sql = FormatString3("SELECT question_id FROM Questions WHERE root_question_id = %d", question_id);
            CResultSet *root_question_result = db_conn->ExecuteQuery(str_sql.c_str());
            while (root_question_result && root_question_result->Next()) {
                question_id = root_question_result->GetInt("question_id");

                // 查询所有根问题的回答
                str_sql = FormatString3("SELECT answer FROM Responses WHERE question_id = %d", question_id);
                CResultSet *fill_in_result = db_conn->ExecuteQuery(str_sql.c_str());
                while (fill_in_result->Next()) {
                    std::string answer = fill_in_result->GetString("answer");
                    Json::Value ans;
                    ans["answer_text"] = answer;
                    // 直接往 question["answers"] 数组中添加答案元素
                    question["answers"].append(ans);
                }
                delete fill_in_result;
            }
            delete root_question_result;
        }

        questions.append(question);  // 添加到问题列表
    }
    delete result_set;

    root["questions"] = questions;  // 将所有问题添加到 root 中

    // 返回 JSON 数据
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}


int ApiRootTables(string &url,string &post_data,string &str_json){
    UNUSED(url);
    char cmd[20];
    int ret = 0;
    int count = 0;
    string table_name;

    //解析命令 解析url获取自定义参数
    QueryParseKeyValue(url.c_str(), "cmd", cmd, NULL);
    LogInfo("url: {}, cmd: {} ",url, cmd);

    if(strcmp(cmd,"count") == 0){   //第一次返回表格数量和名字
        if(getRootTableList(str_json)<0){
            ret = -1;
            return -1;   
        }
    }else{
        if(strcmp(cmd, "normal") != 0){
            LogError("unknow cmd: {}", cmd);
            encodeCountJson1(1, 0, str_json);
            return -1;
        }
        ret = decodeRootTablelistJson(post_data,table_name);

        if(ret == 0){
            if(getRootTable(table_name,str_json)){
                LogError("getRootTable failed");
                encodeCountJson1(1, 0, str_json);
                return -1;
            }
        }else{
            LogError("decodeTableslistJson failed");
            encodeCountJson1(1, 0, str_json);
            return -1;
        }
    }
    return 0;
}