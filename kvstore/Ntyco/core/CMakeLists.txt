#aux_source_directory 是 CMake 中的一个命令，其作用是将指定目录下的所有源文件（.c、.cpp 等）的文件名收集起来，存储到一个指定的变量中
aux_source_directory(. SRC)
#add_library 是 CMake 中的一个命令，用于创建一个库文件
add_library(nty_core ${SRC})