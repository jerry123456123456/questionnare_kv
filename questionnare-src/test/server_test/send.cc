#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define URL "http://192.168.186.138/api/login"
//#define URL "http://192.168.186.138:8081"

// 要发送的 JSON 数据
const char *jsonData = "{"
    "\"user_name\": \"jerry\","
    "\"table_name\": \"love\","
    "\"questions\": ["
    "    {\"question_text\": \"问题 1\", \"answer\": \"B\"},"
    "    {\"question_text\": \"问题 2\", \"answer\": \"B\"},"
    "    {\"question_text\": \"问题 3\", \"answer\": \"A\"}"
    "]"
"}";

//const char *jsonData = "{\"user_name\":\"jerry\"}";

int main(void) {
    CURL *curl;
    CURLcode res;

    // 初始化 libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        // 设置 URL
        curl_easy_setopt(curl, CURLOPT_URL, URL);

        // 设置 HTTP POST 方法
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // 设置请求的内容类型为 JSON
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(NULL, "Content-Type: application/json"));

        // 设置要发送的数据
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData);

        // 发送请求
        res = curl_easy_perform(curl);

        // 检查请求是否成功
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("请求发送成功！\n");
        }

        // 清理
        curl_easy_cleanup(curl);
    }

    // 关闭 libcurl
    curl_global_cleanup();

    return 0;
}
