#include "api_root_delete_table.h"
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api_common.h"
#include "json/json.h"
#include "http_conn.h"
#include <sys/time.h>

int encodeCountJson2(int ret, string &str_json) {
    Json::Value root;
    root["code"] = ret;
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

/*
{
    "title":xxx
}
*/
int decodeRootDeleteTablelistJson(string &str_json,string &table_name){
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

template <typename... Args>
std::string FormatString4(const std::string &format, Args... args) {
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) +
                1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                      buf.get() + size - 1); // We don't want the '\0' inside
}


//返回的字符串
/*
{
    "code":xxx
}
*/
int deleteRootTable(std::string &table_name, std::string str_json) {
    int ret = 0;
    std::string str_sql;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    Json::Value root;

    int survey_id = 0;
    str_sql = FormatString4("SELECT survey_id FROM Surveys WHERE title = '%s'", table_name.c_str());
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());
    while (result_set->Next()) {
        survey_id = result_set->GetInt("survey_id");

        // 先删除Responses表中对应记录
        str_sql = FormatString4("SELECT response_id FROM Responses WHERE survey_id = %d", survey_id);
        int response_id = 0;
        CResultSet *response_set = db_conn->ExecuteQuery(str_sql.c_str());
        while (response_set->Next()) {
            response_id = response_set->GetInt("response_id");
            str_sql = FormatString4("DELETE FROM Responses WHERE response_id = %d", response_id);
            if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                encodeCountJson2(1, str_json);
                return -1;
            }
        }
        delete response_set;

        // 接着删除Options表中对应记录
        str_sql = FormatString4("SELECT question_id FROM Questions WHERE survey_id = %d", survey_id);
        int question_id = 0;
        CResultSet *question_set = db_conn->ExecuteQuery(str_sql.c_str());
        while (question_set->Next()) {
            question_id = question_set->GetInt("question_id");
            str_sql = FormatString4("SELECT option_id FROM Options WHERE question_id = %d", question_id);
            int option_id = 0;
            CResultSet *option_set = db_conn->ExecuteQuery(str_sql.c_str());
            while (option_set->Next()) {
                option_id = option_set->GetInt("option_id");
                str_sql = FormatString4("DELETE FROM Options WHERE option_id = %d", option_id);
                if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                    encodeCountJson2(1, str_json);
                    return -1;
                }
            }
            delete option_set;
        }
        delete question_set;

        // 然后删除Questions表中对应记录
        str_sql = FormatString4("SELECT question_id FROM Questions WHERE survey_id = %d", survey_id);
        question_id = 0;
        CResultSet *question_set_again = db_conn->ExecuteQuery(str_sql.c_str());
        while (question_set_again->Next()) {
            question_id = question_set_again->GetInt("question_id");
            str_sql = FormatString4("DELETE FROM Questions WHERE question_id = %d", question_id);
            if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                encodeCountJson2(1, str_json);
                return -1;
            }
        }
        delete question_set_again;

        // 最后删除Surveys表中对应记录
        str_sql = FormatString4("DELETE FROM Surveys WHERE survey_id = %d", survey_id);
        if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
            encodeCountJson2(1, str_json);
            return -1;
        }
    }
    delete result_set;

    // 返回 JSON 数据
    encodeCountJson2(0, str_json);
    return 0;
}


int ApiRootTableDelete(string &url,string &post_data,string &str_json){
    string table_name;
    UNUSED(url);

    // 判断数据是否为空
    if (post_data.empty()) {
        return -1;
    }
    // 解析json
    if (decodeRootDeleteTablelistJson(post_data, table_name) < 0) {
        encodeCountJson2(1,str_json);
        return -1;
    }

    //在数据库删除这个问卷
    if (deleteRootTable(table_name,str_json) < 0) {
        encodeCountJson2(1,str_json);
        return -1;
    }

    encodeCountJson2(0,str_json);
    return 0;
}