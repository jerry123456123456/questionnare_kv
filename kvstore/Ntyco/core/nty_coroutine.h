// 防止头文件被重复包含的预处理器指令
#ifndef __NTY_COROUTINE_H__
#define __NTY_COROUTINE_H__

// 定义 _GNU_SOURCE 宏，启用 GNU 扩展特性
#define _GNU_SOURCE
// 包含动态链接库操作的头文件
#include <dlfcn.h>

// 定义 _USE_UCONTEXT 宏，用于启用 ucontext 相关功能
#define _USE_UCONTEXT

// 包含标准输入输出库头文件
#include <stdio.h>
// 包含标准库头文件，提供内存分配、随机数生成等功能
#include <stdlib.h>
// 包含字符串处理库头文件
#include <string.h>
// 包含标准整数类型定义头文件
#include <stdint.h>
// 包含极限值定义头文件，如 INT_MAX 等
#include <limits.h>
// 包含断言库头文件，用于调试时的条件检查
#include <assert.h>
// 包含格式化整数类型输出的头文件
#include <inttypes.h>
// 包含 Unix 标准库头文件，提供系统调用和文件操作等功能
#include <unistd.h>
// 包含 POSIX 线程库头文件
#include <pthread.h>
// 包含文件控制选项头文件，用于设置文件描述符的属性
#include <fcntl.h>
// 包含系统时间相关头文件，用于获取时间信息
#include <sys/time.h>
// 包含内存映射库头文件，用于内存管理
#include <sys/mman.h>
// 包含 TCP 相关的头文件
#include <netinet/tcp.h>

// 如果定义了 _USE_UCONTEXT 宏，则包含 ucontext 相关的头文件
#ifdef _USE_UCONTEXT
#include <ucontext.h>
#endif

// 包含 epoll 相关的头文件，用于高效的 I/O 多路复用
#include <sys/epoll.h>
// 包含 poll 相关的头文件，用于 I/O 事件的轮询
#include <sys/poll.h>

// 包含错误号定义头文件，用于处理系统调用的错误信息
#include <errno.h>

// 包含自定义的队列和树结构的头文件
#include "nty_queue.h"
#include "nty_tree.h"

// 定义 epoll 事件的最大数量
#define NTY_CO_MAX_EVENTS		(1024*1024)
// 定义协程栈的最大大小
#define NTY_CO_MAX_STACKSIZE	(128*1024) // {http: 16*1024, tcp: 4*1024}

// 定义一个宏，用于将整数 x 左移一位，通常用于位操作
#define BIT(x)	 				(1 << (x))
// 定义一个宏，用于清除整数 x 对应的位
#define CLEARBIT(x) 			~(1 << (x))

// 定义一个宏，用于取消文件描述符等待的 64 位整数标识
#define CANCEL_FD_WAIT_UINT64	1

// 定义一个函数指针类型，用于表示协程的执行函数
typedef void (*proc_coroutine)(void *);

// 定义一个枚举类型，用于表示协程的各种状态
typedef enum {
    // 协程等待读事件
    NTY_COROUTINE_STATUS_WAIT_READ,
    // 协程等待写事件
    NTY_COROUTINE_STATUS_WAIT_WRITE,
    // 协程刚创建，处于新状态
    NTY_COROUTINE_STATUS_NEW,
    // 协程准备好执行
    NTY_COROUTINE_STATUS_READY,
    // 协程已经退出
    NTY_COROUTINE_STATUS_EXITED,
    // 协程正在执行中
    NTY_COROUTINE_STATUS_BUSY,
    // 协程正在睡眠
    NTY_COROUTINE_STATUS_SLEEPING,
    // 协程的等待时间已过期
    NTY_COROUTINE_STATUS_EXPIRED,
    // 协程对应的文件描述符已到达文件末尾
    NTY_COROUTINE_STATUS_FDEOF,
    // 协程已分离，不依赖其他协程
    NTY_COROUTINE_STATUS_DETACH,
    // 协程已被取消
    NTY_COROUTINE_STATUS_CANCELLED,
    // 协程等待计算资源
    NTY_COROUTINE_STATUS_PENDING_RUNCOMPUTE,
    // 协程正在进行计算
    NTY_COROUTINE_STATUS_RUNCOMPUTE,
    // 协程等待 I/O 读事件
    NTY_COROUTINE_STATUS_WAIT_IO_READ,
    // 协程等待 I/O 写事件
    NTY_COROUTINE_STATUS_WAIT_IO_WRITE,
    // 协程等待多个事件
    NTY_COROUTINE_STATUS_WAIT_MULTI
} nty_coroutine_status;

