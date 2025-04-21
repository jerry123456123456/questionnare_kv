# 定义编译器为 gcc
CC = gcc
# 定义一个变量 ECHO 为 echo 命令，可用于输出信息
ECHO = echo

# 定义子目录为 core/，表示项目中的核心代码目录
SUB_DIR = core/
# 定义示例代码目录为 sample/
SAMPLE_DIR = sample/
# 使用 shell 命令 pwd 获取当前工作目录，并赋值给 ROOT_DIR 变量
ROOT_DIR = $(shell pwd)
# 定义目标文件存放目录为项目根目录下的 objs 目录
OBJS_DIR = $(ROOT_DIR)/objs
# 定义可执行文件存放目录为项目根目录下的 bin 目录
BIN_DIR = $(ROOT_DIR)/bin

# 定义一系列可执行文件的名称，这些文件将在编译过程中生成
BIN = nty_server nty_client nty_bench nty_server_mulcore hook_tcpserver nty_http_server nty_mysql_client nty_mysql_oper nty_websocket_server nty_http_server_mulcore ntyco_httpd nty_rediscli

# 定义静态库的名称为 libntyco.a
LIB = libntyco.a

# 定义编译时的通用选项
# -lpthread 表示链接 pthread 库，用于支持多线程编程
# -O3 表示开启最高级别的优化
# -ldl 表示链接动态链接库
# -I $(ROOT_DIR)/core 表示将项目根目录下的 core 目录添加到头文件搜索路径中
FLAG = -lpthread -O3 -ldl -I $(ROOT_DIR)/core  -I /usr/include/tirpc

# 定义编译某些目标时需要的额外选项，主要用于链接第三方库
# -lcrypto -lssl 用于链接加密和 SSL 库
# -lmysqlclient 用于链接 MySQL 客户端库
# -lhiredis 用于链接 Redis 客户端库
# -I /usr/include/mysql/ -I /usr/local/include/hiredis/ 分别将 MySQL 和 Redis 的头文件目录添加到搜索路径中
THIRDFLAG = -lcrypto -lssl -lmysqlclient -lhiredis -I /usr/include/mysql/ -I /usr/local/include/hiredis/

# 使用 wildcard 函数获取当前目录下所有的 .c 文件，并赋值给 CUR_SOURCE 变量
CUR_SOURCE = ${wildcard *.c}
# 使用 patsubst 函数将 CUR_SOURCE 中的 .c 文件替换为 .o 文件，并赋值给 CUR_OBJS 变量
CUR_OBJS = ${patsubst %.c, %.o, $(CUR_SOURCE)}

# 导出一些变量，使得这些变量在子 make 进程中也可以使用
export CC BIN_DIR OBJS_DIR ROOT_IDR FLAG BIN ECHO EFLAG

# 定义默认目标 all，它依赖于 check_objs、check_bin、$(SUB_DIR) 和 $(LIB)
# check_objs 用于检查 objs 目录是否存在，不存在则创建
# check_bin 用于检查 bin 目录是否存在，不存在则创建
# $(SUB_DIR) 表示进入 core 子目录执行 make 操作
# $(LIB) 表示生成静态库 libntyco.a
all : check_objs check_bin $(SUB_DIR) $(LIB)
# .PHONY 用于声明 all 为伪目标，即使存在名为 all 的文件，make 也会执行相应的规则
.PHONY : all

# 定义目标 bin，它依赖于 $(SAMPLE_DIR) 和 $(BIN)
# $(SAMPLE_DIR) 表示进入 sample 子目录执行 make 操作
# $(BIN) 表示生成所有的可执行文件
bin: $(SAMPLE_DIR) $(BIN)

# 定义 $(SUB_DIR) 目标，它依赖于 ECHO
# 执行 make -C $@ 命令，即进入 core 子目录执行 make 操作
# $@ 表示当前目标，即 core/
$(SUB_DIR) : ECHO
	make -C $@

# 定义 ECHO 目标，用于输出 SUB_DIR 的值
ECHO :
	@echo $(SUB_DIR)

# 定义 check_objs 目标，用于检查 objs 目录是否存在
# 如果不存在，则使用 mkdir -p 命令创建该目录
check_objs:
	if [ ! -d "objs" ]; then \
		mkdir -p objs;  \
	fi

# 定义 check_bin 目标，用于检查 bin 目录是否存在
# 如果不存在，则使用 mkdir -p 命令创建该目录
check_bin:
	if [ ! -d "bin" ]; then \
		mkdir -p bin;   \
	fi

# 定义 nty_server 目标，它依赖于多个目标文件和 nty_server.c 文件
# 使用 $(CC) 编译器将这些文件编译成可执行文件，并输出到 $(BIN_DIR)/nty_server
# $^ 表示所有的依赖文件
# $(FLAG) 表示编译选项
nty_server : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_server.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

# 定义 nty_client 目标，编译规则与 nty_server 类似
nty_client : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_client.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

# 定义 nty_bench 目标，编译规则与 nty_server 类似
nty_bench : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_bench.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

# 定义 nty_server_mulcore 目标，编译规则与 nty_server 类似
nty_server_mulcore : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_server_mulcore.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

# 定义 nty_http_server 目标，编译规则与 nty_server 类似
nty_http_server : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_http_server.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

# 定义 nty_websocket_server 目标，编译规则与 nty_server 类似，但需要额外的 THIRDFLAG 选项
nty_websocket_server : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_websocket_server.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(THIRDFLAG)

# 定义 nty_mysql_client 目标，编译规则与 nty_server 类似，但需要额外的 THIRDFLAG 选项
nty_mysql_client : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_mysql_client.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(THIRDFLAG)

# 定义 nty_mysql_oper 目标，编译规则与 nty_server 类似，但需要额外的 THIRDFLAG 选项
nty_mysql_oper : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_mysql_oper.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(THIRDFLAG)

# 定义 nty_rediscli 目标，编译规则与 nty_server 类似，但需要额外的 THIRDFLAG 选项
nty_rediscli : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_rediscli.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(THIRDFLAG)

# 定义 hook_tcpserver 目标，编译规则与 nty_server 类似
hook_tcpserver: $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/hook_tcpserver.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG)

# 定义 nty_http_server_mulcore 目标，编译规则与 nty_server 类似
nty_http_server_mulcore : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_http_server_mulcore.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

# 定义 ntyco_httpd 目标，编译规则与 nty_server 类似
ntyco_httpd : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/ntyco_httpd.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

# 定义 libntyco.a 目标，它依赖于多个目标文件
# 使用 ar 命令将这些目标文件打包成静态库 libntyco.a
# $@ 表示当前目标，即 libntyco.a
# $^ 表示所有的依赖文件
libntyco.a : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o
	ar rcs $@ $^

# 定义 clean 目标，用于清理生成的文件
# 删除 $(BIN_DIR) 目录下的所有文件、$(OBJS_DIR) 目录下的所有文件以及静态库 libntyco.a
clean :
	rm -rf $(BIN_DIR)/* $(OBJS_DIR)/* libntyco.a