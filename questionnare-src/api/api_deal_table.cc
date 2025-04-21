#include "api_deal_table.h"
#include "api_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sstream>

// 声明外部函数
extern int send_kvstore_command(const char* command, char* response, size_t response_size);

// int decodeDealTableJson(string &str_json,string &user_name,string &token,string &tablename){
//     bool res;
//     Json::Value root;
//     Json::Reader jsonReader;
//     res = jsonReader.parse(str_json, root);
//     if (!res) {
//         LogError("parse reg json failed ");
//         return -1;
//     }

//     if (root["user"].isNull()) {
//         LogError("user null");
//         return -1;
//     }
//     user_name = root["user"].asString();

//     if (root["token"].isNull()) {
//         LogError("token null");
//         return -1;
//     }
//     token = root["token"].asString();

//     if (root["tablename"].isNull()) {
//         LogError("tablename null");
//         return -1;
//     }
//     tablename = root["tablename"].asString();

//     return 0;
// }

int encodeDealtableJson(int ret, string &str_json) {
    Json::Value root;
    root["code"] = ret;
    Json::FastWriter writer;
    str_json = writer.write(root);

    LogInfo("str_json: {}", str_json);
    return 0;
}

// 与kvstore交互的CacheSetCount函数
extern int CacheSetCount(std::string key, int64_t count);

// 与kvstore交互的CacheGetCount函数
extern int CacheGetCount( std::string key, int64_t &count);

// 与kvstore交互的CacheIncrCount函数
extern int CacheIncrCount(const std::string key);

// 与kvstore交互的CacheDecrCount函数
extern int CacheDecrCount(const std::string key);

//这个函数的功能就是用户上传一条答案
int handleUpdateQuestion(string user, string questionname, string table_name, string answer) {
    CDBManager *db_manager = CDBManager::getInstance(); 
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};

    int user_id;
    int question_id;
    int survey_id;
    int option_id;
    int is_dead;

    string question_text;
    string question_type;
    
    // 获取 user_id
    sprintf(sql_cmd, "select user_id from Users where user_name = '%s'", user.c_str());
    CResultSet *result_set1 = db_conn->ExecuteQuery(sql_cmd);
    if (result_set1 && result_set1->Next()) {
        user_id = result_set1->GetInt("user_id");
        ret = 0;
    } else {
        ret = -1;
    }
    delete result_set1;

    if (ret != 0) return ret;

    // 获取 survey_id
    memset(sql_cmd, 0, sizeof(sql_cmd));
    sprintf(sql_cmd, "select survey_id,is_dead from Surveys where title = '%s' and user_id = %d", table_name.c_str(), user_id);
    CResultSet *result_set2 = db_conn->ExecuteQuery(sql_cmd);
    if (result_set2 && result_set2->Next()) {
        survey_id = result_set2->GetInt("survey_id");
        is_dead = result_set2->GetInt("is_dead");
        if(is_dead == 1){
            return -1;
        }
        ret = 0;
    } else {
        ret = -1;
    }
    delete result_set2;

    if (ret != 0) return ret;

    // 获取 question_id, question_text, question_type
    memset(sql_cmd, 0, sizeof(sql_cmd));
    sprintf(sql_cmd, "select question_id, question_text, question_type from Questions where question_text = '%s' and survey_id = %d", questionname.c_str(), survey_id);
    CResultSet *result_set3 = db_conn->ExecuteQuery(sql_cmd);
    if (result_set3 && result_set3->Next()) {
        question_id = result_set3->GetInt("question_id");
        question_text = result_set3->GetString("question_text");
        question_type = result_set3->GetString("question_type");
        ret = 0;
    } else {
        ret = -1;
    }
    delete result_set3;

    if (ret != 0) return ret;

    // 如果是选项题，获取 option_id
    if (question_type != "fill_in_blank") {
        sprintf(sql_cmd, "select option_id from Options where option_text = '%s' and question_id = %d", answer.c_str(), question_id);
        CResultSet *result_set4 = db_conn->ExecuteQuery(sql_cmd);
        if (result_set4 && result_set4->Next()) {
            option_id = result_set4->GetInt("option_id");
            ret = 0;
        } else {
            ret = -1;
        }
        delete result_set4;

        if (ret != 0) return ret;
    }

    // 执行插入或更新操作
    memset(sql_cmd, 0, sizeof(sql_cmd));

    if (question_type != "fill_in_blank") {
        // 查询是否已存在相同的记录
        sprintf(sql_cmd, "SELECT COUNT(*) FROM Responses WHERE question_id = %d AND user_id = %d AND survey_id = %d AND option_id = %d", question_id, user_id, survey_id, option_id);
        CResultSet *result_set5 = db_conn->ExecuteQuery(sql_cmd);
        int count = 0;
        if (result_set5 && result_set5->Next()) {
            count = result_set5->GetInt("COUNT(*)");
        }
        delete result_set5;

        if (count == 0) {
            // 如果不存在，插入记录
            sprintf(sql_cmd, "insert into Responses (question_id, user_id, answer, survey_id, option_id) values (%d, %d, '%s', %d, %d)", question_id, user_id, answer.c_str(), survey_id, option_id);
            if (!db_conn->ExecuteCreate(sql_cmd)) {
                LogError("插入记录操作失败");
                return -1;
            }
        } else {
            // 如果存在，更新记录
            sprintf(sql_cmd, "UPDATE Responses SET answer = '%s' WHERE question_id = %d AND user_id = %d AND survey_id = %d AND option_id = %d", answer.c_str(), question_id, user_id, survey_id, option_id);
            if (!db_conn->ExecuteCreate(sql_cmd)) {
                LogError("更新记录操作失败");
                return -1;
            }
        }
    } else {
        // 对于填空题，查询是否已存在相同的记录
        sprintf(sql_cmd, "SELECT COUNT(*) FROM Responses WHERE question_id = %d AND user_id = %d AND survey_id = %d", question_id, user_id, survey_id);
        CResultSet *result_set6 = db_conn->ExecuteQuery(sql_cmd);
        int count = 0;
        if (result_set6 && result_set6->Next()) {
            count = result_set6->GetInt("COUNT(*)");
        }
        delete result_set6;

        if (count == 0) {
            // 如果不存在，插入记录
            sprintf(sql_cmd, "insert into Responses (question_id, user_id, answer, survey_id) values (%d, %d, '%s', %d)", question_id, user_id, answer.c_str(), survey_id);
            if (!db_conn->ExecuteCreate(sql_cmd)) {
                LogError("插入记录操作失败");
                return -1;
            }
        } else {
            // 如果存在，更新记录
            sprintf(sql_cmd, "UPDATE Responses SET answer = '%s' WHERE question_id = %d AND user_id = %d AND survey_id = %d", answer.c_str(), question_id, user_id, survey_id);
            if (!db_conn->ExecuteCreate(sql_cmd)) {
                LogError("更新记录操作失败");
                return -1;
            }
        }
    }

    // 更新 is_filled 字段
    string update_sql = "UPDATE Surveys SET is_filled = 1 WHERE survey_id = ?";
    LogInfo("执行更新is_filled的语句: {}", update_sql);

    CPrepareStatement *update_stmt = new CPrepareStatement();
    if (update_stmt->Init(db_conn->GetMysql(), update_sql)) {
        uint32_t update_index = 0;
        update_stmt->SetParam(update_index++, survey_id);
        bool update_bRet = update_stmt->ExecuteUpdate();
        if (update_bRet) {
            LogInfo("成功将is_filled更新为1，对应survey_id: {}", user_id);
        } else {
            LogError("更新is_filled失败，对应survey_id: {}", user_id);
        }
    }
    delete update_stmt;

    // 更新缓存
    if (CacheIncrCount(TABLE_USER_COUNT + user) < 0) {
        LogError("CacheIncrCount 操作失败");
    }

    return ret;
}



