#ifndef __NETLIB_H__
#define __NETLIB_H__

#include"ostype.h"
#include <cstdint>

#define NETLIB_OPT_SET_CALLBACK 1
#define NETLIB_OPT_SET_CALLBACK_DATA 2
#define NETLIB_OPT_GET_REMOTE_IP 3
#define NETLIB_OPT_GET_REMOTE_PORT 4
#define NETLIB_OPT_GET_LOCAL_IP 5
#define NETLIB_OPT_GET_LOCAL_PORT 6
#define NETLIB_OPT_SET_SEND_BUF_SIZE 7
#define NETLIB_OPT_SET_RECV_BUF_SIZE 8

#define NETLIB_MAX_SOCKET_BUF_SIZE (128 * 1024)

#ifdef __cplusplus
extern "C"{  // `extern "C"` 声明告诉编译器使用 C 语言的命名规则，即不进行名称修饰（不进行名称重整、函数名不会被加上类似于函数的作用域标识符等）
#endif
    int netlib_init();
    int netlib_destroy();
    int netlib_listen(
        const char *server_ip,
        uint16_t port,
        callback_t callback,
        void *callback_data
    );

    net_handle_t netlib_connect(
        const char *server_ip,
        uint16_t port,
        callback_t callback,
        void *callback_data
    );

    int netlib_send(net_handle_t handle,void *buf,int len);

    int netlib_recv(net_handle_t handle,void *buf,int len);

    int netlib_close(net_handle_t handle);

    int netlib_option(net_handle_t handle,int opt,void *optval);

    int netlib_register_timer(callback_t callback,void *user_data,uint64_t interval);

    int netlib_delete_timer(callback_t callback,void *user_data);

    int netlib_add_loop(callback_t callback,void *user_data);

    void netlib_eventloop(uint32_t wait_timeout = 100);

    void netlib_stop_event();

    bool netlib_is_running();

#ifdef __cplusplus
}
#endif

#endif