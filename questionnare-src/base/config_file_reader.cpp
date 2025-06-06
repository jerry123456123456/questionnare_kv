#include <iostream>

#include "config_file_reader.h"
CConfigFileReader::CConfigFileReader(const char *filename) {
    _LoadFile(filename);
}

CConfigFileReader::~CConfigFileReader() {}

//这个是使用例子
// char *log_level = config_file.GetConfigName("log_level");   //读取日志设置级别
char *CConfigFileReader::GetConfigName(const char *name){
    if(!load_ok_)return NULL;

    char *value = NULL;
    map<string,string>::iterator it = config_map_.find(name);
    if(it != config_map_.end()){
        value = (char *)it->second.c_str();
    }
    return value;
}

int CConfigFileReader::SetConfigValue(const char *name, const char *value) {
    if (!load_ok_)
        return -1;

    map<string, string>::iterator it = config_map_.find(name);
    if (it != config_map_.end()) {
        it->second = value;
    } else {
        config_map_.insert(make_pair(name, value));
    }
    return _WriteFIle();
}

void CConfigFileReader::_LoadFile(const char *filename) {
    config_file_.clear();
    config_file_.append(filename);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("can not open %s,errno = %d", filename, errno);
        return;
    }

    char buf[256];
    for (;;) {
        char *p = fgets(buf, 256, fp);
        if (!p)
            break;

        size_t len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = 0; // remove \n at the end

        char *ch = strchr(buf, '#'); // remove string start with #
        if (ch)
            *ch = 0;

        if (strlen(buf) == 0)
            continue;

        _ParseLine(buf);
    }

    std::cout << "Map contents:" << std::endl;
    for (const auto& kv : config_map_) {
        std::cout << "Key: " << kv.first << ", Value: " << kv.second << std::endl;
    }

    fclose(fp);
    load_ok_ = true;
}

//写回配置文件
int CConfigFileReader::_WriteFIle(const char *filename) {
    FILE *fp = NULL;
    if (filename == NULL) {
        fp = fopen(config_file_.c_str(), "w");
    } else {
        fp = fopen(filename, "w");
    }
    if (fp == NULL) {
        return -1;
    }

    char szPaire[128];
    map<string, string>::iterator it = config_map_.begin();
    for (; it != config_map_.end(); it++) {
        memset(szPaire, 0, sizeof(szPaire));
        snprintf(szPaire, sizeof(szPaire), "%s=%s\n", it->first.c_str(),
                 it->second.c_str());
        uint32_t ret = fwrite(szPaire, strlen(szPaire), 1, fp);
        if (ret != 1) {
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return 0;
}

void CConfigFileReader::_ParseLine(char *line) {    //这个函数的作用是将字符串等号分割为两个，以键值对的方式存储在map中
    char *p = strchr(line, '=');
    if (p == NULL)
        return;

    *p = 0;     //以0分隔
    char *key = _TrimSpace(line);
    char *value = _TrimSpace(p + 1);
    if (key && value) {
        config_map_.insert(make_pair(key, value));
    }
}

char *CConfigFileReader::_TrimSpace(char *name) {     //去除字符串开头和结尾的空白字符，并返回开始的位置
    // remove starting space or tab
    char *start_pos = name;
    while ((*start_pos == ' ') || (*start_pos == '\t')) {
        start_pos++;
    }

    if (strlen(start_pos) == 0)
        return NULL;

    // remove ending space or tab
    char *end_pos = name + strlen(name) - 1;
    while ((*end_pos == ' ') || (*end_pos == '\t')) {
        *end_pos = 0;
        end_pos--;
    }

    int len = (int)(end_pos - start_pos) + 1;
    if (len <= 0)
        return NULL;

    return start_pos;
}