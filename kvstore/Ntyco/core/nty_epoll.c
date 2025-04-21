#include <sys/eventfd.h>  // 包含 eventfd 系统调用的头文件，用于创建事件文件描述符
#include "nty_coroutine.h"  // 包含自定义的协程框架头文件

/**
 * @brief 创建一个 epoll 实例
 * @return 成功时返回新创建的 epoll 实例的文件描述符，失败时返回 -1
 */
int nty_epoller_create(void) {
    // 调用系统函数 epoll_create 创建一个 epoll 实例，参数 1024 表示该实例初始时能处理的最大文件描述符数量
    return epoll_create(1024);
} 

/**
 * @brief 等待 epoll 实例上的事件发生
 * @param t 一个 timespec 结构体，用于指定等待的超时时间
 * @return 成功时返回就绪的文件描述符数量，超时返回 0，出错返回 -1
 */
int nty_epoller_wait(struct timespec t) {
    // 获取当前的协程调度器实例
    nty_schedule *sched = nty_coroutine_get_sched();
    // 调用系统函数 epoll_wait 等待事件发生
    // sched->poller_fd 是 epoll 实例的文件描述符
    // sched->eventlist 是用于存储就绪事件的数组
    // NTY_CO_MAX_EVENTS 是事件数组的最大长度
    // t.tv_sec*1000.0 + t.tv_nsec/1000000.0 是将 timespec 结构体中的时间转换为毫秒
    return epoll_wait(sched->poller_fd, sched->eventlist, NTY_CO_MAX_EVENTS, t.tv_sec*1000.0 + t.tv_nsec/1000000.0);
}

/**
 * @brief 注册并触发事件文件描述符的监听
 * @return 成功时返回 0，出错时由于使用了 assert 会终止程序
 */
int nty_epoller_ev_register_trigger(void) {
    // 获取当前的协程调度器实例
    nty_schedule *sched = nty_coroutine_get_sched();

    // 检查调度器的事件文件描述符是否已经创建
    if (!sched->eventfd) {
        // 如果未创建，则调用 eventfd 函数创建一个非阻塞的事件文件描述符
        sched->eventfd = eventfd(0, EFD_NONBLOCK);
        // 断言事件文件描述符创建成功，如果失败则终止程序
        assert(sched->eventfd != -1);
    }

    // 定义一个 epoll_event 结构体，用于设置要监听的事件
    struct epoll_event ev;
    // 设置监听的事件为 EPOLLIN，表示有可读事件发生
    ev.events = EPOLLIN;
    // 设置事件关联的文件描述符为调度器的事件文件描述符
    ev.data.fd = sched->eventfd;
    // 调用 epoll_ctl 函数将事件文件描述符添加到 epoll 实例中进行监听
    int ret = epoll_ctl(sched->poller_fd, EPOLL_CTL_ADD, sched->eventfd, &ev);

    // 断言 epoll_ctl 调用成功，如果失败则终止程序
    assert(ret != -1);
}