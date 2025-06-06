#include"http_conn.h"
#include "api_mytables.h"
#include"api_root_tables.h"
#include"api_root_delete_table.h"
#include "api_root_create_table.h"
#include "api_deal_table.h"
#include "api_login.h"
#include"api_common.h"
#include"api_register.h"
#include"api_login.h"
#include "dlog.h"
#include"api_mytables.h"
#include"api_unreg.h"
#include "http_parser_wrapper.h"

static HttpConnMap_t g_http_conn_map;

extern string strMsfsUrl;
extern string strDiscovery;

// conn_handle 从0开始递增，可以防止因socket handle重用引起的一些冲突
static uint32_t g_conn_handle_generator = 0;

typedef hash_map<uint32_t, CHttpConn *> UserMap_t;
static UserMap_t g_uuid_conn_map;

uint32_t CHttpConn::s_uuid_alloctor = 0;

std::mutex CHttpConn::s_resp_mutex;
std::list<ResponsePdu_t *> CHttpConn::s_response_pdu_list;

ThreadPool g_thread_pool;


CHttpConn *FindHttpConnByHandle(uint32_t conn_handle) {
    CHttpConn *pConn = NULL;
    HttpConnMap_t::iterator it = g_http_conn_map.find(conn_handle);
    if (it != g_http_conn_map.end()) {
        pConn = it->second;
    }

    return pConn;
}

CHttpConn *GetHttpConnByUuid(uint32_t uuid) {
    CHttpConn *pConn = NULL;
    UserMap_t::iterator it = g_uuid_conn_map.find(uuid);
    if (it != g_uuid_conn_map.end()) {
        pConn = (CHttpConn *)it->second;
    }

    return pConn;
}

void httpconn_callback(void *callback_data, uint8_t msg, uint32_t handle,
                       uint32_t uParam, void *pParam) {
    NOTUSED_ARG(uParam);
    NOTUSED_ARG(pParam);

    // convert void* to uint32_t, oops
    uint32_t conn_handle = *((uint32_t *)(&callback_data));
    CHttpConn *pConn = FindHttpConnByHandle(conn_handle);
    if (!pConn) {
        return;
    }

    switch (msg) {
    case NETLIB_MSG_READ:
        pConn->OnRead();
        break;
    case NETLIB_MSG_WRITE:
        pConn->OnWrite();
        break;
    case NETLIB_MSG_CLOSE:
        pConn->OnClose();
        break;
    default:
        LogError("!!!httpconn_callback error msg:{}", msg);
        
        break;
    }
}

void http_conn_timer_callback(void *callback_data, uint8_t msg, uint32_t handle,
                              void *pParam) {
    UNUSED(pParam);
    CHttpConn *pConn = NULL;
    HttpConnMap_t::iterator it, it_old;
    uint64_t cur_time = GetTickCount();

    for (it = g_http_conn_map.begin(); it != g_http_conn_map.end();) {
        it_old = it;
        it++;

        pConn = it_old->second;
        pConn->OnTimer(cur_time);
    }
}

void InitHttpConn() {
    netlib_register_timer(http_conn_timer_callback, NULL, 1000);
}

/////////////////////////////////
CHttpConn::CHttpConn() {
    busy_ = false;
    m_sock_handle = NETLIB_INVALID_HANDLE;
    state_ = CONN_STATE_IDLE;

    last_send_tick_ = last_recv_tick_ = GetTickCount();
    conn_handle_ = ++g_conn_handle_generator;
    if (conn_handle_ == 0) {
        conn_handle_ = ++g_conn_handle_generator;
    }

    uuid_ = ++CHttpConn::s_uuid_alloctor; // 单线程里用的，不需要加锁
    if (uuid_ == 0) {
        uuid_ = ++CHttpConn::s_uuid_alloctor;
    }
    g_uuid_conn_map.insert(make_pair(uuid_, this)); // 单线程里用的，不需要加锁
    LogInfo("conn_uuid: {}, conn_handle_: {:X}", uuid_, conn_handle_);
}

CHttpConn::~CHttpConn() {
    LogInfo("~CHttpConn, handle = {}", conn_handle_);
}

// int CHttpConn::Send(void *data, int len) {
//     last_send_tick_ = GetTickCount();

//     if (busy_) {
//         out_buf_.Write(data, len);
//         return len;
//     }

//     int ret = netlib_send(m_sock_handle, data, len);
//     if (ret < 0)
//         ret = 0;

