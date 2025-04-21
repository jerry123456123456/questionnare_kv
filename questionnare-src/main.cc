#include "api_common.h"
#include "api_deal_table.h"
#include "api_login.h"
#include "api_register.h"
#include "api_mytables.h"
#include "config_file_reader.h"
#include "http_conn.h"
#include "netlib.h"
#include "util.h"
#include "dlog.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

std::string GetFromKvstore(const std::string& key);
std::string trimRight(const std::string& str);
int CacheDecrCount( std::string key);
int CacheIncrCount( std::string key);
int CacheGetCount( std::string key, int64_t &count);
int CacheSetCount( std::string key, int64_t count);


// 全局变量用于存储kvstore连接的套接字
int kvstore_sockfd = -1;

// 建立与kvstore的长连接
bool connect_to_kvstore() {
    // 假设kvstore运行在本地的8888端口，可根据实际情况修改
    const char* kvstore_ip = "127.0.0.1";
    int kvstore_port = 9096;

    // 创建套接字
    kvstore_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (kvstore_sockfd < 0) {
        LogError("Failed to create socket");
        return false;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(kvstore_port);
    if (inet_pton(AF_INET, kvstore_ip, &server_addr.sin_addr) <= 0) {
        LogError("Invalid address or address not supported");
        close(kvstore_sockfd);
        kvstore_sockfd = -1;
        return false;
    }

    // 连接到kvstore服务器
    if (connect(kvstore_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LogError("Connection failed");
        close(kvstore_sockfd);
        kvstore_sockfd = -1;
        return false;
    }

    return true;
}

// 去除字符串末尾的换行符
std::string removeTrailingNewline(const std::string& str) {
    std::string result = str;
    // 从字符串末尾开始查找换行符
    result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
        return ch != '\n' && ch != '\r';
    }).base(), result.end());
    return result;
}

// 与kvstore交互的函数
// int send_kvstore_command(const char* command, char* response, size_t response_size) {
//     std::cout << command << std::endl;

//     if (kvstore_sockfd == -1) {
//         LogError("Not connected to kvstore");
//         return -1;
//     }

//     //command_str = removeTrailingNewline(command_str);
//     // 确保命令以换行符结尾
//     std::string command_str = command;
//     if (command_str.back() != '\n') {
//         command_str += '\n';
//     }

//     // 发送命令
//     ssize_t send_len = send(kvstore_sockfd, command_str.c_str(), command_str.length(), 0);
//     if (send_len < 0) {
//         LogError("Failed to send command");
//         perror("send"); // 输出详细的错误信息
//         return -1;
//     } else if (send_len != static_cast<ssize_t>(command_str.length())) {
//         LogError("Partial command sent");
//         return -1;
//     }

//     // 接收响应
//     ssize_t recv_len = recv(kvstore_sockfd, response, response_size - 1, 0);
//     if (recv_len < 0) {
//         LogError("Failed to receive response");
//         perror("recv"); // 输出详细的错误信息
//         return -1;
//     } else if (recv_len == 0) {
//         LogError("Connection closed by peer");
//         return -1;
//     }
//     response[recv_len] = '\0';

//     std::cout << response << std::endl;

//     return 0;
// }
// 与kvstore交互的函数
int send_kvstore_command(const char* command, char* response, size_t response_size) {
    std::cout << "Sending command: " << command << std::endl;

    if (kvstore_sockfd == -1) {
        LogError("Not connected to kvstore");
        return -1;
    }

    // 确保命令以换行符结尾
    std::string command_str = command;
    if (command_str.back() != '\n') {
        command_str += '\n';
    }

    // 发送命令
    ssize_t total_sent = 0;
    while (total_sent < static_cast<ssize_t>(command_str.length())) {
        ssize_t sent = send(kvstore_sockfd, command_str.c_str() + total_sent, command_str.length() - total_sent, 0);
        if (sent < 0) {
            LogError("Failed to send command");
            perror("send");
            return -1;
        }
        total_sent += sent;
    }

    // 接收响应
    ssize_t recv_len = recv(kvstore_sockfd, response, response_size - 1, 0);
    if (recv_len < 0) {
        LogError("Failed to receive response");
        perror("recv"); // 输出详细的错误信息
        return -1;
    } else if (recv_len == 0) {
        LogError("Connection closed by peer");
        return -1;
    }
    response[recv_len] = '\0';

    std::cout << "Received response: " << response << std::endl;

    return 0;
}