// 辅助函数，用于将vector<int> 类型的question_ids转换为逗号分隔的字符串，以便在SQL语句中使用IN关键字
string getQuestionIdListString(const vector<int>& question_ids) {
    stringstream ss;
    for (size_t i = 0; i < question_ids.size(); ++i) {
        if (i > 0) {
            ss << ", ";
        }
        ss << question_ids[i];
    }
    return ss.str();
}

//这个函数功能是用户删除自己填写的表
int handleDeleteTable(string user, string table_name) {
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    int user_id = -1;
    int survey_id = -1;
    int is_dead=0;

    // 先从user获取user_id
    sprintf(sql_cmd, "select user_id from Users where user_name = '%s'", user.c_str());
    CResultSet *result_set = db_conn->ExecuteQuery(sql_cmd);
    if (result_set && result_set->Next()) {
        // 从结果获取到user_id
        user_id = result_set->GetInt("user_id");
        ret = 0;
    } else {
        LogError("未能获取到用户 {} 的user_id", user);
        ret = -1;
        return ret;
    }
    delete result_set;

    memset(sql_cmd, 0, sizeof(sql_cmd));

    // 根据表名和user_id查到survey_id
    sprintf(sql_cmd, "select survey_id,is_dead from Surveys where title = '%s' and user_id = %d", table_name.c_str(), user_id);
    result_set = db_conn->ExecuteQuery(sql_cmd);
    if (result_set && result_set->Next()) {
        // 从结果获取到survey_id
        survey_id = result_set->GetInt("survey_id");
        is_dead = result_set->GetInt("is_dead");
        if(is_dead == 1){
            return -1;
        }
        ret = 0;
    } else {
        LogError("未能根据表名 {} 和用户ID {} 获取到survey_id", table_name, user_id);
        ret = -1;
        return ret;;
    }
    delete result_set;

    // 根据survey_id查到Questions表中所有相关问题的question_id
    vector<int> question_ids;
    memset(sql_cmd, 0, sizeof(sql_cmd));
    sprintf(sql_cmd, "select question_id from Questions where survey_id = %d", survey_id);
    result_set = db_conn->ExecuteQuery(sql_cmd);
    while (result_set && result_set->Next()) {
        // 将获取到的每个question_id存入vector
        question_ids.push_back(result_set->GetInt("question_id"));
    }
    delete result_set;

    // 删除Responses表中所有符合条件的记录（该用户填写的这个表的所有答案）
    if (!question_ids.empty()) {
        // 构建删除语句，使用IN关键字处理多个question_id
        sprintf(sql_cmd, "delete from Responses where user_id = %d and question_id IN (%s)", user_id, getQuestionIdListString(question_ids).c_str());
        if (!db_conn->ExecuteDrop(sql_cmd)) {
            // 执行删除语句
            LogError("{} 操作失败", sql_cmd);
            ret = -1;
            return ret;
        }
    } else {
        LogError("未找到与survey_id {} 相关的任何问题ID，无法删除Responses表中的记录", survey_id);
        ret = -1;
        return ret;
    }

    // 查询用户表数量-1（假设这里和更新操作类似，也需要对缓存进行处理，这里是数量减1）
    if (CacheDecrCount(TABLE_USER_COUNT + user) < 0) {
        LogError(" CacheDecrCount 操作失败");
    }

END:
    return ret;
}

