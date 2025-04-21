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
int send_kvstore_command(const char* command, char* response, size_t response_size) {
    std::cout << command << std::endl;

    if (kvstore_sockfd == -1) {
        LogError("Not connected to kvstore");
        return -1;
    }

    // 去除请求字符串末尾的换行符
    std::string command_str = command;
    command_str = removeTrailingNewline(command_str);

    // 发送命令
    ssize_t send_len = send(kvstore_sockfd, command_str.c_str(), command_str.length(), 0);
    if (send_len < 0) {
        LogError("Failed to send command");
        perror("send"); // 输出详细的错误信息
        return -1;
    } else if (send_len != static_cast<ssize_t>(command_str.length())) {
        LogError("Partial command sent");
        return -1;
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

    std::cout << response << std::endl;

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