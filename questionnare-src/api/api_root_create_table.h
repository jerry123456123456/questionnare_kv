#ifndef _API_ROOT_CREATE_TABLE_H_
#define _API_ROOT_CREATE_TABLE_H_
#include "api_common.h"

struct table_cb_t{
    string table_name;
    string user_name;
};

//到时间触发的回调函数
void table_cb(void *callback_data, uint8_t msg, uint32_t handle,
                              void *pParam);

int ApiRootTableCreate(string &url,string &post_data,string &str_json);

#endif