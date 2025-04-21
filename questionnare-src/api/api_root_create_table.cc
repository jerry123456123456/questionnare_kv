#include "api_root_create_table.h"
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api_common.h"
#include "json/json.h"
#include "db_pool.h"
#include "http_conn.h"
#include <sys/time.h>
#include "netlib.h"
#include"util.h"
#include <sstream>
#include <iomanip>
#include <ctime>

int encodeCountJson3(int ret, string &str_json) {
    Json::Value root;
    root["code"] = ret;
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

template <typename... Args>
std::string FormatString5(const std::string &format, Args... args) {
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) +
                1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                      buf.get() + size - 1); // We don't want the '\0' inside
}


//http://192.168.186.138/api/root/table_create?cmd=users
//获取所有用户的名字
/*
{
    {
        "user_name":xxx
    },
    {
        "user_name":xxx
    },
    {
        "user_name":xxx
    }
}
*/
int getUsers(string &str_json){
    int ret = 0;
    std::string str_sql;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    string user_name;
    str_sql = FormatString5("select user_name from Users where user_name != 'root'");
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());

    // 创建一个 JSON 根对象
    Json::Value root;
    Json::Value users(Json::arrayValue);  // 创建一个数组来存放用户名字

    while(result_set->Next()){
        user_name = result_set->GetString("user_name");

        // 为每个用户创建一个对象并添加到数组中
        Json::Value user;
        user["user_name"] = user_name;
        users.append(user);
    }

    // 将用户数组添加到根对象
    root = users;

    // 返回 JSON 数据
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

