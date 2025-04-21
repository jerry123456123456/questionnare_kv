#include"base_socket.h"
#include"event_dispatch.h"
#include "ostype.h"
#include <arpa/inet.h>
#include <asm-generic/ioctls.h>
#include <asm-generic/socket.h>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

//网络句柄（net_handle）通常是指在网络编程中用来标识一个网络连接或套接字的唯一标识符
typedef hash_map<net_handle_t,CBaseSocket *>SocketMap;  //用于存储网络句柄和套接字之间的映射关系
SocketMap g_socket_map;

void AddBaseSocket(CBaseSocket *pSocket){
    g_socket_map.insert(make_pair((net_handle_t)pSocket->GetSocket(),pSocket));  
}

/*等价上面
void AddBaseSocket(CBaseSocket *pSocket){
    g_socket_map[(net_handle_t)pSocket->GetSocket()] = pSocket;
}
*/

void RemoveBaseSocket(CBaseSocket *pSocket){
    g_socket_map.erase((net_handle_t)pSocket->GetSocket());
}

CBaseSocket *FindBaseSocket(net_handle_t fd){
    CBaseSocket *pSocket=NULL;
    SocketMap::iterator iter=g_socket_map.find(fd);
    //接着通过first和second方式访问
    if(iter!=g_socket_map.end()){
        pSocket=iter->second;
        pSocket->AddRef();
    }
    return pSocket;
}

CBaseSocket::CBaseSocket(){
    socket_=INVALID_SOCKET;  //在创建 `CBaseSocket` 对象时，其套接字初始状态为无效
    state_=SOCKET_STATE_IDLE;  //这里表示套接字的空闲状态
}

CBaseSocket::~CBaseSocket() {
    // printf("CBaseSocket::~CBaseSocket, socket=%d\n", m_socket);
}

int CBaseSocket::Listen(const char *server_ip,uint16_t port,
                        callback_t callback,void *callback_data){
    local_ip_=server_ip;
    local_port_=port;
    callback_=callback;
    callback_data_=callback_data;

    socket_=socket(AF_INET,SOCK_STREAM,0);
    if(socket_==INVALID_SOCKET){
        printf("socket failed, err_code=%d, server_ip=%s, port=%u",
               _GetErrorCode(), server_ip, port);
        return NETLIB_ERROR;
    }
    _SetReuseAddr(socket_);   //设置地址复用
    _SetNonBlock(socket_);   //非阻塞

    sockaddr_in serv_addr;   //目标的ip和port
    _SetAddr(server_ip,port,&serv_addr);
    //将给定的套接字与指定的地址（存储在 `serv_addr` 结构体中）进行绑定
    /*
    `::bind` 的写法中的双冒号 `::` 是作用域解析操作符，表示在全局作用域中查找函数 `bind`，
    以避免与局部定义的同名函数或其他作用域中的函数发生冲突。在这种情况下，
    `::bind` 明确指定了使用全局作用域中的 `bind` 函数，而不是在局部作用域中查找
    */
    int ret =::bind(socket_,(sockaddr *)&serv_addr,sizeof(serv_addr));
    if(ret==SOCKET_ERROR){
        printf("bind failed, err_code=%d, server_ip=%s, port=%u",
               _GetErrorCode(), server_ip, port);
        closesocket(socket_);
        return NETLIB_ERROR;
    }
    ret=listen(socket_,64);  //请求队列的最大长度是64
    if (ret == SOCKET_ERROR) {
        printf("listen failed, err_code=%d, server_ip=%s, port=%u",
               _GetErrorCode(), server_ip, port);
        closesocket(socket_);
        return NETLIB_ERROR;
    }
    state_=SOCKET_STATE_LISTENING;
    printf("CBaseSocket::Listen on %s:%d", server_ip, port);
    AddBaseSocket(this);   //当前的对象pSocket
    //获取唯一实例，并添加到epoll事件树中
    CEventDispatch::Instance()->AddEvent(socket_, SOCKET_READ | SOCKET_EXCEP);
    return NETLIB_OK;
}