// 关闭与kvstore的连接
void close_kvstore_connection() {
    if (kvstore_sockfd != -1) {
        close(kvstore_sockfd);
        kvstore_sockfd = -1;
    }
}

void http_callback(void *callback_data, uint8_t msg, uint32_t handle,
                   void *pParam) {
    UNUSED(callback_data);
    UNUSED(pParam);
    if (msg == NETLIB_MSG_CONNECT) {
        // 这里是不是觉得很奇怪,为什么new了对象却没有释放?
        // 实际上对象在被Close时使用delete this的方式释放自己
        CHttpConn *pConn = new CHttpConn();
        pConn->OnConnect(handle);
    } else {
        LogError("!!!error msg:{}", msg);
    }
}

void http_loop_callback(void *callback_data, uint8_t msg, uint32_t handle,
                        void *pParam) {
    UNUSED(callback_data);
    UNUSED(msg);
    UNUSED(handle);
    UNUSED(pParam);
    CHttpConn::SendResponseDataList(); // 静态函数, 将要发送的数据循环发给客户端
}

int initHttpConn(uint32_t thread_num) {
    g_thread_pool.Init(thread_num); // 初始化线程数量
    g_thread_pool.Start();          // 启动多线程
    netlib_add_loop(http_loop_callback,NULL); // http_loop_callback被epoll所在线程循环调用
    return 0;
}

const char SPECIAL_CHAR = '#';

// 为字符串添加特殊字符
std::string addSpecialChar(const std::string& str) {
    return str + SPECIAL_CHAR;
}

// 去除字符串末尾的特殊字符
std::string removeSpecialChar(const std::string& str) {
    if (!str.empty() && str.back() == SPECIAL_CHAR) {
        return str.substr(0, str.length() - 1);
    }
    return str;
}

//mysql存储的部分信息加载kvstore缓存
int ApiInit() {
    if (!connect_to_kvstore()) {
        LogError("Failed to connect to kvstore");
        return -1;
    }

    CDBManager *db_manager = CDBManager::getInstance();
    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
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
    ret = CacheSetCount("TABLE_USER_COUNT", (int64_t)count);
    if (ret < 0) {
        LogError("CacheSetCount failed");
        close_kvstore_connection();
        return -1;
    }

    // 对于个人的文件数量 和图片分享数量，在登录的时候 读取到kvstore

    close_kvstore_connection();
    return 0;
}


int CacheSetCount( std::string key, int64_t count) {
    char command[1024];
    std::string keyWithSpecial = addSpecialChar(key);
    std::string valueStr = std::to_string(count);
    std::string valueWithSpecial = addSpecialChar(valueStr);
    // 确保命令格式正确，使用 # 作为分隔符
    int command_length = snprintf(command, sizeof(command), "RSET#%s%s\n", keyWithSpecial.c_str(), valueWithSpecial.c_str());
    if (command_length < 0 || command_length >= sizeof(command)) {
        LogError("Command buffer overflow in CacheSetCount");
        return -1;
    }
    char response[1024];
    int ret = send_kvstore_command(command, response, sizeof(response));
    if (ret == 0) {
        std::string trimmedResponse = removeSpecialChar(std::string(response));
        if (trimmedResponse == "OK\r\n") {
            return 0;
        }
    }
    LogError("Failed to set count in CacheSetCount. Response: %s", response);
    return -1;
}

