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

//mysql存储的部分信息加载kvstore缓存
static int ApiInit() {
    if (!connect_to_kvstore()) {
        LogError("Failed to connect to kvstore");
        return -1;
    }

    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");  //从数据库管理器获取一个名为 "tuchuang_slave" 的数据库连接对象。这个连接对象用于执行数据库操作
    AUTO_REL_DBCONN(db_manager, db_conn);

    int ret = 0;
    // 共享文件的数量 统计加载到kvstore
    int count = 0;
    ret = DBGetAllTablesCount(db_conn, count);
    printf("%d\n", count);
    if (ret < 0) {
        LogError("GetTablesCount failed");
        close_kvstore_connection();
        return -1;
    }
    // 加载到kvstore
    ret = CacheSetCount(nullptr, "TABLE_USER_COUNT", (int64_t)count);
    if (ret < 0) {
        LogError("CacheSetCount failed");
        close_kvstore_connection();
        return -1;
    }

    // 对于个人的文件数量 和图片分享数量，在登录的时候 读取到kvstore

    close_kvstore_connection();
    return 0;
}

static int CacheSetCount(void*, std::string key, int64_t count) {
    char command[1024];
    // 确保命令格式正确
    int command_length = snprintf(command, sizeof(command), "RSET %s %lld\n", key.c_str(), count);
    if (command_length < 0 || command_length >= sizeof(command)) {
        LogError("Command buffer overflow in CacheSetCount");
        return -1;
    }
    char response[1024];
    int ret = send_kvstore_command(command, response, sizeof(response));
    if (ret == 0 && strstr(response, "OK") != nullptr) {
        return 0;
    } else {
        LogError("Failed to set count in CacheSetCount. Response: %s", response);
        return -1;
    }
}

static int CacheGetCount(void*, std::string key, int64_t &count) {
    count = 0;
    char command[1024];
    // 确保命令格式正确
    int command_length = snprintf(command, sizeof(command), "RGET %s\n", key.c_str());
    if (command_length < 0 || command_length >= sizeof(command)) {
        LogError("Command buffer overflow in CacheGetCount");
        return -1;
    }
    char response[1024];
    int ret = send_kvstore_command(command, response, sizeof(response));
    if (ret == 0 && response[0] != '\0') {
        // 去除响应中的换行符
        char* newline = strchr(response, '\n');
        if (newline) {
            *newline = '\0';
        }
        count = atoll(response);
        return 0;
    } else {
        LogError("Failed to get count in CacheGetCount. Response: %s", response);
        return -1;
    }
}

// 键值递增
static int CacheIncrCount(void*, std::string key) {
    // 先使用 GET 命令获取当前值
    char get_command[1024];
    // 确保命令格式正确
    int get_command_length = snprintf(get_command, sizeof(get_command), "RGET %s\n", key.c_str());
    if (get_command_length < 0 || get_command_length >= sizeof(get_command)) {
        LogError("Command buffer overflow in CacheIncrCount (GET)");
        return -1;
    }
    char get_response[1024];
    int get_ret = send_kvstore_command(get_command, get_response, sizeof(get_response));
    if (get_ret != 0) {
        LogError("Failed to get count in CacheIncrCount. Response: %s", get_response);
        return -1;
    }

    // 将获取到的字符串转换为整数
    int64_t count = 0;
    if (get_response[0] != '\0') {
        // 去除响应中的换行符
        char* newline = strchr(get_response, '\n');
        if (newline) {
            *newline = '\0';
        }
        count = atoll(get_response);
    }

    // 对值进行加一操作
    count++;

    // 使用 SET 命令将新值存回
    char set_command[1024];
    // 确保命令格式正确
    int set_command_length = snprintf(set_command, sizeof(set_command), "RSET %s %lld\n", key.c_str(), count);
    if (set_command_length < 0 || set_command_length >= sizeof(set_command)) {
        LogError("Command buffer overflow in CacheIncrCount (SET)");
        return -1;
    }
    char set_response[1024];
    int set_ret = send_kvstore_command(set_command, set_response, sizeof(set_response));
    if (set_ret == 0) {
        LogInfo("{}-{}", key, count);
        return 0;
    } else {
        LogError("Failed to set count in CacheIncrCount. Response: %s", set_response);
        return -1;
    }
}

// 键值递减
static int CacheDecrCount(void*, std::string key) {
    // 先使用 GET 命令获取当前值
    char get_command[1024];
    // 确保命令格式正确
    int get_command_length = snprintf(get_command, sizeof(get_command), "RGET %s\n", key.c_str());
    if (get_command_length < 0 || get_command_length >= sizeof(get_command)) {
        LogError("Command buffer overflow in CacheDecrCount (GET)");
        return -1;
    }
    char get_response[1024];
    int get_ret = send_kvstore_command(get_command, get_response, sizeof(get_response));
    if (get_ret != 0) {
        LogError("Failed to get count in CacheDecrCount. Response: %s", get_response);
        return -1;
    }

    // 将获取到的字符串转换为整数
    int64_t count = 0;
    if (get_response[0] != '\0') {
        // 去除响应中的换行符
        char* newline = strchr(get_response, '\n');
        if (newline) {
            *newline = '\0';
        }
        count = atoll(get_response);
    }

    // 对值进行减一操作
    count--;

    // 检查值是否小于 0
    if (count < 0) {
        LogError("{} 请检测你的逻辑 decr  count < 0  -> {}", key, count);
        count = 0; // 文件数量最小为 0 值
    }

    // 使用 SET 命令将新值存回
    char set_command[1024];
    // 确保命令格式正确
    int set_command_length = snprintf(set_command, sizeof(set_command), "RSET %s %lld\n", key.c_str(), count);
    if (set_command_length < 0 || set_command_length >= sizeof(set_command)) {
        LogError("Command buffer overflow in CacheDecrCount (SET)");
        return -1;
    }
    char set_response[1024];
    int set_ret = send_kvstore_command(set_command, set_response, sizeof(set_response));
    if (set_ret == 0) {
        LogInfo("{}-{}", key, count);
        return 0;
    } else {
        LogError("Failed to set count in CacheDecrCount. Response: %s", set_response);
        return -1;
    }
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