//     if (ret < len) {
//         out_buf_.Write(
//             (char *)data + ret,
//             len - ret); // 保存buffer里面，下次reactor write触发后再发送
//         busy_ = true;
//         LogInfo("not send all, remain= {}", out_buf_.GetWriteOffset());
//     } else {
//         OnWriteComlete();
//     }

//     return len;
// }
int CHttpConn::Send(void *data, int len) {
    last_send_tick_ = GetTickCount();

    if (busy_) {
        out_buf_.Write(data, len);
        LogInfo("Connection is busy, data written to buffer. Len: {}", len);
        return len;
    }

    int ret = netlib_send(m_sock_handle, data, len);
    if (ret < 0) {
        LogError("netlib_send failed. Error code: {}", ret);
        return ret;
    }

    if (ret < len) {
        out_buf_.Write((char *)data + ret, len - ret);
        busy_ = true;
        LogInfo("Partially sent. Sent: {}, Remain: {}", ret, len - ret);
    } else {
        OnWriteComlete();
        LogInfo("All data sent successfully. Len: {}", len);
        // 添加调试信息
        LogDebug("Checking connection status after sending...");
        // 可以在这里添加更多的状态检查逻辑
    }

    return ret;
}

void CHttpConn::Close() {
    state_ = CONN_STATE_CLOSED;

    g_http_conn_map.erase(conn_handle_);
    g_uuid_conn_map.erase(uuid_); // 移除uuid
    netlib_close(m_sock_handle);

    ReleaseRef();
}

void CHttpConn::OnConnect(net_handle_t handle) {
    // LogInfo("CHttpConn, handle = {}", handle);
    m_sock_handle = handle;
    state_ = CONN_STATE_CONNECTED;
    g_http_conn_map.insert(make_pair(conn_handle_, this));

    netlib_option(handle, NETLIB_OPT_SET_CALLBACK, (void *)httpconn_callback);
    netlib_option(handle, NETLIB_OPT_SET_CALLBACK_DATA,
                  reinterpret_cast<void *>(conn_handle_));
    netlib_option(handle, NETLIB_OPT_GET_REMOTE_IP, (void *)&peer_ip_);
}

void CHttpConn::OnRead(){
    for(;;){
        uint32_t free_buf_len =
            in_buf_.GetAllocSize() - in_buf_.GetWriteOffset();
        if (free_buf_len < READ_BUF_SIZE + 1)
            in_buf_.Extend(READ_BUF_SIZE + 1);

        int ret = netlib_recv(m_sock_handle,
                              in_buf_.GetBuffer() + in_buf_.GetWriteOffset(),
                              READ_BUF_SIZE);
        if (ret <= 0)
            break;

        in_buf_.IncWriteOffset(ret);

        last_recv_tick_ = GetTickCount();
    }

    //每次请求对应一个http连接，所以读完数据之后，不用再同一个连接里面准备读下一个请求
    char *in_buf = (char *)in_buf_.GetBuffer();
    uint32_t buf_len = in_buf_.GetWriteOffset();
    in_buf[buf_len] = '\0';

    // 如果buf_len 过长可能是受到攻击，则断开连接
    // 正常的url最大长度为2048，我们接受的所有数据长度不得大于2K
    if (buf_len > 2048) {
        LogError("get too much data: {}", in_buf);
        Close();
        return;
    }

    printf("s\n",in_buf);
    LogDebug("buf_len: {}, conn_handle_: {}, in_buf: {}", buf_len, conn_handle_, in_buf);
    // 解析http数据
    http_parser_.ParseHttpContent(in_buf, buf_len); // 1. 从socket接口读取数据；2.然后把数据放到buffer in_buf; 3.http解析
    if(http_parser_.IsReadAll()){
        string url = http_parser_.GetUrl();
        printf("%s\n",url.c_str());
        string content = http_parser_.GetBodyContent();
        printf("%s\n",content.c_str());
        LogInfo("url: {}", url.c_str());
        if(strncmp(url.c_str(),"/api/reg",8) == 0){  //注册
            _HandleRegisterRequest(url, content);
        }else if(strncmp(url.c_str(),"/api/unreg",10) == 0){  //注销
            _HandleunRegisterRequest(url, content);
        }else if(strncmp(url.c_str(), "/api/login", 10) == 0){ //登录
            printf("login\n");
            _HandleLoginRequest(url, content);
        }else if(strncmp(url.c_str(), "/api/mytables", 12) == 0) { //用户下拉可以填的表格，cmd两种参数
        //}else if(strncmp(url.c_str(), "/api/xxx", 3) == 0) { //用户下拉可以填的表格，cmd两种参数
            //count的意思是先获取表格的个数以及title；normal的意思是具体一个表的详细问题
            _HandleMytablesRequest(url, content);
        }else if(strncmp(url.c_str(), "/api/upload", 10) == 0) { //用户上传表格
            _HandleUploadTableRequest(url, content);
        }else if(strncmp(url.c_str(), "/api/delete", 10) == 0){
            _HandleDeleteTableRequest(url,content);
        }else if(strncmp(url.c_str(),"/api/root/tables",16) == 0){
            _HandleRootTableRequest(url,content);
        }else if(strncmp(url.c_str(),"/api/root/table_delete",20) == 0){
            _HandleRootTableDeleteRequest(url,content);
        }else if(strncmp(url.c_str(),"/api/root/table_create",20) == 0){
            _HandleRootTableCreateRequest(url,content);
        }else if(strncmp(url.c_str(),"/api/mypictures",16) == 0){
            _HandlePicture();
        }else{
            LogError("url unknown, url= {}", url);
            Close();
        }
    }
}