int CacheGetCount( std::string key, int64_t &count) {
    count = 0;
    char command[1024];
    std::string keyWithSpecial = addSpecialChar(key);
    // 确保命令格式正确，使用 # 作为分隔符
    int command_length = snprintf(command, sizeof(command), "RGET#%s\n", keyWithSpecial.c_str());
    if (command_length < 0 || command_length >= sizeof(command)) {
        LogError("Command buffer overflow in CacheGetCount");
        return -1;
    }
    char response[1024];
    int ret = send_kvstore_command(command, response, sizeof(response));
    if (ret == 0 && response[0] != '\0') {
        std::string trimmedResponse = removeSpecialChar(std::string(response));
        if (trimmedResponse == "NO EXIST") {
            return -1;
        }
        count = std::stoll(trimmedResponse);
        return 0;
    } else {
        LogError("Failed to get count in CacheGetCount. Response: %s", response);
        return -1;
    }
}

// 键值递增
int CacheIncrCount( std::string key) {
    // 先使用 GET 命令获取当前值
    char get_command[1024];
    std::string keyWithSpecial = addSpecialChar(key);
    // 确保命令格式正确，使用 # 作为分隔符
    int get_command_length = snprintf(get_command, sizeof(get_command), "RGET#%s\n", keyWithSpecial.c_str());
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
        std::string trimmedResponse = removeSpecialChar(std::string(get_response));
        if (trimmedResponse == "NO EXIST") {
            count = 0;
        } else {
            count = std::stoll(trimmedResponse);
        }
    }

    // 对值进行加一操作
    count++;

    // 使用 SET 命令将新值存回
    char set_command[1024];
    std::string valueStr = std::to_string(count);
    std::string valueWithSpecial = addSpecialChar(valueStr);
    // 确保命令格式正确，使用 # 作为分隔符
    int set_command_length = snprintf(set_command, sizeof(set_command), "RSET#%s%s\n", keyWithSpecial.c_str(), valueWithSpecial.c_str());
    if (set_command_length < 0 || set_command_length >= sizeof(set_command)) {
        LogError("Command buffer overflow in CacheIncrCount (SET)");
        return -1;
    }
    char set_response[1024];
    int set_ret = send_kvstore_command(set_command, set_response, sizeof(set_response));
    if (set_ret == 0) {
        std::string trimmedResponse = removeSpecialChar(std::string(set_response));
        if (trimmedResponse == "\r\n") {
            LogInfo("{}-{}", key, count);
            return 0;
        }
    }
    LogError("Failed to set count in CacheIncrCount. Response: %s", set_response);
    return -1;
}

// 键值递减
int CacheDecrCount( std::string key) {
    // 先使用 GET 命令获取当前值
    char get_command[1024];
    std::string keyWithSpecial = addSpecialChar(key);
    // 确保命令格式正确，使用 # 作为分隔符
    int get_command_length = snprintf(get_command, sizeof(get_command), "RGET#%s\n", keyWithSpecial.c_str());
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
        std::string trimmedResponse = removeSpecialChar(std::string(get_response));
        if (trimmedResponse == "NO EXIST") {
            count = 0;
        } else {
            count = std::stoll(trimmedResponse);
        }
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
    std::string valueStr = std::to_string(count);
    std::string valueWithSpecial = addSpecialChar(valueStr);
    // 确保命令格式正确，使用 # 作为分隔符
    int set_command_length = snprintf(set_command, sizeof(set_command), "RSET#%s%s\n", keyWithSpecial.c_str(), valueWithSpecial.c_str());
    if (set_command_length < 0 || set_command_length >= sizeof(set_command)) {
        LogError("Command buffer overflow in CacheDecrCount (SET)");
        return -1;
    }
    char set_response[1024];
    int set_ret = send_kvstore_command(set_command, set_response, sizeof(set_response));
    if (set_ret == 0) {
        std::string trimmedResponse = removeSpecialChar(std::string(set_response));
        if (trimmedResponse == "\r\n") {
            LogInfo("{}-{}", key, count);
            return 0;
        }
    }
    LogError("Failed to set count in CacheDecrCount. Response: %s", set_response);
    return -1;
}