// 定义一个枚举类型，用于表示协程计算资源的状态
typedef enum {
    // 协程计算资源繁忙
    NTY_COROUTINE_COMPUTE_BUSY,
    // 协程计算资源空闲
    NTY_COROUTINE_COMPUTE_FREE
} nty_coroutine_compute_status;

// 定义一个枚举类型，用于表示协程的 I/O 事件类型
typedef enum {
    // 读事件
    NTY_COROUTINE_EV_READ,
    // 写事件
    NTY_COROUTINE_EV_WRITE
} nty_coroutine_event;


// 定义一个链表头类型，用于管理协程的链表
LIST_HEAD(_nty_coroutine_link, _nty_coroutine);
// 定义一个尾队列头类型，用于管理协程的尾队列
TAILQ_HEAD(_nty_coroutine_queue, _nty_coroutine);

// 定义一个红黑树头类型，用于管理睡眠中的协程
RB_HEAD(_nty_coroutine_rbtree_sleep, _nty_coroutine);
// 定义一个红黑树头类型，用于管理等待中的协程
RB_HEAD(_nty_coroutine_rbtree_wait, _nty_coroutine);

// 定义别名，方便使用协程链表头类型
typedef struct _nty_coroutine_link nty_coroutine_link;
// 定义别名，方便使用协程尾队列头类型
typedef struct _nty_coroutine_queue nty_coroutine_queue;

// 定义别名，方便使用睡眠协程红黑树头类型
typedef struct _nty_coroutine_rbtree_sleep nty_coroutine_rbtree_sleep;
// 定义别名，方便使用等待协程红黑树头类型
typedef struct _nty_coroutine_rbtree_wait nty_coroutine_rbtree_wait;

// 如果没有定义 _USE_UCONTEXT 宏，则定义一个结构体表示 CPU 上下文
#ifndef _USE_UCONTEXT
typedef struct _nty_cpu_ctx {
    // 栈指针
    void *esp; 
    // 基址指针
    void *ebp;
    // 指令指针
    void *eip;
    // 通用寄存器
    void *edi;
    // 通用寄存器
    void *esi;
    // 通用寄存器
    void *ebx;
    // 通用寄存器
    void *r1;
    // 通用寄存器
    void *r2;
    // 通用寄存器
    void *r3;
    // 通用寄存器
    void *r4;
    // 通用寄存器
    void *r5;
} nty_cpu_ctx;
#endif

// 定义一个结构体表示协程调度器
typedef struct _nty_schedule {
    // 调度器创建的时间戳
    uint64_t birth;
    // 如果使用 ucontext 则使用 ucontext_t 类型的上下文，否则使用自定义的 nty_cpu_ctx 类型
#ifdef _USE_UCONTEXT
    ucontext_t ctx;
#else
    nty_cpu_ctx ctx;
#endif
    // 调度器的栈指针
    void *stack;
    // 调度器的栈大小
    size_t stack_size;
    // 已创建的协程数量
    int spawned_coroutines;
    // 调度器的默认超时时间
    uint64_t default_timeout;
    // 当前正在执行的协程指针
    struct _nty_coroutine *curr_thread;
    // 系统页面大小
    int page_size;
    // epoll 实例的文件描述符
    int poller_fd;
    // 事件文件描述符，用于触发事件
    int eventfd;
    // epoll 事件列表
    struct epoll_event eventlist[NTY_CO_MAX_EVENTS];
    // epoll 事件的数量
    int nevents;
    // 新产生的事件数量
    int num_new_events;
    // 用于延迟操作的互斥锁
    pthread_mutex_t defer_mutex;
    // 准备好执行的协程队列
    nty_coroutine_queue ready;
    // 延迟执行的协程队列
    nty_coroutine_queue defer;
    // 正在执行的协程链表
    nty_coroutine_link busy;
    // 睡眠中的协程红黑树
    nty_coroutine_rbtree_sleep sleeping;
    // 等待中的协程红黑树
    nty_coroutine_rbtree_wait waiting;
} nty_schedule;

