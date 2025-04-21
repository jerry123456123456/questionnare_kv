#include"tc_common.h"

#include "config_file_reader.h"
#include<iostream>

bool UserLogin(CacheManager *cacheManager,const string &username,const string &password){
    CacheConn *conn  = cacheManager->GetCacheConn("token");
    if (!conn) {
        std::cerr << "无法获取 Redis 连接!" << std::endl;
        return false;
    }

    AUTO_REL_CACHECONN(cacheManager, conn); // 确保连接在使用后被释放

    //检查用户是否存在
    string stored_password = conn->Get("user:" + username);
    if (stored_password.empty()) {
        std::cout << "用户不存在，请注册。" << std::endl;
        return false;
    }

    // 验证密码
    if (stored_password == password) {
        std::cout << "登录成功！" << std::endl;
        return true;
    } else {
        std::cout << "密码错误！" << std::endl;
        return false;
    }
}

// 用户注册示例
bool UserRegister(CacheManager *cacheManager, const string &username, const string &password) {
    //从连接池中获取连接
    CacheConn *conn = cacheManager->GetCacheConn("token");
    if (!conn) {
        std::cerr << "无法获取 Redis 连接!" << std::endl;
        return false;
    }

    AUTO_REL_CACHECONN(cacheManager, conn); // 确保连接在使用后被释放

    if(!conn->Get("user:" + username).empty()){
        std::cout << "用户名已经存在，请选择其他用户名" << std::endl;
    }

    //插入用户信息到redis
    conn->Set("user:" + username, password);
    std::cout << "注册成功"<< std::endl;
    return true;
}


int main(){
    char *str_qs_http_server_conf = (char *)"/home/jerry/Desktop/questionnare/questionnare-src/qs_http_server.conf";

    CacheManager::SetConfPath(str_qs_http_server_conf); //设置配置文件路径
    CacheManager *cache_manager = CacheManager::getInstance();
    if (!cache_manager) {
        LogError("CacheManager init failed");
        return -1;
    }
    UserRegister(cache_manager,"test_user","123456");

    UserLogin(cache_manager,"test_user","12345");
    UserLogin(cache_manager,"test_user","123456");

    return 0;
}

/*执行结果
root@k8s-master1:/home/jerry/Desktop/questionnare/questionnare-src/test/redis_test/build# ./redis_test 
Map contents:
Key: CacheInstances, Value: token,ranking_list
Key: DBInstances, Value: qs_master,qs_slave
Key: HttpListenIP, Value: 0.0.0.0
Key: HttpPort, Value: 8081
Key: ThreadNum, Value: 8
Key: log_level, Value: info
Key: qs_master_dbname, Value: survey_system
Key: qs_master_host, Value: localhost
Key: qs_master_maxconncnt, Value: 8
Key: qs_master_password, Value: 123456
Key: qs_master_port, Value: 3306
Key: qs_master_username, Value: root
Key: qs_slave_dbname, Value: survey_system
Key: qs_slave_host, Value: localhost
Key: qs_slave_maxconncnt, Value: 8
Key: qs_slave_password, Value: 123456
Key: qs_slave_port, Value: 3306
Key: qs_slave_username, Value: root
Key: ranking_list_db, Value: 1
Key: ranking_list_host, Value: 127.0.0.1
Key: ranking_list_maxconncnt, Value: 8
Key: ranking_list_port, Value: 6379
Key: token_db, Value: 0
Key: token_host, Value: 127.0.0.1
Key: token_maxconncnt, Value: 8
Key: token_port, Value: 6379
cache pool: token, list size: 2
cache pool: ranking_list, list size: 2
注册成功
密码错误！
登录成功！
root@k8s-master1:/home/jerry/Desktop/questionnare/questionnare-src/test/redis_test/build# 
*/