// 与kvstore交互的Get函数
std::string GetFromKvstore(const std::string& key) {
    char command[1024];
    std::string keyWithSpecial = addSpecialChar(key);
    // 确保命令格式正确，使用 # 作为分隔符
    snprintf(command, sizeof(command), "RGET#%s\n", keyWithSpecial.c_str());
    char response[1024];
    int ret = send_kvstore_command(command, response, sizeof(response));
    if (ret == 0 && response[0] != '\0') {
        std::string trimmedResponse = removeSpecialChar(std::string(response));
        if (trimmedResponse == "NO EXIST") {
            return "";
        }
        return trimmedResponse;
    }
    return "";
}

// 去除字符串末尾的空白字符
std::string trimRight(const std::string& str) {
    auto it = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
    return std::string(str.begin(), it.base());
}

int main(int argc, char *argv[]) {
    // 默认情况下，往一个读端关闭的管道或socket连接中写数据将引发SIGPIPE信号。我们需要在代码中捕获并处理该信号，
    // 或者至少忽略它，因为程序接收到SIGPIPE信号的默认行为是结束进程，而我们绝对不希望因为错误的写操作而导致程序退出。
    // SIG_IGN 忽略信号的处理程序
    signal(SIGPIPE, SIG_IGN);
    int ret = 0;
    // 获取配置文件路径
    char *str_qs_http_server_conf = NULL;
    if (argc > 1) {
        str_qs_http_server_conf = argv[1];
    } else {
        str_qs_http_server_conf = (char *)"/home/jerry/Desktop/questionnare/questionnare-src/qs_http_server.conf";
    }
    printf("conf file path: %s\n", str_qs_http_server_conf);

    // 解析配置文件,修改源文件中类中不存在的默认构造函数
    CConfigFileReader config_file(str_qs_http_server_conf);
    char *log_level = config_file.GetConfigName("log_level");   // 读取日志设置级别
    DLog::SetLevel(log_level);   // 设置日志打印级别

    char *http_listen_ip = config_file.GetConfigName("HttpListenIP");
    printf("%s\n", http_listen_ip);
    char *str_http_port = config_file.GetConfigName("HttpPort");        // 8081 -- nginx.conf,当前服务的端口

    char *str_thread_num = config_file.GetConfigName("ThreadNum");  // 线程池数量，目前是epoll + 线程池方式
    uint32_t thread_num = atoi(str_thread_num);

    LogInfo("main into"); // 单例模式 日志库 spdlog

    // 删除Redis连接池相关代码

    CDBManager::SetConfPath(str_qs_http_server_conf);   // 设置配置文件路径
    CDBManager *db_manager = CDBManager::getInstance();
    if (!db_manager) {
        LogError("DBManager init failed");
        return -1;
    }

    // 建立与kvstore的长连接
    if (!connect_to_kvstore()) {
        LogError("Failed to connect to kvstore");
        return -1;
    }

    // 检测监听ip和端口
    if (!http_listen_ip || !str_http_port) {
        LogError("config item missing, exit... ip:{}, port:{}", http_listen_ip,
                 str_http_port);
        close_kvstore_connection();
        return -1;
    }
    // reactor网络模型 1epoll+ 线程池
    ret = netlib_init();
    if (ret == NETLIB_ERROR) {
        LogError("netlib_init failed");
        close_kvstore_connection();
        return ret;
    }

    uint16_t http_port = atoi(str_http_port);
    CStrExplode http_listen_ip_list(http_listen_ip, ';');
    for (uint32_t i = 0; i < http_listen_ip_list.GetItemCnt(); i++) {
        ret = netlib_listen(http_listen_ip_list.GetItem(i), http_port,
                            http_callback, NULL);
        if (ret == NETLIB_ERROR) {
            close_kvstore_connection();
            return ret;
        }
    }
    initHttpConn(thread_num);

    LogInfo("server start listen on:For http://{}:{}", http_listen_ip, http_port);

    LogInfo("now enter the event loop...");

    WritePid();

    // 超时参数影响回发客户端的时间
    netlib_eventloop(1);

    // 关闭与kvstore的连接
    close_kvstore_connection();

    return 0;

}