void CHttpConn::OnWrite() {
    if (!busy_)
        return;

    // LogInfo("send: {}", m_out_buf.GetWriteOffset());
    int ret = netlib_send(m_sock_handle, out_buf_.GetBuffer(),
                          out_buf_.GetWriteOffset());
    if (ret < 0)
        ret = 0;

    int out_buf_size = (int)out_buf_.GetWriteOffset();

    out_buf_.Read(NULL, ret);

    if (ret < out_buf_size) {
        busy_ = true;
        LogInfo("not send all, remain = {}", out_buf_.GetWriteOffset());
    } else {
        OnWriteComlete();
        busy_ = false;
    }
}

void CHttpConn::OnClose() { Close(); }

void CHttpConn::OnTimer(uint64_t curr_tick) {
    if (curr_tick > last_recv_tick_ + HTTP_CONN_TIMEOUT) {
        LogWarn("HttpConn timeout, handle={}", conn_handle_);
        Close();
    }
}

///////////////////////////////////////////这两个api使用线程池，其他不使用是保证互斥
    // 账号注册处理
    int CHttpConn::_HandleRegisterRequest(string &url, string &post_data){
        g_thread_pool.Exec(ApiRegisterUser, uuid_, url, post_data);
        // 这里不应该再有数据了，
        return 0;
    }

    // 账号登陆处理 
    int CHttpConn::_HandleLoginRequest(string &url, string &post_data)
    {
        g_thread_pool.Exec(ApiUserLogin, uuid_, url, post_data);
        return 0;
    }