// 定义一个结构体表示协程
typedef struct _nty_coroutine {
    // 私有成员，注释中未详细说明用途
    //private
    // 如果使用 ucontext 则使用 ucontext_t 类型的上下文，否则使用自定义的 nty_cpu_ctx 类型
#ifdef _USE_UCONTEXT
    ucontext_t ctx;
#else
    nty_cpu_ctx ctx;
#endif
    // 协程的执行函数指针
    proc_coroutine func;
    // 传递给协程执行函数的参数
    void *arg;
    // 协程的额外数据指针
    void *data;
    // 协程的栈大小
    size_t stack_size;
    // 协程上一次的栈大小
    size_t last_stack_size;
    // 协程的状态
    nty_coroutine_status status;
    // 协程所属的调度器指针
    nty_schedule *sched;
    // 协程创建的时间戳
    uint64_t birth;
    // 协程的唯一标识符
    uint64_t id;
    // 如果定义了 CANCEL_FD_WAIT_UINT64 宏，则使用文件描述符和事件标志，否则使用 64 位整数表示文件描述符等待
#if CANCEL_FD_WAIT_UINT64
    int fd;
    unsigned short events;  //POLL_EVENT
#else
    int64_t fd_wait;
#endif
    // 协程执行函数的名称
    char funcname[64];
    // 用于协程连接的指针
    struct _nty_coroutine *co_join;
    // 协程退出时的指针
    void **co_exit_ptr;
    // 协程的栈指针,低地址
    void *stack;
    // 协程的基址指针
    void *ebp;
    // 协程的操作计数
    uint32_t ops;
    // 协程的睡眠时间（微秒）
    uint64_t sleep_usecs;
    // 用于红黑树的睡眠节点
    RB_ENTRY(_nty_coroutine) sleep_node;
    // 用于红黑树的等待节点
    RB_ENTRY(_nty_coroutine) wait_node;
    // 用于链表的忙碌节点
    LIST_ENTRY(_nty_coroutine) busy_next;
    // 用于尾队列的准备好节点
    TAILQ_ENTRY(_nty_coroutine) ready_next;
    // 用于尾队列的延迟节点
    TAILQ_ENTRY(_nty_coroutine) defer_next;
    // 用于尾队列的条件节点
    TAILQ_ENTRY(_nty_coroutine) cond_next;
    // 用于尾队列的 I/O 节点
    TAILQ_ENTRY(_nty_coroutine) io_next;
    // 用于尾队列的计算节点
    TAILQ_ENTRY(_nty_coroutine) compute_next;
    // 协程的 I/O 操作结构体
    struct {
        // I/O 缓冲区指针
        void *buf;
        // I/O 操作的字节数
        size_t nbytes;
        // I/O 操作的文件描述符
        int fd;
        // I/O 操作的返回值
        int ret;
        // I/O 操作的错误号
        int err;
    } io;
    // 协程的计算调度器指针
    struct _nty_coroutine_compute_sched *compute_sched;
    // 准备好的文件描述符数量
    int ready_fds;
    // 用于 poll 操作的文件描述符数组指针
    struct pollfd *pfds;
    // 文件描述符的数量
    nfds_t nfds;
} nty_coroutine;

// 定义一个结构体表示协程计算调度器
typedef struct _nty_coroutine_compute_sched {
    // 如果使用 ucontext 则使用 ucontext_t 类型的上下文，否则使用自定义的 nty_cpu_ctx 类型
#ifdef _USE_UCONTEXT
    ucontext_t ctx;
#else
    nty_cpu_ctx ctx;
#endif
    // 协程队列
    nty_coroutine_queue coroutines;
    // 当前正在执行的协程指针
    nty_coroutine *curr_coroutine;
    // 用于运行的互斥锁
    pthread_mutex_t run_mutex;
    // 用于运行的条件变量
    pthread_cond_t run_cond;
    // 用于协程的互斥锁
    pthread_mutex_t co_mutex;
    // 用于链表的计算节点
    LIST_ENTRY(_nty_coroutine_compute_sched) compute_next;
    // 协程计算资源的状态
    nty_coroutine_compute_status compute_status;
} nty_coroutine_compute_sched;

// 声明一个全局的线程键，用于存储调度器指针
extern pthread_key_t global_sched_key;

// 定义一个内联函数，用于获取当前线程的调度器指针
static inline nty_schedule *nty_coroutine_get_sched(void) {
    return pthread_getspecific(global_sched_key);
}

