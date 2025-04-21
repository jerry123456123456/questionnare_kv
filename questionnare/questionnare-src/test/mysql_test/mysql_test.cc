//source /home/jerry/Desktop/questionnare/questionnare-src/test/mysql_test/test.sql
#include"db_pool.h"
#include"tc_common.h"
#include<iostream>

int main(){
    char *str_qs_http_server_conf = (char *)"/home/jerry/Desktop/questionnare/questionnare-src/qs_http_server.conf";
    CDBManager::SetConfPath(str_qs_http_server_conf);   //设置配置文件路径
    CDBManager *db_manager = CDBManager::getInstance();  //里面调用了Init()
    if (!db_manager) {
        LogError("DBManager init failed");
        return -1;
    }

    CDBConn *db_conn = db_manager->GetDBConn("qs_slave");
    AUTO_REL_DBCONN(db_manager, db_conn);  //自动归还连接

    char sql_cmd[SQL_MAX_LEN] = {0};
    sprintf(sql_cmd,"INSERT INTO Users (username, password, is_root) VALUES ('admin', 'password123', 1)");

    LogInfo("执行: {}", sql_cmd);
    if (!db_conn->ExecuteCreate_test(sql_cmd)) //执行sql语句
    {
        LogError("{} 操作失败", sql_cmd);
        return -1;
    }

    return 0;
}

/*执行成功的结果
root@k8s-master1:/home/jerry/Desktop/questionnare/questionnare-src/test/mysql_test/build# ./mysql_test 
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

[2024-10-27 00:29:07.982][thread 137197][/home/jerry/Desktop/questionnare/questionnare-src/mysql/db_pool.cc:569,Init][info] : db_host:localhost, db_port:3306, db_dbname:survey_system, db_username:root, db_password:123456
WARNING: MYSQL_OPT_RECONNECT is deprecated and will be removed in a future version.
[2024-10-27 00:29:07.992][thread 137197][/home/jerry/Desktop/questionnare/questionnare-src/mysql/db_pool.cc:569,Init][info] : db_host:localhost, db_port:3306, db_dbname:survey_system, db_username:root, db_password:123456
WARNING: MYSQL_OPT_RECONNECT is deprecated and will be removed in a future version.
[2024-10-27 00:29:07.998][thread 137197][/home/jerry/Desktop/questionnare/questionnare-src/test/mysql_test/mysql_test.cc:20,main][info] : 执行: INSERT INTO users (username, password, is_root) VALUES ('admin', 'password123', 1)
[2024-10-27 00:29:08.050][thread 137197][/home/jerry/Desktop/questionnare/questionnare-src/mysql/db_pool.cc:258,ExecuteCreate_test][info] : INSERT操作成功，影响了 1 行
*/