net_handle_t CBaseSocket::Connect(const char *server_ip, uint16_t port,
                                  callback_t callback, void *callback_data) {
    printf("CBaseSocket::Connect, server_ip=%s, port=%d", server_ip, port);

    remote_ip_ = server_ip;
    remote_port_ = port;
    callback_ = callback;
    callback_data_ = callback_data;

    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == INVALID_SOCKET) {
        printf("socket failed, err_code=%d, server_ip=%s, port=%u",
               _GetErrorCode(), server_ip, port);
        return NETLIB_INVALID_HANDLE;
    }

    _SetNonBlock(socket_);
    _SetNoDelay(socket_);
    sockaddr_in serv_addr;
    _SetAddr(server_ip, port, &serv_addr);
    int ret = connect(socket_, (sockaddr *)&serv_addr, sizeof(serv_addr));
    if ((ret == SOCKET_ERROR) && (!_IsBlock(_GetErrorCode()))) {
        printf("connect failed, err_code=%d, server_ip=%s, port=%u",
               _GetErrorCode(), server_ip, port);
        closesocket(socket_);
        return NETLIB_INVALID_HANDLE;
    }
    state_ = SOCKET_STATE_CONNECTING;
    AddBaseSocket(this);
    CEventDispatch::Instance()->AddEvent(socket_, SOCKET_ALL);

    return (net_handle_t)socket_;
}

int CBaseSocket::Send(void *buf, int len) {
    if (state_ != SOCKET_STATE_CONNECTED)
        return NETLIB_ERROR;

    int ret = send(socket_, (char *)buf, len, 0);
    if (ret == SOCKET_ERROR) {
        int err_code = _GetErrorCode();
        if (_IsBlock(err_code)) {
#if ((defined _WIN32) || (defined __APPLE__))
            CEventDispatch::Instance()->AddEvent(m_socket, SOCKET_WRITE);
#endif
            ret = 0;
            // printf("socket send block fd=%d", m_socket);
        } else {
            printf("send failed, err_code=%d, len=%d", err_code, len);
        }
    }

    return ret;
}

int CBaseSocket::Recv(void *buf, int len) {
    return recv(socket_, (char *)buf, len, 0);
}

int CBaseSocket::Close(){
    CEventDispatch::Instance()->RemoveEvent(socket_, SOCKET_ALL);
    RemoveBaseSocket(this);
    closesocket(socket_);
    ReleaseRef();
    return 0;
}


void CBaseSocket::OnRead(){
    if(state_==SOCKET_STATE_LISTENING){
        _AcceptNewSocket();
    }else{
        u_long avail=0;
        //它用于获取有关套接字状态或配置的信息,avail存储查询结果
        //- `FIONREAD`：表示要查询套接字接收缓冲区中可读取的字节数
        int ret = ioctlsocket(socket_,FIONREAD,&avail);
        if((SOCKET_ERROR == ret || (avail== 0))){
            //即套接字操作出现错误或者没有可读取的数据，那么会调用 `callback_` 函数，并传递相应的参数，用于处理套接字关闭或其他相关操作
            callback_(callback_data_, NETLIB_MSG_CLOSE, (net_handle_t)socket_,
                      NULL);  //错误
        }else{
            callback_(callback_data_, NETLIB_MSG_READ, (net_handle_t)socket_,
                      NULL);
        }
    }
}

void CBaseSocket::OnWrite(){
#if ((defined _WIN32) || (defined __APPLE__))
    CEventDispatch::Instance()->RemoveEvent(m_socket, SOCKET_WRITE);
#endif

    if(state_==SOCKET_STATE_CONNECTING){
        int error = 0;
        socklen_t len=sizeof(error);
#ifdef _WIN32
        getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char *)&error, &len);
#else
        getsockopt(socket_, SOL_SOCKET, SO_ERROR, (void *)&error, &len);
#endif
        if (error) {
            callback_(callback_data_, NETLIB_MSG_CLOSE, (net_handle_t)socket_,
                      NULL);
        } else {
            state_ = SOCKET_STATE_CONNECTED;
            callback_(callback_data_, NETLIB_MSG_CONFIRM, (net_handle_t)socket_,
                      NULL);
        }
    }else{
        callback_(callback_data_, NETLIB_MSG_WRITE, (net_handle_t)socket_,
                  NULL);
    }
}

void CBaseSocket::OnClose(){
    state_=SOCKET_STATE_CLOSING;
    callback_(callback_data_, NETLIB_MSG_CLOSE, (net_handle_t)socket_, NULL);
}

void CBaseSocket::SetSendBufSize(uint32_t send_size){
    int ret = setsockopt(socket_,SOL_SOCKET,SO_SNDBUF,&send_size,4);
    if(ret==SOCKET_ERROR){
        printf("set SO_SNDBUF failed for fd=%d", socket_);
    }
    socklen_t len=4;
    int size = 0;
    //获取套接字的大小并存储在size中,len是size数据结构的大小
    getsockopt(socket_,SOL_SOCKET,SO_SNDBUF,&size,&len);
    printf("socket=%d send_buf_size=%d", socket_, size);
}