// 定义一个内联函数，用于计算两个时间戳之间的差值（微秒）
static inline uint64_t nty_coroutine_diff_usecs(uint64_t t1, uint64_t t2) {
    return t2 - t1;
}

// 定义一个内联函数，用于获取当前时间的时间戳（微秒）
static inline uint64_t nty_coroutine_usec_now(void) {
    struct timeval t1 = {0, 0};
    gettimeofday(&t1, NULL);
    return t1.tv_sec * 1000000 + t1.tv_usec;
}

// 声明一个函数，用于创建 epoll 实例
int nty_epoller_create(void);

// 声明一个函数，用于取消协程的事件
void nty_schedule_cancel_event(nty_coroutine *co);
// 声明一个函数，用于调度协程的事件
void nty_schedule_sched_event(nty_coroutine *co, int fd, nty_coroutine_event e, uint64_t timeout);

// 声明一个函数，用于将协程从睡眠状态中移除
void nty_schedule_desched_sleepdown(nty_coroutine *co);
// 声明一个函数，用于将协程设置为睡眠状态
void nty_schedule_sched_sleepdown(nty_coroutine *co, uint64_t msecs);

// 声明一个函数，用于将协程从等待状态中移除
nty_coroutine* nty_schedule_desched_wait(int fd);
// 声明一个函数，用于将协程设置为等待状态
void nty_schedule_sched_wait(nty_coroutine *co, int fd, unsigned short events, uint64_t timeout);

// 声明一个函数，用于启动调度器的运行
void nty_schedule_run(void);

// 声明一个函数，用于注册事件触发机制
int nty_epoller_ev_register_trigger(void);
// 声明一个函数，用于等待 epoll 事件
int nty_epoller_wait(struct timespec t);
// 声明一个函数，用于恢复协程的执行
int nty_coroutine_resume(nty_coroutine *co);
// 声明一个函数，用于释放协程的资源
void nty_coroutine_free(nty_coroutine *co);
// 声明一个函数，用于创建一个新的协程
int nty_coroutine_create(nty_coroutine **new_co, proc_coroutine func, void *arg);
// 声明一个函数，用于暂停
void nty_coroutine_yield(nty_coroutine *co);
// 声明一个函数，用于睡眠
void nty_coroutine_sleep(uint64_t msecs);

//协程api
int nty_socket(int domain, int type, int protocol);
int nty_accept(int fd, struct sockaddr *addr, socklen_t *len);
ssize_t nty_recv(int fd, void *buf, size_t len, int flags);
ssize_t nty_send(int fd, const void *buf, size_t len, int flags);
int nty_close(int fd);
int nty_poll(struct pollfd *fds, nfds_t nfds, int timeout);
int nty_connect(int fd, struct sockaddr *name, socklen_t namelen);

