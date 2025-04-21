#include "api_register.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api_common.h"
#include "http_conn.h"
#include <sys/time.h>
#include <time.h>

//解析用户注册信息的json包
/*json数据如下
    {
        userName:xxxx,
        firstPwd:xxx,
    }
*/

int decodeRegisterJson(const std::string &str_json,string &user_name,string &pwd){
    bool res;
    Json::Value root;
    Json::Reader jsonReader; //用来解析json格式的字符串
    //提供parse方法按规则解析
    res = jsonReader.parse(str_json,root);
    if (!res) {
        LogError("parse reg json failed ");
        return -1;
    }

    //用户名
    if(root["userName"].isNull()){
        LogError("userName null\n");
        return -1;
    }
    user_name = root["userName"].asString();

    //密码
    if (root["firstPwd"].isNull()) {
        LogError("firstPwd null\n");
        return -1;
    }
    pwd = root["firstPwd"].asString();

    return 0;
}

//封装注册用户的json,比如用户注册之后，如果成功，返回值是0，否则是1
int encodeRegisterJson(int ret ,string &str_json){
    Json::Value root;
    root["code"] = ret;
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

template <typename... Args>
std::string formatString2(const std::string &format, Args... args) {
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...) +
                1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                       buf.get() + size - 1); // We don't want the '\0' inside
}

int registerUser(string &user_name, string &pwd) {
    int ret = 0;
    uint32_t user_id;
    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);

    string str_sql;
    // 查询用户是否已存在的SQL语句
    printf("%s\n",user_name.c_str());
    str_sql = formatString2("select * from Users where user_name='%s'", user_name.c_str());
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());

    if(result_set && result_set->Next()){
        // 存在在返回
        LogWarn("id: {}, user_name: {}  已经存在", result_set->GetInt("id"), result_set->GetString("user_name"));
        delete result_set;
        ret = 2;
    }else{
        // 如果用户不存在，则进行注册
        str_sql = "insert into Users (user_name, password) values (?,?)";
        LogInfo("执行: {}", str_sql);

        // 必须在释放连接前delete
        // CPrepareStatement对象，否则有可能多个线程操作mysql对象，会crash
        CPrepareStatement *stmt = new CPrepareStatement();
        if(stmt->Init(db_conn->GetMysql(),str_sql)){
            uint32_t index = 0;
            stmt->SetParam(index++,user_name);
            stmt->SetParam(index++, pwd);
            bool bRet = stmt->ExecuteUpdate();
            if (bRet) {
                ret = 0;
                user_id = db_conn->GetInsertId();
                LogInfo("insert user_id: {}", user_id);
            } else {
                LogError("insert user_info failed. {}", str_sql);
                ret = 1;
            }
        }
        delete stmt;
    }
    return ret;
}


#define HTTP_RESPONSE_HTML                                                     \
    "HTTP/1.1 200 OK\r\n"                                                      \
    "Connection:close\r\n"                                                     \
    "Content-Length:%d\r\n"                                                    \
    "Content-Type:application/json;charset=utf-8\r\n\r\n%s"
//注册信息的 post_data
int ApiRegisterUser(uint32_t conn_uuid, std::string &url, std::string &post_data){
    string str_json;
    UNUSED(url);
    int ret = 0;
    string user_name;
    string pwd;

    LogInfo("uuid: {}, url: {}, post_data: {}", conn_uuid, url, post_data);

    // 判断数据是否为空
    if (post_data.empty()) {
        LogError("decodeRegisterJson failed");
        encodeRegisterJson(1, str_json);
        ret = -1;
        goto END;
    }
    // 解析json
    if (decodeRegisterJson(post_data, user_name,pwd) <
        0) {
        LogError("decodeRegisterJson failed");
        encodeRegisterJson(1, str_json);
        ret = -1;
        goto END;
    }

    //注册账号
    ret = registerUser(user_name,pwd);
    ret = encodeRegisterJson(ret, str_json);
    // 这里是裸数据
    // 发送到回发队列里

//!!!!!!!!!!!!!!!!!!!!!!!!!标签只是一个位置，正常顺序执行过来的代码也会执行下面的逻辑
END:
    char *str_content = new char[HTTP_RESPONSE_HTML_MAX];
    uint32_t ulen = str_json.length();
    snprintf(str_content, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, ulen,
             str_json.c_str());
    str_json = str_content;
    CHttpConn::AddResponseData(conn_uuid, str_json);
    delete[] str_content;

    return ret;
}