void CBaseSocket::SetRecvBufSize(uint32_t recv_size) {
    int ret = setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &recv_size, 4);
    if (ret == SOCKET_ERROR) {
        printf("set SO_RCVBUF failed for fd=%d", socket_);
    }

    socklen_t len = 4;
    int size = 0;
    getsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &size, &len);
    printf("socket=%d recv_buf_size=%d", socket_, size);
}

int CBaseSocket::_GetErrorCode(){
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

bool CBaseSocket::_IsBlock(int error_code) {
#ifdef _WIN32
    return ((error_code == WSAEINPROGRESS) || (error_code == WSAEWOULDBLOCK));
#else
    return ((error_code == EINPROGRESS) || (error_code == EWOULDBLOCK));
#endif
}

void CBaseSocket::_SetNonBlock(SOCKET fd){
#ifdef _WIN32
    u_long nonblock = 1;
    int ret = ioctlsocket(fd, FIONBIO, &nonblock);
#else
    //取出来设成非阻塞在设回去
    int ret = fcntl(fd,F_SETFL,O_NONBLOCK | fcntl(fd,F_GETFL));
#endif
    if (ret == SOCKET_ERROR) {
        printf("_SetNonblock failed, err_code=%d, fd=%d", _GetErrorCode(), fd);
    }
}

void CBaseSocket::_SetReuseAddr(SOCKET fd) {
    int reuse = 1;
    int ret =
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
    if (ret == SOCKET_ERROR) {
        printf("_SetReuseAddr failed, err_code=%d, fd=%d", _GetErrorCode(), fd);
    }
}

void CBaseSocket::_SetNoDelay(SOCKET fd) {
    int nodelay = 1;
    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&nodelay,
                         sizeof(nodelay));
    if (ret == SOCKET_ERROR) {
        printf("_SetNoDelay failed, err_code=%d, fd=%d", _GetErrorCode(), fd);
    }
}

void CBaseSocket::_SetAddr(const char *ip,const uint16_t port,
                           sockaddr_in *addr){
    memset(addr,0,sizeof(sockaddr_in));
    addr->sin_family=AF_INET;
    addr->sin_port=htons(port);
    addr->sin_addr.s_addr=inet_addr(ip);
    //这段代码的作用是在指定的IP地址无效时，尝试通过主机名解析获取有效的IP地址，并将其赋值给 `sockaddr_in` 结构体中的 `sin_addr.s_addr` 字段，以确保 `addr` 结构体中包含有效的IP地址
    if (addr->sin_addr.s_addr == INADDR_NONE) {
        hostent *host = gethostbyname(ip);
        if (host == NULL) {
            printf("gethostbyname failed, ip=%s, port=%u", ip, port);
            return;
        }

        addr->sin_addr.s_addr = *(uint32_t *)host->h_addr;
    }
}

void CBaseSocket::_AcceptNewSocket(){
    SOCKET fd = 0;
    sockaddr_in peer_addr;
    socklen_t addr_len = sizeof(sockaddr_in);
    char ip_str[64];
    while((fd=accept(socket_,(sockaddr *)&peer_addr,&addr_len))!=INVALID_SOCKET){
        CBaseSocket *pSocket = new CBaseSocket();
        uint32_t ip = ntohl(peer_addr.sin_addr.s_addr);
        uint16_t port = ntohs(peer_addr.sin_port);

        snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip >> 24,
                 (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);

        pSocket->SetSocket(fd);
        pSocket->SetCallback(callback_);
        pSocket->SetCallbackData(callback_data_);
        pSocket->SetState(SOCKET_STATE_CONNECTED);
        pSocket->SetRemoteIp(ip_str);
        pSocket->SetRemotePort(port);

        _SetNoDelay(fd);
        _SetNonBlock(fd);
        AddBaseSocket(pSocket);
        CEventDispatch::Instance()->AddEvent(fd, SOCKET_READ | SOCKET_EXCEP);
        //通过在建立连接后调用 callback_，服务器能够有效地通知应用层有新客户端连接，并允许应用层在不阻塞的情况下进行处理
        callback_(callback_data_, NETLIB_MSG_CONNECT, (net_handle_t)fd, NULL);
    }
}