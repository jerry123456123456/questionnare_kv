#include"util.h"
#include <cassert>
#include <cstdint>
#include<sstream>

using namespace std;

CRefObject::CRefObject(){
    lock_=NULL;
    ref_count_=1;
}

CRefObject::~CRefObject(){}

void CRefObject::AddRef(){
    if(lock_){
        lock_->lock();
        ref_count_++;
        lock_->unlock();
    }else{   //如果没有锁对象，也就意味着这是在单线程的环境下
        ref_count_++;
    }
}

void CRefObject::ReleaseRef(){
    if(lock_){
        lock_->lock();
        ref_count_--;
        if(ref_count_==0){
            delete this;
            lock_->unlock();      // 这个是源码中我感觉缺少的部分
            return;
        }
        lock_->unlock();
    }else {
        ref_count_--;
        if (ref_count_ == 0)
            delete this;
    }
}

uint64_t GetTickCount(){
#ifdef _WIN32   
    LARGE_INTEGER liCounter;
    LARGE_INTEGER liCurrent;

    if (!QueryPerformanceFrequency(&liCounter))
        return GetTickCount();

    QueryPerformanceCounter(&liCurrent);
    return (uint64_t)(liCurrent.QuadPart * 1000 / liCounter.QuadPart);
#else
    struct timeval tval;
    uint64_t ret_tick;

    gettimeofday(&tval,NULL);  //用于获取当前时间的精确值
    // 通过 `tval.tv_sec * 1000L` 将秒数转换为毫秒，并通过 `tval.tv_usec / 1000L` 将微秒部分转换为毫秒，然后将二者相加，得到了以毫秒为单位的时间戳
    ret_tick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
    return ret_tick;
#endif
}

void util_sleep(uint32_t millisecond){
#ifdef _WIN32
    Sleep(millisecond);
#else   
    usleep(millisecond*1000);
#endif
}

//接受一个字符串和分隔符，并将字符串，拆分为多个字符串
CStrExplode::CStrExplode(char *str,char seperator){
    item_cnt_=1;
    char *pos=str;
    while(*pos){
        if(*pos==seperator){
            item_cnt_++;
        }
        pos++;
    }
    item_list_=new char *[item_cnt_];

    int idx=0;
    char *start=pos=str;
    //处理中间子串
    while(*pos){
        if(pos!=start && *pos ==seperator){
            uint32_t len = pos - start;
            item_list_[idx]=new char[len+1];
            strncpy(item_list_[idx],start,len);
            item_list_[idx][len]='\0';
            idx++;
            start=pos+1;
        }
        pos++;
    }

    //处理最后一个子串
    uint32_t len=pos-start;
    if(len!=0){
        item_list_[idx]=new char[len+1];
        strncpy(item_list_[idx],start,len);
        item_list_[idx][len]='\0';
    }
}

CStrExplode::~CStrExplode(){
    for(uint32_t i=0;i<item_cnt_;i++){
        delete[] item_list_[i];
    }
    delete[] item_list_;
}

//目的是替换字符串中的特定字符为另一个字符
char *ReplaceStr(char *src,char old_char,char new_char){
    if(src==NULL){
        return NULL;
    }
    char *head=src;
    while(*head!='\0'){
        if(*head==old_char){
            *head=new_char;
        }
        ++head;
    }
    return src;
}

string Int2String(uint32_t user_id){
    stringstream ss;
    ss<<user_id;
    return ss.str();
}

uint32_t String2Int(const string &value) {
    return (uint32_t)atoi(value.c_str());
}

//由于被替换的内容可能包含？，所以需要更新开始搜寻的位置信息，来避免刚刚插入的？
void ReplaceMark(string &str,string &new_value,uint32_t &begin_pos){
    //size_type通常被用来表示字符串的长度、索引或者位置。这个类型被设计为足够大以便能够容纳字符串的最大可能长度，因此它可以确保足够的存储空间来表示字符串的大小
    string::size_type pos = str.find('?',begin_pos);
    if(pos==string::npos){
        return;
    }
    string prime_new_value = "'"+new_value+"'";
    str.replace(pos,1,prime_new_value);  //在字符串 `str` 中从位置 `pos` 处开始，替换长度为 1 的子字符串（即问号 `?`）为 `prime_new_value`
    begin_pos=pos+prime_new_value.size();
}