//////////////////////////////////////////
    //图片
    int CHttpConn::_HandlePicture(){
        //使用grpc调用python服务器获取图片
        ImageClient client(grpc::CreateChannel("127.0.0.1:50051",grpc::InsecureChannelCredentials()));
        //通过grpc获取图片的二进制数据
        std::string image_data = client.GetImage();

        //如果没有成功获取图片
        if(image_data.empty()){
            std::cerr << "Failed to retrieve image data from server." << std::endl;
            return -1;
        }

        // 设置响应头，指定返回的是图片
        std::string str_header = "HTTP/1.1 200 OK\r\n";
        str_header += "Content-Type: image/jpeg\r\n";  // 根据实际图片类型设置，例如 image/png 或 image/jpg
        str_header += "Content-Length: " + std::to_string(image_data.length()) + "\r\n";
        str_header += "\r\n";  // 响应头结束

        // 合并响应头和图片数据
        std::string response = str_header + image_data;

        // 发送响应
        int ret = Send((void*)response.c_str(), response.length());
        if (ret < 0) {
            std::cerr << "Failed to send response." << std::endl;
            return -1;
        }
        return 0; // 发送成功，返回 0
    }

    //注销
    int CHttpConn::_HandleunRegisterRequest(string &url, string &post_data){
        string str_json;
        int ret = ApiunRegisterUser(url,post_data,str_json);
        char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
        uint32_t ulen = str_json.length();
        snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, ulen,
                str_json.c_str());
        ret = Send((void *)szContent, strlen(szContent)); // 返回值暂时不做处理
        if (ret < 0) {
            std::cerr << "Failed to send response." << std::endl;
            return -1;
        }
        delete[] szContent;
        return 0;
    }

    // 用户加载表格
    int CHttpConn::_HandleMytablesRequest(string &url, string &post_data){
        string str_json;
        int ret = ApiMyTables(url,post_data,str_json);
        char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
        uint32_t ulen = str_json.length();
        snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, ulen,
                str_json.c_str());
        ret = Send((void *)szContent, strlen(szContent)); // 返回值暂时不做处理
        if (ret < 0) {
            std::cerr << "Failed to send response." << std::endl;
            return -1;
        }
        delete[] szContent;
        return 0;
    }

    //用户上传表格
    int CHttpConn::_HandleUploadTableRequest(string &url, string &post_data){
        string str_json;
        int ret = ApiUploadTable(url,post_data,str_json);
        char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
        uint32_t ulen = str_json.length();
        snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, ulen,
                str_json.c_str());
        ret = Send((void *)szContent, strlen(szContent)); // 返回值暂时不做处理
        if (ret < 0) {
            std::cerr << "Failed to send response." << std::endl;
            return -1;
        }
        delete[] szContent;
        return 0;
    }

    //用户删除表格
    int CHttpConn::_HandleDeleteTableRequest(string &url,string &post_data){
        string str_json;
        int ret = ApiDeleteTable(url,post_data,str_json);
        char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
        uint32_t ulen = str_json.length();
        snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, ulen,
                str_json.c_str());
        ret = Send((void *)szContent, strlen(szContent)); // 返回值暂时不做处理
        if (ret < 0) {
            std::cerr << "Failed to send response." << std::endl;
            return -1;
        }
        delete[] szContent;
        return 0;
    }

    //root用户加载表格
    int CHttpConn::_HandleRootTableRequest(string &url,string &post_data){
        string str_json;
        int ret = ApiRootTables(url,post_data,str_json);
        char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
        uint32_t ulen = str_json.length();
        snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, ulen,
                str_json.c_str());
        ret = Send((void *)szContent, strlen(szContent)); // 返回值暂时不做处理
        if (ret < 0) {
            std::cerr << "Failed to send response." << std::endl;
            return -1;
        }
        delete[] szContent;
        return 0;
    }

    int CHttpConn::_HandleRootTableDeleteRequest(string &url,string &post_data){
        string str_json;
        int ret = ApiRootTableDelete(url,post_data,str_json);
        char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
        uint32_t ulen = str_json.length();
        snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, ulen,
                str_json.c_str());
        ret = Send((void *)szContent, strlen(szContent)); // 返回值暂时不做处理
        if (ret < 0) {
            std::cerr << "Failed to send response." << std::endl;
            return -1;
        }
        delete[] szContent;
        return 0;
    }

    int CHttpConn::_HandleRootTableCreateRequest(string &url,string &post_data){
        string str_json;
        int ret = ApiRootTableCreate(url,post_data,str_json);
        char *szContent = new char[HTTP_RESPONSE_HTML_MAX];
        uint32_t ulen = str_json.length();
        snprintf(szContent, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_HTML, ulen,
                str_json.c_str());
        ret = Send((void *)szContent, strlen(szContent)); // 返回值暂时不做处理
        if (ret < 0) {
            std::cerr << "Failed to send response." << std::endl;
            return -1;
        }
        delete[] szContent;
        return 0;
    }
//////////////////////////////////////////

void CHttpConn::OnWriteComlete() {
    LogInfo("write complete");
    Close();
}

//静态
void CHttpConn::SendResponseDataList() {
    // LogInfo("into");
    // 发送数据
    s_resp_mutex.lock();
    while (!s_response_pdu_list.empty()) {
        ResponsePdu_t *pResp = s_response_pdu_list.front();
        s_response_pdu_list.pop_front();
        s_resp_mutex.unlock();
        CHttpConn *pConn = GetHttpConnByUuid(
            pResp->conn_uuid); // 该连接有可能已经被释放，如果被释放则返回NULL
        LogInfo("conn_uuid: {}", pResp->conn_uuid); //{0:x}
        if (pConn) {
             LogInfo("send: {}", pResp->resp_data);
            pConn->Send((void *)pResp->resp_data.c_str(),
                        pResp->resp_data.size());  // 最终socket send
        }
        delete pResp;

        s_resp_mutex.lock();
    }

    s_resp_mutex.unlock();
}

void CHttpConn::AddResponseData(uint32_t conn_uuid, string &resp_data) {
    LogInfo("into");
    ResponsePdu_t *pResp = new ResponsePdu_t;
    pResp->conn_uuid = conn_uuid;
    pResp->resp_data = std::move(resp_data);

    s_resp_mutex.lock();
    s_response_pdu_list.push_back(pResp);
    s_resp_mutex.unlock();
}