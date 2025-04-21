cmake_minimum_required(VERSION 3.0)
project(redis_test)

AUX_SOURCE_DIRECTORY(/home/jerry/Desktop/questionnare/questionnare-src/base BASE_LIST)
AUX_SOURCE_DIRECTORY(/home/jerry/Desktop/questionnare/questionnare-src/mysql MYSQL_LIST)
AUX_SOURCE_DIRECTORY(/home/jerry/Desktop/questionnare/questionnare-src/redis REDIS_LIST)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)

#判断编译器类型,如果是gcc编译器,则在编译选项中加入c++14支持
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-std=c++14 ${CMAKE_CXX_FLAGS}")
    message(STATUS "optional:-std=c++14")   
endif(CMAKE_COMPILER_IS_GNUCXX)

add_compile_options(-w)
# 如果要开启警告使用 -Wall
# -Wno-deprecated选项用于关闭编译器产生的关于使用已经被废弃（deprecated）的函数、变量或头文件时产生的警告信息
# -Wno-unused-parameter选项则用于关闭编译器产生的有关未使用函数参数的警告信息。
ADD_DEFINITIONS(-g -Wno-deprecated -Wno-unused-parameter -Bsymbolic -D_REENTRANT -D_FILE_OFFSET_BITS=64
-DAC_HAS_INFO -DAC_HAS_WARNING -DAC_HAS_ERROR -DAC_HAS_CRITICAL -DTIXML_USE_STL
-DAC_HAS_DEBUG -DLINUX_DAEMON)

# 添加库文件路径，部分Linux版本不自动加载/usr/local/lib
LINK_DIRECTORIES(/usr/local/lib)

# 添加头文件路径
INCLUDE_DIRECTORIES(/usr/local/include/hiredis /usr/include/mysql)
include_directories(/usr/local/include)  # 根据你的安装路径调整
include_directories(/home/jerry/Desktop/questionnare/questionnare-src/base)
include_directories(/home/jerry/Desktop/questionnare/questionnare-src/mysql)
include_directories(/home/jerry/Desktop/questionnare/questionnare-src/redis)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 添加需要的库
find_package(Threads REQUIRED)  # 如果需要多线程支持
find_package(fmt REQUIRED)       # fmt 库
find_package(spdlog REQUIRED)   # spdlog 库

# 添加可执行文件
add_executable(redis_test ${SRC_LIST} ${BASE_LIST} ${MYSQL_LIST} ${REDIS_LIST})

# 链接库
target_link_libraries(redis_test PRIVATE mysqlclient fmt::fmt spdlog::spdlog ${CMAKE_THREAD_LIBS_INIT})

# 如果有其他库，比如你的 CDBManager 和 DLog 相关的实现文件，记得在这里添加它们
# target_sources(mysql_test PRIVATE your_other_sources.cpp)
