KVStore 项目说明
项目概述
本项目是一个键值存储系统，包含多种数据结构（数组、红黑树、哈希表）的实现，并提供了相应的测试用例。
编译项目
在项目根目录下，使用 make 命令进行编译：
make

该命令会完成以下操作：
编译 Ntyco 库。
编译 kvstore 主程序。
编译 testcase 测试程序。
编译成功后，会生成两个可执行文件：kvstore 和 testcase。
运行主程序
kvstore 是键值存储系统的核心服务，运行时可以指定监听的端口。例如，要让 kvstore 监听在 9096 端口，可以使用以下命令：
./kvstore 9096
如果不指定端口，程序可能会使用默认端口，具体取决于代码实现。
运行测试程序
testcase 程序用于测试 kvstore 的功能，运行时需要提供三个参数：服务器的 IP 地址、端口号以及测试模式。
测试模式说明
0：执行 rbtree_testcase_1w 测试。
1：执行 rbtree_testcase_3w 测试。
2：执行 array_testcase_1w 测试。
3：执行 hash_testcase 测试。
运行示例
假设 kvstore 运行在 192.168.186.138 的 9096 端口，以下是不同测试模式的运行命令：
执行 rbtree_testcase_1w 测试
./testcase 192.168.186.138 9096 0
执行 rbtree_testcase_3w 测试
./testcase 192.168.186.138 9096 1
执行 array_testcase_1w 测试
./testcase 192.168.186.138 9096 2
执行 hash_testcase 测试
./testcase 192.168.186.138 9096 3
查看测试结果
运行 testcase 程序后，终端会显示测试结果。如果测试通过，会显示 ==> PASS；如果测试失败，则会显示 ==> FAILED 以及具体的错误信息。