//我希望前端发来的一张用户填好的表的json格式如下：
/*
{
    user_name:xxx,
    token:xxx,
    table_name xxx,
    {
        question_text xxx,
        answer xxx,
    }
    {
        question_text xxx,
        answer xxx,
    }
    ...
}
*/
//当调用到这个函数的时候，post_data已经被http_conn函数解析到上面的格式
//这个函数的任务是用户上传填好的表，后端存到数据库
int ApiUploadTable(string &url, string &post_data, string &str_json) {
    //url这个参数没有用
    UNUSED(url);
    int ret = 0;
    string table_name;
    string question_name;
    string answer;
    string user_name;
    string token;

    // 创建JSON解析器对象
    Json::Reader reader;
    Json::Value root;

    // 解析JSON数据
    if (!reader.parse(post_data, root)) {
        LogError("JSON解析失败: {}", reader.getFormattedErrorMessages());
        ret = -1;
        goto END;
    }

    // 获取用户名称
    user_name = root["user_name"].asString();
    printf("%s\n",user_name.c_str());

    //获取token,并验证
    token = root["token"].asString();
    //验证登陆token，成功返回0，失败-1
    ret = VerifyToken(user_name, token); // util_cgi.h
    if(ret == 0){  //验证成功
        // 获取问卷名称
        table_name = root["table_name"].asString();
        printf("%s\n",table_name.c_str());

        // 获取问题和答案数组
        const Json::Value& answers = root["answers"];  // 假设问题数据在JSON中的键为"questions"，根据实际情况修改
        for (const auto& ans : answers) {
            // 获取问题文本
            question_name = ans["question_text"].asString();
            printf("%s\n",question_name.c_str());

            // 获取答案
            answer = ans["answer"].asString();
            printf("%s\n",answer.c_str());

            // 调用handleUpdateQuestion函数插入答案到数据库
            if (handleUpdateQuestion(user_name, question_name,table_name, answer)!= 0) {
                LogError("插入答案到数据库失败");
                ret = -1;
                goto END;
            }
        }
    }else{
        goto END;
    }

END:
    Json::Value value;
    value["code"] = (ret == 0)? 0 : 1;
    str_json = value.toStyledString();
    return ret;
}


/*
{
    user_name:xxx,
    token:xxx,
    table_name xxx,
}
*/
//下面这个函数的目的是用户想要删除填好的一张表
int ApiDeleteTable(string &url, string &post_data, string &str_json){
    //url这个参数没有用
    UNUSED(url);
    int ret = 0;
    string table_name;
    string question_name;
    string answer;
    string user_name;
    string token;

    // 创建JSON解析器对象
    Json::Reader reader;
    Json::Value root;

    // 解析JSON数据
    if (!reader.parse(post_data, root)) {
        LogError("JSON解析失败: {}", reader.getFormattedErrorMessages());
        ret = -1;
        goto END;
    }

    // 获取用户名称
    user_name = root["user_name"].asString();

    //获取token,并验证
    token = root["token"].asString();
    //验证登陆token，成功返回0，失败-1
    ret = VerifyToken(user_name, token); // util_cgi.h

    if(ret == 0){  //验证成功
        // 获取问卷名称
        table_name = root["table_name"].asString();

        //接着删除这个table_name表所有的信息
        if (handleDeleteTable(user_name,table_name)!= 0) {
            LogError("删除答案到数据库失败");
            ret = -1;
            goto END;
        }
    }else{
        goto END;
    }

END:
    Json::Value value;
    value["code"] = (ret == 0)? 0 : 1;
    str_json = value.toStyledString();
    return ret;
}