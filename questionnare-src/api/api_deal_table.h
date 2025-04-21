#ifndef _API_DEALFILE_H_
#define _API_DEALFILE_H_
#include <string>
using std::string;


int ApiUploadTable(string &url, string &post_data, string &str_json);
int ApiDeleteTable(string &url, string &post_data, string &str_json);

#endif