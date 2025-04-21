#include"api_common.h"
#include <string>

string s_dfs_path_client;
string s_storage_web_server_ip;
string s_storage_web_server_port;

//在这里模版与可变参数相辅相成,字符串的格式化和printf的参数填充相似
template<typename... Args>
std::string FormatString(const std::string &format, Args... args) {
    auto size = std::snprintf(NULL, 0, format.c_str(), args...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                       buf.get() + size - 1); // We don't want the '\0' inside
}

//获取每个用户可以填写的调查问卷表的个数
int DBGetUserTableCountByUsername(CDBConn *db_conn, string user_name, int &count) {
    count = 0;
    int ret = 0;
    string str_sql;

    // 先通过用户名获取用户 ID
    str_sql = FormatString("select user_id from Users where user_name='%s'", user_name.c_str());
    LogInfo("执行：{}", str_sql);
    CResultSet *user_id_result_set = db_conn->ExecuteQuery(str_sql.c_str());
    int user_id = 0;
    if (user_id_result_set && user_id_result_set->Next()) {
        user_id = user_id_result_set->GetInt("user_id");
        printf("user_id : %d\n",user_id);
        delete user_id_result_set;
    } else {
        // 用户不存在
        LogError("找不到用户：{}", str_sql);
        ret = -1;
        return ret;
    }

    // 再使用用户 ID 查询调查问卷表个数
    str_sql = FormatString("select count(*) from Surveys where user_id=%d", user_id);
    LogInfo("执行：{}", str_sql);
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set && result_set->Next()) {
        count = result_set->GetInt("count(*)");
        ret = 0;
        delete result_set;
    } else if (!result_set) {
        // 操作失败
        LogError("{} 操作失败", str_sql);
        ret = -1;
    } else {
        // 没有记录则初始化记录数量为 0
        ret = 0;
        LogInfo("没有记录: count: {}", count);
    }
    return ret;
}

//获取所有调查问卷的数量
int DBGetAllTablesCount(CDBConn *db_conn,int &count){
    count = 0;
    int ret = 0;

    //先查看用户是否存在
    string str_sql = "SELECT COUNT(DISTINCT title) AS total_unique_surveys FROM Surveys";
    printf("%s\n",str_sql.c_str());
    CResultSet *result_set = db_conn->ExecuteQuery(str_sql.c_str());
    if (result_set && result_set->Next()) {      //这个是执行语句返回的结果，可以按行循环读取
        // 存在在返回
        count = result_set->GetInt("count(*)");
        LogInfo("count: {}", count);
        ret = 0;
        delete result_set;
    } else if (!result_set) {
        // 操作失败
        LogError("{} 操作失败", str_sql);
        ret = -1;
    } else {
        ret = 0;
        LogInfo("没有记录: count: {}", count);
    }

    return ret;
}