/*
{
    "survey_title": "调查问卷标题示例", // 调查问卷的标题
    "deadline":xxx,  //截止时间
    "users":[
        {
            "user_name":xxx
        },
        {
            "user_name":xxx
        }
        ...
    ]
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
//解析上面的json字符串
struct User {
    string user_name;
};

struct Option {
    string option_text;
};

struct Question {
    string question_text;
    string question_type;
    vector<Option> options;
};

struct SurveyData {
    string survey_title;
    string deadline;
    vector<User> users;
    vector<Question> questions;
};

int decodeSurveyJson(const string &str_json, SurveyData &survey_data) {
    bool res;
    Json::Value root;
    Json::Reader jsonReader;

    // 解析 JSON
    res = jsonReader.parse(str_json, root);
    if (!res) {
        cerr << "Failed to parse JSON" << endl;
        return -1;
    }

    // 解析调查问卷标题
    if (root["survey_title"].isNull()) {
        cerr << "survey_title is null" << endl;
        return -1;
    }
    survey_data.survey_title = root["survey_title"].asString();

    // 解析截止时间
    if (root["deadline"].isNull()) {
        cerr << "deadline is null" << endl;
        return -1;
    }
    survey_data.deadline = root["deadline"].asString();

    // 解析用户列表
    if (root["users"].isNull()) {
        cerr << "users is null" << endl;
        return -1;
    }
    const Json::Value users = root["users"];
    for (unsigned int i = 0; i < users.size(); ++i) {
        User user;
        if (!users[i]["user_name"].isNull()) {
            user.user_name = users[i]["user_name"].asString();
        }
        survey_data.users.push_back(user);
    }

    // 解析问题列表
    if (root["questions"].isNull()) {
        cerr << "questions is null" << endl;
        return -1;
    }
    const Json::Value questions = root["questions"];
    for (unsigned int i = 0; i < questions.size(); ++i) {
        Question question;
        if (!questions[i]["question_text"].isNull()) {
            question.question_text = questions[i]["question_text"].asString();
        }
        if (!questions[i]["question_type"].isNull()) {
            question.question_type = questions[i]["question_type"].asString();
        }

        // 解析选项
        const Json::Value options = questions[i]["options"];
        for (unsigned int j = 0; j < options.size(); ++j) {
            Option option;
            if (!options[j]["option_text"].isNull()) {
                option.option_text = options[j]["option_text"].asString();
            }
            question.options.push_back(option);
        }

        survey_data.questions.push_back(question);
    }

    return 0; // 成功
}


//到时间触发的回调函数
void table_cb(void *callback_data, uint8_t msg, uint32_t handle,
                              void *pParam){
    struct table_cb_t* table = (table_cb_t*)callback_data;

    string table_name = table->table_name;
    string user_name = table->user_name;

    int ret = 0;
    string str_sql;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    int user_id = 0;
    int survey_id = 0;
    int question_id = 0;
    int option_id = 0;

    str_sql = FormatString5("select user_id from Users where user_name = '%s'", user_name.c_str());
    printf("%s\n", str_sql.c_str());
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set->Next()) {
        user_id = result_set->GetInt("user_id");
    }
    delete result_set;

    str_sql = FormatString5("select survey_id from Surveys where user_id = %d and title = '%s'", user_id,table_name.c_str());
    printf("%s\n", str_sql.c_str());
    CResultSet *result_set1 = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set1->Next()) {
        survey_id = result_set1->GetInt("survey_id");
    }
    delete result_set1;

    // 先将根记录插入Surveys表（假设user_id为1代表根用户之类的特殊情况）
    str_sql = FormatString5("update Surveys set is_dead = 1 where survey_id = %d",survey_id);
    printf("%s\n", str_sql.c_str());
    if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
        return;
    }

    ////紧接着更新Questions表
    str_sql = FormatString5("select question_id from Questions where survey_id = %d",survey_id);
    printf("%s\n", str_sql.c_str());
    CResultSet *result_set2 = db_conn->ExecuteQuery(str_sql.c_str());
    while (result_set2->Next()) {
        question_id = result_set2->GetInt("question_id");

        str_sql = FormatString5("update Questions set is_dead = 1 where question_id = %d",question_id);
        printf("%s\n", str_sql.c_str());
        if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
            return;
        }

        ////紧接着更新Options表
        str_sql = FormatString5("select option_id from Options where question_id = %d",question_id);
        printf("%s\n", str_sql.c_str());
        CResultSet *result_set3 = db_conn->ExecuteQuery(str_sql.c_str());
        while (result_set3->Next()) {
            option_id = result_set3->GetInt("option_id");

            str_sql = FormatString5("update Options set is_dead = 1 where option_id = %d",option_id);
            printf("%s\n", str_sql.c_str());
            if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                return;
            }
        }
        delete result_set3;
    }
    delete result_set2;

    //事件触发之后，去掉这个定时器
    netlib_delete_timer(table_cb,(void *)table);
    return;
}

//获取时间差
int times(std::string deadline, uint64_t &interval) {
    // 定义时间格式字符串，用于解析传入的时间字符串
    std::string format = "%Y-%m-%dT%H:%M";
    std::tm tm_deadline = {};
    // 将传入的截止时间字符串解析为std::tm结构体表示的时间
    printf("%s\n",deadline.c_str());
    std::istringstream ss(deadline);
    ss >> std::get_time(&tm_deadline, format.c_str());
    if (ss.fail()) {
        // 如果解析失败，返回错误码（这里简单返回 -1 表示解析时间字符串出错，你可以按需调整错误处理逻辑）
        return -1;
    }

    // 获取当前时间的毫秒数（调用已有的GetTickCount函数）
    uint64_t current_time_ms = GetTickCount();
    printf("%d\n",current_time_ms);

    // 将std::tm结构体表示的时间转换为time_t类型（从1970年1月1日00:00:00 UTC到指定时间的秒数）
    std::time_t time_deadline_t = std::mktime(&tm_deadline);
    // 将time_t类型的时间转换为timeval结构体，用于后续处理
    struct timeval time_deadline_tv;
    time_deadline_tv.tv_sec = time_deadline_t;
    time_deadline_tv.tv_usec = 0;

    // 将timeval结构体表示的截止时间转换为毫秒数
    uint64_t deadline_time_ms = time_deadline_tv.tv_sec * 1000L + time_deadline_tv.tv_usec / 1000L;
    printf("%d\n",deadline_time_ms);

    // 计算时间差（单位为毫秒）
    interval = static_cast<int64_t>(deadline_time_ms - current_time_ms);

    return 0;
}

// 向定时器中添加一个表格的截止
int AddTableTimer(std::string &table_name, std::string &user_name, uint64_t interval) {
    // 为table_cb_t结构体指针分配内存空间，确保其指向一个有效的table_cb_t对象
    struct table_cb_t* table = new table_cb_t();
    if (table == nullptr) {
        // 如果内存分配失败，返回错误码（这里返回 -1 表示内存分配失败，你可按需调整错误处理逻辑）
        return -1;
    }

    // 使用赋值操作符为结构体中的成员变量赋值
    table->table_name = table_name;
    table->user_name = user_name;

    // 注册定时任务，这里假设netlib_register_timer的第一个参数是回调函数指针，第二个参数是传递的参数（这里是table指针），第三个参数是时间间隔
    netlib_register_timer(table_cb, (void*)table, interval);

    return 0;
}

//将创建好的问卷添加到数据库中
/*
struct User {
    string user_name;
};

struct Option {
    string option_text;
};

struct Question {
    string question_text;
    string question_type;
    vector<Option> options;
};

struct SurveyData {
    string survey_title;
    string deadline;
    vector<User> users;
    vector<Question> questions;
};
*/
int createTables(SurveyData &surveydata, string &str_json) {
    int ret = 0;
    string str_sql;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    string survey_title = surveydata.survey_title;
    string deadline = surveydata.deadline;
    uint64_t interval = 0;

    // 用于存储根用户选择题的所有option_text
    std::vector<std::string> multipleChoiceRootOptions;  

    // 先将根记录插入Surveys表（假设user_id为1代表根用户之类的特殊情况）
    str_sql = FormatString5("insert into Surveys (title,user_id,deadline) values('%s',1,'%s')", survey_title.c_str(), deadline.c_str());
    printf("%s\n", str_sql.c_str());
    if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
        encodeCountJson3(1, str_json);
        return -1;
    }
    // 获取刚插入的根问卷的survey_id（通过查询获取，假设survey_id是自增主键且无重复情况）
    int root_survey_id;
    int root_question_id;
    int root_option_id;
    str_sql = FormatString5("select survey_id from Surveys where user_id = 1 and title = '%s' order by survey_id desc limit 1", survey_title.c_str());
    printf("%s\n", str_sql.c_str());
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set->Next()) {
        root_survey_id = result_set->GetInt("survey_id");
        for (size_t i = 0; i < surveydata.questions.size(); ++i) {
            const Question& question = surveydata.questions[i];
            string question_text = question.question_text;
            string question_type = question.question_type;
            if (question.question_type == "单选") {
                question_type = "single_choice";
            } else if (question.question_type == "多选") {
                question_type = "multiple_choice";
            } else if (question.question_type == "填空") {
                question_type = "fill_in_blank";
            } else {
                std::cerr << "Invalid question type: " << question.question_type << ", skipping this question insertion." << std::endl;
                continue;
            }
            //如果填空题的question_text在选择题的option_text中，则跳过
            if (question_type!= "fill_in_blank") {
                //插入到数据库中
                str_sql = FormatString5("insert into Questions (survey_id,question_text,question_type) values(%d,'%s','%s')", root_survey_id, question_text.c_str(), question_type.c_str());
                printf("%s\n", str_sql.c_str());
                if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                    encodeCountJson3(1, str_json);
                    return -1;
                }

                //再查询这个question_id
                str_sql = FormatString5("select question_id from Questions where survey_id = %d and question_text = '%s'", root_survey_id, question_text.c_str());
                printf("%s\n", str_sql.c_str());
                CResultSet *result_set1 = db_conn->ExecuteQuery(str_sql.c_str());
                if (result_set1->Next()) {
                    root_question_id = result_set1->GetInt("question_id");

                    //接下来要插入option
                    for (size_t j = 0; j < surveydata.questions[i].options.size(); ++j) {
                        const Option& option = surveydata.questions[i].options[j];
                        string option_text = option.option_text;
                        //存储option_text
                        multipleChoiceRootOptions.push_back(option_text);
                        //插入到数据库中
                        str_sql = FormatString5("insert into Options (question_id,option_text) values(%d,'%s')", root_question_id, option_text.c_str());
                        printf("%s\n", str_sql.c_str());
                        if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                            encodeCountJson3(1, str_json);
                            return -1;
                        }
                    }
                }
                delete result_set1;
            } else {  //当填空题的时候
                //判断是否存在
                bool skipInsertion = false;
                for (const std::string& option : multipleChoiceRootOptions) {
                    if (question_text == option) {
                        skipInsertion = true;
                        break;
                    }
                }
                if (skipInsertion) {
                    continue;
                }

                //插入到数据库中
                str_sql = FormatString5("insert into Questions (survey_id,question_text,question_type) values(%d,'%s','%s')", root_survey_id, question_text.c_str(), question_type.c_str());
                printf("%s\n", str_sql.c_str());
                if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                    encodeCountJson3(1, str_json);
                    return -1;
                }
                //再查询这个question_id
                str_sql = FormatString5("select question_id from Questions where survey_id = %d and question_text = '%s'", root_survey_id, question_text.c_str());
                printf("%s\n", str_sql.c_str());
                CResultSet *result_set1 = db_conn->ExecuteQuery(str_sql.c_str());
                if (result_set1->Next()) {
                    root_question_id = result_set1->GetInt("question_id");

                    //接下来要插入option
                    for (size_t j = 0; j < surveydata.questions[i].options.size(); ++j) {
                        const Option& option = surveydata.questions[i].options[j];
                        string option_text = option.option_text;
                        //存储option_text
                        multipleChoiceRootOptions.push_back(option_text);
                        //插入到数据库中
                        str_sql = FormatString5("insert into Options (question_id,option_text) values(%d,'%s')", root_question_id, option_text.c_str());
                        printf("%s\n", str_sql.c_str());
                        if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                            encodeCountJson3(1, str_json);
                            return -1;
                        }
                    }
                }
                delete result_set1;
            }
        }
    }
    delete result_set;

    // 插入普通用户对应的survey记录，并获取各自的survey_id保存起来
    std::vector<int> user_survey_ids;
    for (auto it = surveydata.users.begin(); it!= surveydata.users.end(); ++it) {
        string user_name = it->user_name;

        //设置截止时间
        //设置截止时间
        ret = times(deadline,interval);
        AddTableTimer(survey_title,user_name,interval);

        int user_id;
        str_sql = FormatString5("select user_id from Users where user_name = '%s'", user_name.c_str());
        printf("%s\n", str_sql.c_str());
        CResultSet *result_set1 = db_conn->ExecuteQuery(str_sql.c_str());
        if (result_set1->Next()) {
            user_id = result_set1->GetInt("user_id");
            // 插入到Surveys表
            str_sql = FormatString5("insert into Surveys (user_id,title,root_survey_id,deadline) values(%d,'%s',%d,'%s')", user_id, survey_title.c_str(), root_survey_id, deadline.c_str());
            printf("%s\n", str_sql.c_str());
            if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                encodeCountJson3(1, str_json);
                return -1;
            }
            // 获取刚插入的该用户问卷的survey_id（通过查询获取）
            int user_survey_id;
            str_sql = FormatString5("select survey_id from Surveys where user_id = %d and title = '%s' order by survey_id desc limit 1", user_id, survey_title.c_str());
            printf("%s\n", str_sql.c_str());
            CResultSet *result_set2 = db_conn->ExecuteQuery(str_sql.c_str());
            if (result_set2->Next()) {
                user_survey_id = result_set2->GetInt("survey_id");
            }
            delete result_set2;
            user_survey_ids.push_back(user_survey_id);
        }
        delete result_set1;
    }

    // 插入普通用户的Questions表数据，并关联根用户相关id，同时跳过已存在于选择题选项中的填空题
    for (size_t i = 0; i < surveydata.questions.size(); ++i) {
        const Question& question = surveydata.questions[i];
        string question_text = question.question_text;
        string question_type = question.question_type;
        if (question.question_type == "单选") {
            question_type = "single_choice";
        } else if (question.question_type == "多选") {
            question_type = "multiple_choice";
        } else if (question.question_type == "填空") {
            question_type = "fill_in_blank";
        } else {
            std::cerr << "Invalid question type: " << question.question_type << ", skipping this question insertion." << std::endl;
            continue;
        }

        for (size_t user_idx = 0; user_idx < user_survey_ids.size(); user_idx++) {
            int survey_id_to_use = user_survey_ids[user_idx];
            if (question_type == "fill_in_blank") {
                bool skipInsertion = false;
                for (const std::string& option : multipleChoiceRootOptions) {
                    if (question_text == option) {
                        skipInsertion = true;
                        break;
                    }
                }
                if (skipInsertion) {
                    continue;
                }
            }
            //插入到数据库中
            str_sql = FormatString5("insert into Questions (survey_id,question_text,question_type) values(%d,'%s','%s')", survey_id_to_use, question_text.c_str(), question_type.c_str());
            printf("%s\n", str_sql.c_str());
            if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                encodeCountJson3(1, str_json);
                return -1;
            }

            //再查询这个question_id
            str_sql = FormatString5("select question_id from Questions where survey_id = %d and question_text = '%s'", survey_id_to_use, question_text.c_str());
            printf("%s\n", str_sql.c_str());
            CResultSet *result_set3 = db_conn->ExecuteQuery(str_sql.c_str());
            if (result_set3->Next()) {
                int user_question_id = result_set3->GetInt("question_id");
                // 关联根用户的question_id

                {
                    //先找到root_survey_id
                    str_sql = FormatString5("select survey_id from Surveys where title = '%s' and user_id = 1", survey_title.c_str());
                    printf("%s\n", str_sql.c_str());
                    CResultSet *root_survey_Set = db_conn->ExecuteQuery(str_sql.c_str());
                    if(root_survey_Set->Next()){
                        root_survey_id = root_survey_Set->GetInt("survey_id");
                    }

                    //还得先找root_question_id
                    str_sql = FormatString5("select question_id from Questions where survey_id = %d and question_text = '%s'", root_survey_id, question_text.c_str());
                    printf("%s\n", str_sql.c_str());
                    CResultSet *root_question_Set = db_conn->ExecuteQuery(str_sql.c_str());
                    if(root_question_Set->Next()){
                        root_question_id = root_question_Set->GetInt("question_id");
                    }

                    str_sql = FormatString5("update Questions set root_question_id = %d where question_id = %d", root_question_id, user_question_id);
                    printf("%s\n", str_sql.c_str());
                    if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                        encodeCountJson3(1, str_json);
                        return -1;
                    }

                }

                //接下来要插入option
                for (size_t j = 0; j < surveydata.questions[i].options.size(); ++j) {
                    printf("。。。。。。。。。。。。。。。。。\n");
                    const Option& option = surveydata.questions[i].options[j];
                    string option_text = option.option_text;
                    //首先要先获取root_option_id
                    str_sql = FormatString5("select root_question_id from Questions where question_id = %d",user_question_id);
                    CResultSet *result_set5 = db_conn->ExecuteQuery(str_sql.c_str());
                    if(result_set5->Next()){
                        root_question_id = result_set5->GetInt("root_question_id");
                        str_sql = FormatString5("select option_id from Options where question_id = %d and option_text = '%s'",root_question_id,option_text.c_str());
                        CResultSet *result_set6 = db_conn->ExecuteQuery(str_sql.c_str());
                        if(result_set6->Next()){
                            root_option_id = result_set6->GetInt("option_id");
                        }
                        delete result_set6;
                    }
                    delete result_set5;

                    //插入到数据库中
                    str_sql = FormatString5("insert into Options (question_id,option_text,root_option_id) values(%d,'%s',%d)", user_question_id, option_text.c_str(),root_option_id);
                    printf("%s\n", str_sql.c_str());
                    if (db_conn->ExecuteUpdate(str_sql.c_str()) < 0) {
                        encodeCountJson3(1, str_json);
                        return -1;
                    }
                }
            }
            delete result_set3;
        }
    }

    // 返回 JSON 数据（原代码中这部分逻辑可根据实际需求进一步完善，比如构建合适的Json内容等）
    printf("/////////////////////////////////////////////\n");

    Json::Value value;
    value["code"] = (ret == 0)? 0 : 1;
    str_json = value.toStyledString();
    return ret;
}

int ApiRootTableCreate(string &url,string &post_data,string &str_json){
    UNUSED(url);
    char cmd[20];
    int ret = 0;
    SurveyData surveydata;

    //解析命令 解析url获取自定义参数
    QueryParseKeyValue(url.c_str(), "cmd", cmd, NULL);
    LogInfo("url: {}, cmd: {} ",url, cmd);

    if(strcmp(cmd,"users") == 0){
        //没有json字符串，不需要解码
        if(getUsers(str_json)<0){
            ret = -1;
            return -1;   
        }
    }else{
        if(strcmp(cmd, "normal") != 0){
            LogError("unknow cmd: {}", cmd);
            encodeCountJson3(1,  str_json);
            return -1;
        }

        //解析传来的json字符串
        ret = decodeSurveyJson(post_data,surveydata);
        if(ret == 0){
            //这里前端问题目前解决不了，这里就现在后端代码中解决这个问题
            //在数据库删除这个问卷
            if (createTables(surveydata,str_json) < 0) {
                encodeCountJson3(1,str_json);
                return -1;
            }          
        }else{
            encodeCountJson3(1, str_json);
            return -1;
        }
    }

    return 0;
}