ssize_t nty_sendto(int fd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t nty_recvfrom(int fd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);

#define COROUTINE_HOOK 
#ifdef  COROUTINE_HOOK

// 定义一个函数指针类型 socket_t，它指向的函数接受三个整数参数（domain, type, protocol），并返回一个整数。这个函数指针类型用于表示标准的 socket 系统调用函数。
typedef int (*socket_t)(int domain, int type, int protocol);
// 声明一个外部变量 socket_f，它是 socket_t 类型的函数指针。这个变量将在其他地方被赋值为标准 socket 函数的地址，用于后续的函数调用。
extern socket_t socket_f;

// 定义一个函数指针类型 connect_t，它指向的函数接受三个参数（一个整数 fd、一个指向 struct sockaddr 的指针和一个 socklen_t 类型的长度），并返回一个整数。这个函数指针类型用于表示标准的 connect 系统调用函数。
typedef int(*connect_t)(int, const struct sockaddr *, socklen_t);
// 声明一个外部变量 connect_f，它是 connect_t 类型的函数指针。这个变量将在其他地方被赋值为标准 connect 函数的地址，用于后续的函数调用。
extern connect_t connect_f;

// 定义一个函数指针类型 read_t，它指向的函数接受一个整数文件描述符、一个指向 void 类型的指针和一个 size_t 类型的长度，返回一个 ssize_t 类型的值。这个函数指针类型用于表示标准的 read 系统调用函数。
typedef ssize_t(*read_t)(int, void *, size_t);
// 声明一个外部变量 read_f，它是 read_t 类型的函数指针。这个变量将在其他地方被赋值为标准 read 函数的地址，用于后续的函数调用。
extern read_t read_f;

// 定义一个函数指针类型 recv_t，它指向的函数接受四个参数（一个整数文件描述符、一个指向 void 类型的指针、一个 size_t 类型的长度和一个整数标志），返回一个 ssize_t 类型的值。这个函数指针类型用于表示标准的 recv 系统调用函数。
typedef ssize_t(*recv_t)(int sockfd, void *buf, size_t len, int flags);
// 声明一个外部变量 recv_f，它是 recv_t 类型的函数指针。这个变量将在其他地方被赋值为标准 recv 函数的地址，用于后续的函数调用。
extern recv_t recv_f;

// 定义一个函数指针类型 recvfrom_t，它指向的函数接受五个参数（一个整数文件描述符、一个指向 void 类型的指针、一个 size_t 类型的长度、一个整数标志、一个指向 struct sockaddr 的指针和一个指向 socklen_t 类型的指针），返回一个 ssize_t 类型的值。这个函数指针类型用于表示标准的 recvfrom 系统调用函数。
typedef ssize_t(*recvfrom_t)(int sockfd, void *buf, size_t len, int flags,
        struct sockaddr *src_addr, socklen_t *addrlen);
// 声明一个外部变量 recvfrom_f，它是 recvfrom_t 类型的函数指针。这个变量将在其他地方被赋值为标准 recvfrom 函数的地址，用于后续的函数调用。
extern recvfrom_t recvfrom_f;

// 定义一个函数指针类型 write_t，它指向的函数接受一个整数文件描述符、一个指向 const void 类型的指针和一个 size_t 类型的长度，返回一个 ssize_t 类型的值。这个函数指针类型用于表示标准的 write 系统调用函数。
typedef ssize_t(*write_t)(int, const void *, size_t);
// 声明一个外部变量 write_f，它是 write_t 类型的函数指针。这个变量将在其他地方被赋值为标准 write 函数的地址，用于后续的函数调用。
extern write_t write_f;

// 定义一个函数指针类型 send_t，它指向的函数接受四个参数（一个整数文件描述符、一个指向 const void 类型的指针、一个 size_t 类型的长度和一个整数标志），返回一个 ssize_t 类型的值。这个函数指针类型用于表示标准的 send 系统调用函数。
typedef ssize_t(*send_t)(int sockfd, const void *buf, size_t len, int flags);
// 声明一个外部变量 send_f，它是 send_t 类型的函数指针。这个变量将在其他地方被赋值为标准 send 函数的地址，用于后续的函数调用。
extern send_t send_f;

// 定义一个函数指针类型 sendto_t，它指向的函数接受五个参数（一个整数文件描述符、一个指向 const void 类型的指针、一个 size_t 类型的长度、一个整数标志、一个指向 const struct sockaddr 的指针和一个 socklen_t 类型的长度），返回一个 ssize_t 类型的值。这个函数指针类型用于表示标准的 sendto 系统调用函数。
typedef ssize_t(*sendto_t)(int sockfd, const void *buf, size_t len, int flags,
        const struct sockaddr *dest_addr, socklen_t addrlen);
// 声明一个外部变量 sendto_f，它是 sendto_t 类型的函数指针。这个变量将在其他地方被赋值为标准 sendto 函数的地址，用于后续的函数调用。
extern sendto_t sendto_f;

// 定义一个函数指针类型 accept_t，它指向的函数接受三个参数（一个整数文件描述符、一个指向 struct sockaddr 的指针和一个指向 socklen_t 类型的指针），返回一个整数。这个函数指针类型用于表示标准的 accept 系统调用函数。
typedef int(*accept_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
// 声明一个外部变量 accept_f，它是 accept_t 类型的函数指针。这个变量将在其他地方被赋值为标准 accept 函数的地址，用于后续的函数调用。
extern accept_t accept_f;

// 定义一个新的函数指针类型 close_t，它指向的函数接受一个整数文件描述符，返回一个整数。这个函数指针类型用于表示标准的 close 系统调用函数。
typedef int(*close_t)(int);
// 声明一个外部变量 close_f，它是 close_t 类型的函数指针。这个变量将在其他地方被赋值为标准 close 函数的地址，用于后续的函数调用。
extern close_t close_f;

// 声明一个函数 init_hook，它不接受任何参数，返回一个整数。这个函数的作用是初始化上述声明的所有函数指针，将它们指向对应的标准系统调用函数。
int init_hook(void);

#endif
    
#endif
