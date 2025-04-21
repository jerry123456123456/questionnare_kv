//这段代码定义了一些实用工具类和函数，涉及引用计数、字符串操作、时间处理等功能，且支持跨平台的实现
#ifndef __UTIL_H__
#define __UTIL_H__

//关闭安全检查警告
#include <cstdint>
#include <sys/types.h>
#define _CRT_SECURE_NO_DEPRECATE

#include"lock.h"
#include"ostype.h"
#include"util_pdu.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#ifndef _WIN32
#include <strings.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
#define snprintf sprintf_s
#else
#include <pthread.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#endif
//消除未使用的参数警告，通常在函数定义中为了兼容接口而保留参数但不使用时，可以用这个宏来消除编译器的警告
#define NOTUSED_ARG(v) ((void)v)

//用于实现引用计数管理的基类，引用计数用于控制对象的生命周期，当引用计数位0，对象会被释放
class CRefObject{
public:
    CRefObject();
    virtual ~CRefObject();

    void SetLock(CLock *lock){lock_=lock;}  //设置锁对象，用于线程间安全的引用计数管理
    void AddRef();  //增加引用计数
    void ReleaseRef(); //减少引用计数
private:
    int ref_count_;
    CLock *lock_;
};

uint64_t GetTickCount(); //返回从系统启动到现在经过的时间，单位是毫秒
void util_sleep(uint32_t millisecond);  //使当前线程睡眠指定的毫秒数，相当于在程序中实现延迟的功能

//用于字符串分割操作，将字符串按照给定的分隔符分割成多个子字符串
class CStrExplode{
public:
    CStrExplode(char *str,char seperator);  //接受一个字符串和分隔符，并将字符串，拆分为多个字符串
    virtual ~CStrExplode();

    uint32_t GetItemCnt(){return item_cnt_;}  //回分割出的字符串数量
    char *GetItem(uint32_t idx){return item_list_[idx];}  //根据索引返回指定的子字符串
private:    
    uint32_t item_cnt_;
    char **item_list_;
};

//替换字符串中的某个字符，将字符串src中的old_char替换为new_char
char *ReplaceStr(char *src,char old_char,char new_char);

//互相转换
string Int2String(uint32_t user_id);
uint32_t String2Int(const string &value);

//将字符串中的某个占位符替换为给定的值，有两个重载版本
void ReplaceMark(string &str,string &new_value,string &begin_pos);
void ReplaceMark(string &str,uint32_t new_value,uint32_t &begin_pos);

//将当前进程的pid写入文件，通常用于服务程序记录当前运行的进程号，以便后续管理进程，如停止或重启
void WritePid();

//用于字符和十六进制之间的转换
inline unsigned char ToHex(const unsigned char &x);
inline unsigned char FromHex(const unsigned char &x);

//将字符串中的特殊字符转换为 URL 安全的形式
string URLEncode(const string &sIn);
//将 URL 中的编码字符转换为原始形式
string URLDecode(const string &sIn);

//获取指定文件的大小，返回文件的字节数，如果不存在，返回负值
int64_t GetFileSize(const char *path);

//在指定的内存块src_str中查找子字符串sub_str。该函数可以查找子字符串在一段内存数据中的位置
const char *MemFind(const char *src_str,size_t src_len,const char *sub_str,size_t sub_len,bool flag=true);

#endif