void ReplaceMark(string &str, uint32_t new_value, uint32_t &begin_pos) {
    stringstream ss;
    ss << new_value;

    string str_value = ss.str();
    string::size_type pos = str.find('?', begin_pos);
    if (pos == string::npos) {
        return;
    }

    str.replace(pos, 1, str_value);
    begin_pos = pos + str_value.size();
}

//获取id并写入文件
void WritePid(){
    uint32_t curPid;
#ifdef _WIN32
    urPid = (uint32_t)GetCurrentProcess();
#else
    curPid=(uint32_t)getpid();
#endif
    FILE *f=fopen("server.pid","w");
    assert(f);
    char szPid[32];
    snprintf(szPid,sizeof(szPid),"%d",curPid);
    //将字符串 `szPid` 写入文件流 `f` 中
    fwrite(szPid,strlen(szPid),1,f);
    fclose(f);
}

inline unsigned char ToHex(const unsigned char &x) {
    return x > 9 ? x - 10 + 'A' : x + '0';
}

inline unsigned char FromHex(const unsigned char &x) {
    return isdigit(x) ? x - '0' : x - 'A' + 10;
}

string URLEncode(const string &in) {
    string out;
    for (size_t ix = 0; ix < in.size(); ix++) {
        unsigned char buf[4];
        memset(buf, 0, 4);
        if (isalnum((unsigned char)in[ix])) {
            buf[0] = in[ix];
        }
        // else if ( isspace( (unsigned char)in[ix] ) )
        // //貌似把空格编码成%20或者+都可以
        //{
        //     buf[0] = '+';
        // }
        else {
            buf[0] = '%';
            buf[1] = ToHex((unsigned char)in[ix] >> 4);
            buf[2] = ToHex((unsigned char)in[ix] % 16);
        }
        out += (char *)buf;
    }
    return out;
}

string URLDecode(const string &in) {
    string out;
    for (size_t ix = 0; ix < in.size(); ix++) {
        unsigned char ch = 0;
        if (in[ix] == '%') {
            ch = (FromHex(in[ix + 1]) << 4);
            ch |= FromHex(in[ix + 2]);
            ix += 2;
        } else if (in[ix] == '+') {
            ch = ' ';
        } else {
            ch = in[ix];
        }
        out += (char)ch;
    }
    return out;
}

int64_t GetFileSize(const char *path) {
    int64_t filesize = -1;
    struct stat statbuff;
    //`stat` 函数用于获取指定路径的文件或目录的详细信息，并将这些信息存储在一个名为 `statbuff` 的结构体中。这个函数通常用于检索文件或目录的元数据，比如文件大小、创建时间、修改时间等信息
    //返回值为0表示成功
    if (stat(path, &statbuff) < 0) {
        return filesize;
    } else {
        filesize = statbuff.st_size;
    }
    return filesize;
}

/*
### 函数参数：

- `src_str`：源字符串的指针，函数会在这个字符串中查找子字符串。

- `src_len`：源字符串的长度。

- `sub_str`：要查找的子字符串的指针。

- `sub_len`：要查找的子字符串的长度。

- `flag`：一个布尔值，用于指定查找的方向，如果为 `true`，则从源字符串的开头向结尾查找；如果为 `false`，则从源字符串的结尾向开头查找
*/

const char *MemFind(const char *src_str, size_t src_len, const char *sub_str,
                    size_t sub_len, bool flag) {
    if (NULL == src_str || NULL == sub_str || src_len <= 0) {
        return NULL;
    }
    if (src_len < sub_len) {
        return NULL;
    }
    const char *p;
    if (sub_len == 0)
        sub_len = strlen(sub_str);
    if (src_len == sub_len) {
        if (0 == (memcmp(src_str, sub_str, src_len))) {
            return src_str;
        } else {
            return NULL;
        }
    }
    if (flag) {
        for (int i = 0; i < src_len - sub_len; i++) {
            p = src_str + i;
            if (0 == memcmp(p, sub_str, sub_len))
                return p;
        }
    } else {
        for (int i = (src_len - sub_len); i >= 0; i--) {
            p = src_str + i;
            if (0 == memcmp(p, sub_str, sub_len))
                return p;
        }
    }
    return NULL;
}