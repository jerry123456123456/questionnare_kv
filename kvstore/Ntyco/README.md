## NtyCo

#### compile

```
编译ntyco的core文件与编译libntyco.a的静态库
$ make

// 编译sample
$ make bin
```

#### err info
```
nty_mysql_oper.c:8:19: fatal error: mysql.h: No such file or directory

解决方案：
# sudo apt-get install libmysqlclient-dev

nty_rediscli.c:11:21: fatal error: hiredis.h: No such file or directory

解决方案：
需要编译安装hiredis: https://github.com/redis/hiredis

```


#### server 
```
$ ./bin/nty_server
```
#### client
```
./bin/nty_client
```

#### mul_process, mul_core
```
$ ./bin/nty_server_mulcore
```
#### websocket
```
$ ./bin/nty_websocket_server
```
#### http server
```
$ ./bin/nty_http_server_mulcore
```

