#### 编译
1.先进入questionnare-src目录，创建build目录，cmake .. &&  make

2.进入kvstore目录的Ntyco目录，然后make；再退回kvstore目录make

#### 准备
1.如果使用mysql，那么要把root密码设为123456，然后启动mysql： systemctl restart mysql，然后执行脚本创建数据库 ： source /home/jerry/Desktop/questionnare/sql/questionnare.sql;  （具体替换为实际的目录）; 再使用命令创建root用户 ： INSERT INTO Users (user_name, password, is_root) VALUES ('root', MD5('123456'), 1);

2.如果使用tidb，要参照questionnare-src/mysql/tidb.png配置，然后同样创建表，并将questionnare/qs_http_server.conf的3306端口改为4000，并用tiup plaground启动tidb

3.参照questionnare/nginx.txt修改ngnix配置，把nginx.conf移到目标文件并生效。

#### 运行
1.启动grpc服务器 python3 image_server.py

2.启动kvstore服务器 ./kvstore 9096

3.启动问卷服务器 ： ./qs_http_server

#### 使用
在本地互联网 http://192.168.186.138/login 即可访问

#### 测试
运行questionnare/test下的python，得到数据。
