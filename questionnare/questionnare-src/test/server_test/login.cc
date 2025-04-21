// // g++ -g -I/home/jerry/Documents/tuchuang/tc-src/jsoncpp/ server.cc -o server -ljsoncpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include "jsoncpp/json/json.h"
#include "jsoncpp/json/reader.h"
#include "jsoncpp/json/value.h"

#define PORT 8081
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // 创建套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return -1;
    }

    // 设置地址和端口重用选项
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定套接字到指定地址和端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return -1;
    }

    // 监听连接
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        return -1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        // 接受客户端连接
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        // 接收数据
        int valread = read(new_socket, buffer, BUFFER_SIZE);

        if (valread <= 0) {
            std::cout<<"111"<<std::endl;
            close(new_socket);
            continue;
        }

        // 处理OPTIONS请求
        if (strncmp(buffer, "OPTIONS", 7) == 0) {
            const char* response = "HTTP/1.1 204 No Content\r\n"
                                   "Access-Control-Allow-Origin: *\r\n"
                                   "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                                   "Access-Control-Allow-Headers: Content-Type\r\n"
                                   "Connection: close\r\n\r\n";
            send(new_socket, response, strlen(response), 0);
            close(new_socket);
            continue;  // 继续等待下一个连接
        }

        // 检查是否为 POST 请求
        if (strncmp(buffer, "POST", 4) != 0) {
            std::cerr << "Not a POST request, ignoring..." << std::endl;
            close(new_socket);
            continue;
        }

        // 解析JSON数据
        Json::Value root;
        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;

        //到这里要先去掉头部信息
        char *body = strstr(buffer, "\r\n\r\n");
        if (body != NULL) {
            body += 4;  // 跳过 "\r\n\r\n"
        }   
        
        // 创建一个 std::istringstream 对象，确保可以被引用
        std::istringstream stream(body);
        bool parsingSuccessful = Json::parseFromStream(builder, stream, &root, &errs);
        if (parsingSuccessful) {
            std::cout << "Parsed JSON data:" << std::endl;
            std::cout << "user: " << root["user"].asString() << std::endl;
            std::cout << "pwd: " << root["pwd"].asString() << std::endl;

            // 构造并发送回发响应数据
            Json::Value response;
            response["code"] = 0;

            // 将响应数据转换为字符串
            Json::StreamWriterBuilder writer;
            std::string responseData = Json::writeString(writer, response);

            // 发送回发响应数据
            send(new_socket, responseData.c_str(), responseData.length(), 0);
        } else {
            std::cerr << "Failed to parse JSON data: " << errs << std::endl;

            // 构造错误响应数据
            Json::Value response;
            response["code"] = 1;

            // 将响应数据转换为字符串
            Json::StreamWriterBuilder writer;
            std::string responseData = Json::writeString(writer, response);

            // 发送回发响应数据
            send(new_socket, responseData.c_str(), responseData.length(), 0);
        }

        // 关闭连接
        close(new_socket);
    }

    return 0;
}

