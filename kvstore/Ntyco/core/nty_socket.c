#include "nty_coroutine.h"

// 该函数用于将 poll 事件标志转换为 epoll 事件标志
// 参数 events：表示 poll 事件的标志
// 返回值：返回对应的 epoll 事件标志
static uint32_t nty_pollevent_2epoll( short events )
{
    // 初始化一个变量 e 用于存储转换后的 epoll 事件标志
    uint32_t e = 0;	
    // 如果 poll 事件包含 POLLIN，则将 EPOLLIN 标志添加到 e 中
    if( events & POLLIN ) 	e |= EPOLLIN;
    // 如果 poll 事件包含 POLLOUT，则将 EPOLLOUT 标志添加到 e 中
    if( events & POLLOUT )  e |= EPOLLOUT;
    // 如果 poll 事件包含 POLLHUP，则将 EPOLLHUP 标志添加到 e 中
    if( events & POLLHUP ) 	e |= EPOLLHUP;
    // 如果 poll 事件包含 POLLERR，则将 EPOLLERR 标志添加到 e 中
    if( events & POLLERR )	e |= EPOLLERR;
    // 如果 poll 事件包含 POLLRDNORM，则将 EPOLLRDNORM 标志添加到 e 中
    if( events & POLLRDNORM ) e |= EPOLLRDNORM;
    // 如果 poll 事件包含 POLLWRNORM，则将 EPOLLWRNORM 标志添加到 e 中
    if( events & POLLWRNORM ) e |= EPOLLWRNORM;
    // 返回转换后的 epoll 事件标志
    return e;
}

// 该函数用于将 epoll 事件标志转换为 poll 事件标志
// 参数 events：表示 epoll 事件的标志
// 返回值：返回对应的 poll 事件标志
static short nty_epollevent_2poll( uint32_t events )
{
    // 初始化一个变量 e 用于存储转换后的 poll 事件标志
    short e = 0;	
    // 如果 epoll 事件包含 EPOLLIN，则将 POLLIN 标志添加到 e 中
    if( events & EPOLLIN ) 	e |= POLLIN;
    // 如果 epoll 事件包含 EPOLLOUT，则将 POLLOUT 标志添加到 e 中
    if( events & EPOLLOUT ) e |= POLLOUT;
    // 如果 epoll 事件包含 EPOLLHUP，则将 POLLHUP 标志添加到 e 中
    if( events & EPOLLHUP ) e |= POLLHUP;
    // 如果 epoll 事件包含 EPOLLERR，则将 POLLERR 标志添加到 e 中
    if( events & EPOLLERR ) e |= POLLERR;
    // 如果 epoll 事件包含 EPOLLRDNORM，则将 POLLRDNORM 标志添加到 e 中
    if( events & EPOLLRDNORM ) e |= POLLRDNORM;
    // 如果 epoll 事件包含 EPOLLWRNORM，则将 POLLWRNORM 标志添加到 e 中
    if( events & EPOLLWRNORM ) e |= POLLWRNORM;
    // 返回转换后的 poll 事件标志
    return e;
}

/*
 * nty_poll_inner 函数的功能是将套接字描述符添加到 epoll 中进行监听，
 * 然后让出当前协程，等待事件发生，最后将套接字描述符从 epoll 中移除
 * 参数 fds：指向 pollfd 结构体数组的指针，包含要监听的文件描述符和事件
 * 参数 nfds：表示 fds 数组的元素个数
 * 参数 timeout：表示超时时间，单位为毫秒
 * 返回值：返回 fds 数组的元素个数
 */
//nty_poll_inner 函数的作用是将文件描述符添加到 epoll 实例中进行监听，然后让出当前协程，等待事件发生
static int nty_poll_inner(struct pollfd *fds, nfds_t nfds, int timeout) {

    // 如果超时时间为 0，则直接调用系统的 poll 函数进行监听
    if (timeout == 0)
    {
        return poll(fds, nfds, timeout);
    }
    // 如果超时时间小于 0，则将超时时间设置为 INT_MAX
    if (timeout < 0)
    {
        timeout = INT_MAX;
    }

    // 获取当前的协程调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    // 如果调度器为空，则输出错误信息并返回 -1
    if (sched == NULL) {
        printf("scheduler not exit!\n");
        return -1;
    }
    
    // 获取当前正在运行的协程
    nty_coroutine *co = sched->curr_thread;
    
    // 遍历 fds 数组中的每个元素
    int i = 0;
    for (i = 0;i < nfds;i ++) {
    
        // 定义一个 epoll_event 结构体变量 ev
        struct epoll_event ev;
        // 将 fds[i] 的事件标志转换为 epoll 事件标志，并赋值给 ev.events
        ev.events = nty_pollevent_2epoll(fds[i].events);
        // 将 fds[i] 的文件描述符赋值给 ev.data.fd
        ev.data.fd = fds[i].fd;
        // 将 fds[i] 的文件描述符添加到 epoll 实例中进行监听
        epoll_ctl(sched->poller_fd, EPOLL_CTL_ADD, fds[i].fd, &ev);

        // 将 fds[i] 的事件标志赋值给当前协程的 events 成员
        co->events = fds[i].events;
        // 将当前协程设置为等待状态，等待指定的文件描述符上的事件发生
        // 统一事件处理逻辑:将协程设置为等待状态后，事件的处理逻辑能够在统一的调度循环中进行，这样可以简化代码结构，提高代码的可维护性。在 nty_schedule_run 函数中，会统一处理超时事件、就绪队列中的协程以及 epoll 事件
        nty_schedule_sched_wait(co, fds[i].fd, fds[i].events, timeout);
    }
    // 让出当前协程的执行权
    nty_coroutine_yield(co); 

    // 再次遍历 fds 数组中的每个元素
    for (i = 0;i < nfds;i ++) {
    
        // 定义一个 epoll_event 结构体变量 ev
        struct epoll_event ev;
        // 将 fds[i] 的事件标志转换为 epoll 事件标志，并赋值给 ev.events
        ev.events = nty_pollevent_2epoll(fds[i].events);
        // 将 fds[i] 的文件描述符赋值给 ev.data.fd
        ev.data.fd = fds[i].fd;
        // 将 fds[i] 的文件描述符从 epoll 实例中移除
        epoll_ctl(sched->poller_fd, EPOLL_CTL_DEL, fds[i].fd, &ev);

        // 将指定文件描述符上等待的协程从等待队列中移除
        nty_schedule_desched_wait(fds[i].fd);
    }

    // 返回 fds 数组的元素个数
    return nfds;
}

// 该函数用于创建一个新的套接字，并将其设置为非阻塞模式
// 参数 domain：表示协议族，如 AF_INET、AF_INET6 等
// 参数 type：表示套接字类型，如 SOCK_STREAM、SOCK_DGRAM 等
// 参数 protocol：表示协议类型，通常为 0
// 返回值：返回新创建的套接字描述符，如果失败则返回 -1
int nty_socket(int domain, int type, int protocol) {

    // 调用系统的 socket 函数创建一个新的套接字
    int fd = socket(domain, type, protocol);
    // 如果创建失败，则输出错误信息并返回 -1
    if (fd == -1) {
        printf("Failed to create a new socket\n");
        return -1;
    }
    // 将新创建的套接字设置为非阻塞模式
    int ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    // 如果设置失败，则关闭该套接字并返回 -1
    if (ret == -1) {
        close(ret);
        return -1;
    }
    // 设置套接字的 SO_REUSEADDR 选项，允许地址重用
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
    
    // 返回新创建的套接字描述符
    return fd;
}

// 该函数用于接受一个新的连接，并将新连接的套接字设置为非阻塞模式
// 参数 fd：表示监听套接字的描述符
// 参数 addr：指向 sockaddr 结构体的指针，用于存储客户端的地址信息
// 参数 len：指向 socklen_t 类型的指针，用于存储客户端地址信息的长度
// 返回值：返回新连接的套接字描述符，如果失败则返回 -1
int nty_accept(int fd, struct sockaddr *addr, socklen_t *len) {
    // 初始化新连接的套接字描述符为 -1
    int sockfd = -1;
    // 设置超时时间为 1 毫秒
    int timeout = 1;
    // 获取当前正在运行的协程
    nty_coroutine *co = nty_coroutine_get_sched()->curr_thread;
    
    // 循环等待新的连接
    while (1) {
        // 定义一个 pollfd 结构体变量 fds
        struct pollfd fds;
        // 将监听套接字的描述符赋值给 fds.fd
        fds.fd = fd;
        // 设置要监听的事件为 POLLIN、POLLERR 和 POLLHUP
        fds.events = POLLIN | POLLERR | POLLHUP;
        // 调用 nty_poll_inner 函数进行监听
        nty_poll_inner(&fds, 1, timeout);

        /*
        在这个函数中，接收连接之前调用nty_poll_inner，如果触发事件，加入ready队列并由run函数统一调用
        ，当resume的时候，就会回到inner函数中的nty_coroutine_yield(co); 
        继续执行并设置了fds里的fd的事件，然后会继续调用到sockfd = accept(fd, addr, len);建立连接吗
        */

        //建立连接的fd在nty_poll_inner中触发并设置
        // 调用系统的 accept 函数接受一个新的连接
        sockfd = accept(fd, addr, len);
        // 如果接受失败
        if (sockfd < 0) {
            // 如果错误码为 EAGAIN，则继续循环等待
            if (errno == EAGAIN) {
                continue;
            } 
            // 如果错误码为 ECONNABORTED，则输出错误信息
            else if (errno == ECONNABORTED) {
                printf("accept : ECONNABORTED\n");
                
            } 
            // 如果错误码为 EMFILE 或 ENFILE，则输出错误信息
            else if (errno == EMFILE || errno == ENFILE) {
                printf("accept : EMFILE || ENFILE\n");
            }
            // 返回 -1 表示接受失败
            return -1;
        } 
        // 如果接受成功，则跳出循环
        else {
            break;
        }
    }

    // 将新连接的套接字设置为非阻塞模式
    int ret = fcntl(sockfd, F_SETFL, O_NONBLOCK);
    // 如果设置失败，则关闭该套接字并返回 -1
    if (ret == -1) {
        close(sockfd);
        return -1;
    }
    // 设置监听套接字的 SO_REUSEADDR 选项，允许地址重用
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
    
    // 返回新连接的套接字描述符
    return sockfd;
}

// 该函数用于建立一个新的连接
// 参数 fd：表示套接字的描述符
// 参数 name：指向 sockaddr 结构体的指针，用于存储服务器的地址信息
// 参数 namelen：表示服务器地址信息的长度
// 返回值：返回连接结果，0 表示成功，-1 表示失败
int nty_connect(int fd, struct sockaddr *name, socklen_t namelen) {

    // 初始化连接结果为 0
    int ret = 0;

    // 循环尝试建立连接
    while (1) {

        // 定义一个 pollfd 结构体变量 fds
        struct pollfd fds;
        // 将套接字的描述符赋值给 fds.fd
        fds.fd = fd;
        // 设置要监听的事件为 POLLOUT、POLLERR 和 POLLHUP
        fds.events = POLLOUT | POLLERR | POLLHUP;
        // 调用 nty_poll_inner 函数进行监听
        nty_poll_inner(&fds, 1, 1);

        // 调用系统的 connect 函数建立连接
        ret = connect(fd, name, namelen);
        // 如果连接成功，则跳出循环
        if (ret == 0) break;

        // 如果连接失败，且错误码为 EAGAIN、EWOULDBLOCK 或 EINPROGRESS，则继续循环尝试
        if (ret == -1 && (errno == EAGAIN ||
            errno == EWOULDBLOCK || 
            errno == EINPROGRESS)) {
            continue;
        } 
        // 否则，跳出循环
        else {
            break;
        }
    }

    // 返回连接结果
    return ret;
}

// 功能：在协程环境下进行数据接收操作，使用 epoll 监听文件描述符上的可读事件
// 参数：
// fd: 要接收数据的文件描述符
// buf: 用于存储接收到的数据的缓冲区
// len: 缓冲区的长度
// flags: 接收数据的标志
ssize_t nty_recv(int fd, void *buf, size_t len, int flags) {
    // 定义一个 pollfd 结构体变量 fds，用于指定要监听的文件描述符和事件
    struct pollfd fds;
    // 将传入的文件描述符赋值给 fds.fd
    fds.fd = fd;
    // 设置要监听的事件为 POLLIN（可读事件）、POLLERR（错误事件）和 POLLHUP（挂断事件）
    fds.events = POLLIN | POLLERR | POLLHUP;

    // 调用 nty_poll_inner 函数，将文件描述符添加到 epoll 实例中进行监听
    // 并将当前协程挂起，等待事件发生
    nty_poll_inner(&fds, 1, 1);

    // 调用系统的 recv 函数进行数据接收
    int ret = recv(fd, buf, len, flags);
    // 检查 recv 函数的返回值
    if (ret < 0) {
        // 如果发生连接重置错误，返回 -1
        if (errno == ECONNRESET) return -1;
        // 其他错误情况可根据需要添加处理逻辑
        // printf("recv error : %d, ret : %d\n", errno, ret);
    }
    // 返回接收到的数据长度
    return ret;
}

// 功能：在协程环境下进行数据发送操作，使用 epoll 监听文件描述符上的可写事件
// 参数：
// fd: 要发送数据的文件描述符
// buf: 要发送的数据缓冲区
// len: 要发送的数据长度
// flags: 发送数据的标志
ssize_t nty_send(int fd, const void *buf, size_t len, int flags) {
    // 已发送的数据长度，初始化为 0
    int sent = 0;

    // 调用系统的 send 函数发送数据
    int ret = send(fd, ((char*)buf)+sent, len-sent, flags);
    // 检查 send 函数的返回值
    if (ret == 0) return ret;
    // 如果发送成功，更新已发送的数据长度
    if (ret > 0) sent += ret;

    // 循环发送数据，直到所有数据都发送完毕
    while (sent < len) {
        // 定义一个 pollfd 结构体变量 fds，用于指定要监听的文件描述符和事件
        struct pollfd fds;
        // 将传入的文件描述符赋值给 fds.fd
        fds.fd = fd;
        // 设置要监听的事件为 POLLOUT（可写事件）、POLLERR（错误事件）和 POLLHUP（挂断事件）
        fds.events = POLLOUT | POLLERR | POLLHUP;

        // 调用 nty_poll_inner 函数，将文件描述符添加到 epoll 实例中进行监听
        // 并将当前协程挂起，等待事件发生
        nty_poll_inner(&fds, 1, 1);
        // 继续调用系统的 send 函数发送剩余的数据
        ret = send(fd, ((char*)buf)+sent, len-sent, flags);
        // 打印发送的数据长度（可用于调试）
        // printf("send --> len : %d\n", ret);
        // 检查 send 函数的返回值
        if (ret <= 0) {
            // 如果发送失败，跳出循环
            break;
        }
        // 更新已发送的数据长度
        sent += ret;
    }

    // 如果发送失败且没有发送任何数据，返回 ret
    if (ret <= 0 && sent == 0) return ret;
    // 返回已发送的数据长度
    return sent;
}

// 功能：在协程环境下进行数据发送到指定地址的操作，使用 epoll 监听文件描述符上的可写事件
// 参数：
// fd: 要发送数据的文件描述符
// buf: 要发送的数据缓冲区
// len: 要发送的数据长度
// flags: 发送数据的标志
// dest_addr: 目标地址结构体指针
// addrlen: 目标地址结构体的长度
ssize_t nty_sendto(int fd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
    // 已发送的数据长度，初始化为 0
    int sent = 0;

    // 循环发送数据，直到所有数据都发送完毕
    while (sent < len) {
        // 定义一个 pollfd 结构体变量 fds，用于指定要监听的文件描述符和事件
        struct pollfd fds;
        // 将传入的文件描述符赋值给 fds.fd
        fds.fd = fd;
        // 设置要监听的事件为 POLLOUT（可写事件）、POLLERR（错误事件）和 POLLHUP（挂断事件）
        fds.events = POLLOUT | POLLERR | POLLHUP;

        // 调用 nty_poll_inner 函数，将文件描述符添加到 epoll 实例中进行监听
        // 并将当前协程挂起，等待事件发生
        nty_poll_inner(&fds, 1, 1);
        // 调用系统的 sendto 函数发送数据到指定地址
        int ret = sendto(fd, ((char*)buf)+sent, len-sent, flags, dest_addr, addrlen);
        // 检查 sendto 函数的返回值
        if (ret <= 0) {
            // 如果是 EAGAIN 错误，继续循环尝试发送
            if (errno == EAGAIN) continue;
            // 如果是连接重置错误，返回 ret
            else if (errno == ECONNRESET) {
                return ret;
            }
            // 其他错误情况，打印错误信息并断言
            printf("send errno : %d, ret : %d\n", errno, ret);
            assert(0);
        }
        // 更新已发送的数据长度
        sent += ret;
    }
    // 返回已发送的数据长度
    return sent;
}

// 功能：在协程环境下从指定地址接收数据，使用 epoll 监听文件描述符上的可读事件
// 参数：
// fd: 要接收数据的文件描述符
// buf: 用于存储接收到的数据的缓冲区
// len: 缓冲区的长度
// flags: 接收数据的标志
// src_addr: 源地址结构体指针
// addrlen: 源地址结构体的长度指针
ssize_t nty_recvfrom(int fd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    // 定义一个 pollfd 结构体变量 fds，用于指定要监听的文件描述符和事件
    struct pollfd fds;
    // 将传入的文件描述符赋值给 fds.fd
    fds.fd = fd;
    // 设置要监听的事件为 POLLIN（可读事件）、POLLERR（错误事件）和 POLLHUP（挂断事件）
    fds.events = POLLIN | POLLERR | POLLHUP;

    // 调用 nty_poll_inner 函数，将文件描述符添加到 epoll 实例中进行监听
    // 并将当前协程挂起，等待事件发生
    nty_poll_inner(&fds, 1, 1);

    // 调用系统的 recvfrom 函数从指定地址接收数据
    int ret = recvfrom(fd, buf, len, flags, src_addr, addrlen);
    // 检查 recvfrom 函数的返回值
    if (ret < 0) {
        // 如果是 EAGAIN 错误，返回 ret
        if (errno == EAGAIN) return ret;
        // 如果是连接重置错误，返回 0
        if (errno == ECONNRESET) return 0;
        // 其他错误情况，打印错误信息并断言
        printf("recv error : %d, ret : %d\n", errno, ret);
        assert(0);
    }
    // 返回接收到的数据长度
    return ret;
}

// 功能：在协程环境下关闭文件描述符
// 参数：
// fd: 要关闭的文件描述符
int nty_close(int fd) {
    // 调用系统的 close 函数关闭文件描述符
    return close(fd);
}

#ifdef  COROUTINE_HOOK

socket_t socket_f = NULL;

read_t read_f = NULL;
recv_t recv_f = NULL;
recvfrom_t recvfrom_f = NULL;

write_t write_f = NULL;
send_t send_f = NULL;
sendto_t sendto_f = NULL;

accept_t accept_f = NULL;
close_t close_f = NULL;
connect_t connect_f = NULL;


int init_hook(void) {
	socket_f = (socket_t)dlsym(RTLD_NEXT, "socket");
	
    read_f = (read_t)dlsym(RTLD_NEXT, "read");
	recv_f = (recv_t)dlsym(RTLD_NEXT, "recv");
	recvfrom_f = (recvfrom_t)dlsym(RTLD_NEXT, "recvfrom");

	write_f = (write_t)dlsym(RTLD_NEXT, "write");
	send_f = (send_t)dlsym(RTLD_NEXT, "send");
    sendto_f = (sendto_t)dlsym(RTLD_NEXT, "sendto");

	accept_f = (accept_t)dlsym(RTLD_NEXT, "accept");
	close_f = (close_t)dlsym(RTLD_NEXT, "close");
	connect_f = (connect_t)dlsym(RTLD_NEXT, "connect");

    return 0;
}

// 功能：创建一个新的套接字，若调度器存在则将其设置为非阻塞并允许地址重用
// 参数：
// domain: 协议族，如 AF_INET 表示 IPv4
// type: 套接字类型，如 SOCK_STREAM 表示 TCP 流套接字
// protocol: 协议，通常为 0 表示使用默认协议
int socket(int domain, int type, int protocol) {
    // 若全局的 socket 函数指针未初始化，则调用 init_hook 函数进行初始化
    if (!socket_f) init_hook();

    // 获取当前协程的调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    // 若调度器为空，直接调用原始的 socket 函数
    if (sched == NULL) {
        return socket_f(domain, type, protocol);
    }

    // 调用原始的 socket 函数创建套接字
    int fd = socket_f(domain, type, protocol);
    // 若创建失败，打印错误信息并返回 -1
    if (fd == -1) {
        printf("Failed to create a new socket\n");
        return -1;
    }

    // 将套接字设置为非阻塞模式
    int ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    // 若设置失败，关闭套接字并返回 -1
    if (ret == -1) {
        close(ret);
        return -1;
    }

    // 设置套接字允许地址重用
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));

    // 返回创建好的套接字描述符
    return fd;
}

// 功能：从文件描述符中读取数据，若调度器存在则先使用 epoll 监听可读事件
// 参数：
// fd: 要读取数据的文件描述符
// buf: 用于存储读取数据的缓冲区
// count: 要读取的字节数
ssize_t read(int fd, void *buf, size_t count) {
    // 若全局的 read 函数指针未初始化，则调用 init_hook 函数进行初始化
    if (!read_f) init_hook();

    // 获取当前协程的调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    // 若调度器为空，直接调用原始的 read 函数
    if (sched == NULL) {
        return read_f(fd, buf, count);
    }

    // 定义一个 pollfd 结构体，用于指定要监听的文件描述符和事件
    struct pollfd fds;
    fds.fd = fd;
    fds.events = POLLIN | POLLERR | POLLHUP;

    // 调用 nty_poll_inner 函数，将文件描述符添加到 epoll 实例中进行监听
    nty_poll_inner(&fds, 1, 1);

    // 调用原始的 read 函数读取数据
    int ret = read_f(fd, buf, count);
    // 若读取失败且错误码为 ECONNRESET，返回 -1
    if (ret < 0) {
        if (errno == ECONNRESET) return -1;
    }

    // 返回读取的字节数
    return ret;
}

// 功能：从套接字接收数据，若调度器存在则先使用 epoll 监听可读事件
// 参数：
// fd: 要接收数据的套接字描述符
// buf: 用于存储接收数据的缓冲区
// len: 缓冲区的长度
// flags: 接收数据的标志
ssize_t recv(int fd, void *buf, size_t len, int flags) {
    // 若全局的 recv 函数指针未初始化，则调用 init_hook 函数进行初始化
    if (!recv_f) init_hook();

    // 获取当前协程的调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    // 若调度器为空，直接调用原始的 recv 函数
    if (sched == NULL) {
        return recv_f(fd, buf, len, flags);
    }

    // 定义一个 pollfd 结构体，用于指定要监听的文件描述符和事件
    struct pollfd fds;
    fds.fd = fd;
    fds.events = POLLIN | POLLERR | POLLHUP;

    // 调用 nty_poll_inner 函数，将文件描述符添加到 epoll 实例中进行监听
    nty_poll_inner(&fds, 1, 1);

    // 调用原始的 recv 函数接收数据
    int ret = recv_f(fd, buf, len, flags);
    // 若接收失败且错误码为 ECONNRESET，返回 -1
    if (ret < 0) {
        if (errno == ECONNRESET) return -1;
    }

    // 返回接收的字节数
    return ret;
}

// 功能：从套接字接收数据并获取发送方地址，若调度器存在则先使用 epoll 监听可读事件
// 参数：
// fd: 要接收数据的套接字描述符
// buf: 用于存储接收数据的缓冲区
// len: 缓冲区的长度
// flags: 接收数据的标志
// src_addr: 用于存储发送方地址的结构体指针
// addrlen: 发送方地址结构体的长度指针
ssize_t recvfrom(int fd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    // 若全局的 recvfrom 函数指针未初始化，则调用 init_hook 函数进行初始化
    if (!recvfrom_f) init_hook();

    // 获取当前协程的调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    // 若调度器为空，直接调用原始的 recvfrom 函数
    if (sched == NULL) {
        return recvfrom_f(fd, buf, len, flags, src_addr, addrlen);
    }

    // 定义一个 pollfd 结构体，用于指定要监听的文件描述符和事件
    struct pollfd fds;
    fds.fd = fd;
    fds.events = POLLIN | POLLERR | POLLHUP;

    // 调用 nty_poll_inner 函数，将文件描述符添加到 epoll 实例中进行监听
    nty_poll_inner(&fds, 1, 1);

    // 调用原始的 recvfrom 函数接收数据
    int ret = recvfrom_f(fd, buf, len, flags, src_addr, addrlen);
    // 若接收失败，根据不同的错误码进行处理
    if (ret < 0) {
        if (errno == EAGAIN) return ret;
        if (errno == ECONNRESET) return 0;
        // 打印错误信息并断言
        printf("recv error : %d, ret : %d\n", errno, ret);
        assert(0);
    }

    // 返回接收的字节数
    return ret;
}

// 功能：向文件描述符写入数据，若调度器存在则先使用 epoll 监听可写事件
// 参数：
// fd: 要写入数据的文件描述符
// buf: 要写入的数据缓冲区
// count: 要写入的字节数
ssize_t write(int fd, const void *buf, size_t count) {
    // 若全局的 write 函数指针未初始化，则调用 init_hook 函数进行初始化
    if (!write_f) init_hook();

    // 获取当前协程的调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    // 若调度器为空，直接调用原始的 write 函数
    if (sched == NULL) {
        return write_f(fd, buf, count);
    }

    // 已发送的字节数，初始化为 0
    int sent = 0;

    // 调用原始的 write 函数写入数据
    int ret = write_f(fd, ((char*)buf)+sent, count-sent);
    // 若写入成功，更新已发送的字节数
    if (ret == 0) return ret;
    if (ret > 0) sent += ret;

    // 循环写入数据，直到所有数据都发送完毕
    while (sent < count) {
        // 定义一个 pollfd 结构体，用于指定要监听的文件描述符和事件
        struct pollfd fds;
        fds.fd = fd;
        fds.events = POLLOUT | POLLERR | POLLHUP;

        // 调用 nty_poll_inner 函数，将文件描述符添加到 epoll 实例中进行监听
        nty_poll_inner(&fds, 1, 1);
        // 继续调用原始的 write 函数写入剩余的数据
        ret = write_f(fd, ((char*)buf)+sent, count-sent);
        // 若写入失败，跳出循环
        if (ret <= 0) {
            break;
        }
        // 更新已发送的字节数
        sent += ret;
    }

    // 若写入失败且没有发送任何数据，返回 ret
    if (ret <= 0 && sent == 0) return ret;

    // 返回已发送的字节数
    return sent;
}

// 功能：向套接字发送数据，若调度器存在则先使用 epoll 监听可写事件
// 参数：
// fd: 要发送数据的套接字描述符
// buf: 要发送的数据缓冲区
// len: 要发送的数据长度
// flags: 发送数据的标志
ssize_t send(int fd, const void *buf, size_t len, int flags) {
    // 若全局的 send 函数指针未初始化，则调用 init_hook 函数进行初始化
    if (!send_f) init_hook();

    // 获取当前协程的调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    // 若调度器为空，直接调用原始的 send 函数
    if (sched == NULL) {
        return send_f(fd, buf, len, flags);
    }

    // 已发送的字节数，初始化为 0
    int sent = 0;

    // 调用原始的 send 函数发送数据
    int ret = send_f(fd, ((char*)buf)+sent, len-sent, flags);
    // 若发送成功，更新已发送的字节数
    if (ret == 0) return ret;
    if (ret > 0) sent += ret;

    // 循环发送数据，直到所有数据都发送完毕
    while (sent < len) {
        // 定义一个 pollfd 结构体，用于指定要监听的文件描述符和事件
        struct pollfd fds;
        fds.fd = fd;
        fds.events = POLLOUT | POLLERR | POLLHUP;

        // 调用 nty_poll_inner 函数，将文件描述符添加到 epoll 实例中进行监听
        nty_poll_inner(&fds, 1, 1);
        // 继续调用原始的 send 函数发送剩余的数据
        ret = send_f(fd, ((char*)buf)+sent, len-sent, flags);
        // 若发送失败，跳出循环
        if (ret <= 0) {
            break;
        }
        // 更新已发送的字节数
        sent += ret;
    }

    // 若发送失败且没有发送任何数据，返回 ret
    if (ret <= 0 && sent == 0) return ret;

    // 返回已发送的字节数
    return sent;
}

// 功能：向指定地址的套接字发送数据，若调度器存在则先使用 epoll 监听可写事件
// 参数：
// sockfd: 要发送数据的套接字描述符
// buf: 要发送的数据缓冲区
// len: 要发送的数据长度
// flags: 发送数据的标志
// dest_addr: 目标地址结构体指针
// addrlen: 目标地址结构体的长度
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
        const struct sockaddr *dest_addr, socklen_t addrlen) {
    // 若全局的 sendto 函数指针未初始化，则调用 init_hook 函数进行初始化
    if (!sendto_f) init_hook();

    // 获取当前协程的调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    // 若调度器为空，直接调用原始的 sendto 函数
    if (sched == NULL) {
        return sendto_f(sockfd, buf, len, flags, dest_addr, addrlen);
    }

    // 定义一个 pollfd 结构体，用于指定要监听的文件描述符和事件
    struct pollfd fds;
    fds.fd = sockfd;
    fds.events = POLLOUT | POLLERR | POLLHUP;

    // 调用 nty_poll_inner 函数，将文件描述符添加到 epoll 实例中进行监听
    nty_poll_inner(&fds, 1, 1);

    // 调用原始的 sendto 函数发送数据
    int ret = sendto_f(sockfd, buf, len, flags, dest_addr, addrlen);
    // 若发送失败，根据不同的错误码进行处理
    if (ret < 0) {
        if (errno == EAGAIN) return ret;
        if (errno == ECONNRESET) return 0;
        // 打印错误信息并断言
        printf("recv error : %d, ret : %d\n", errno, ret);
        assert(0);
    }

    // 返回发送的字节数
    return ret;
}

// 功能：接受一个新的连接，若调度器存在则先使用 epoll 监听可读事件
// 参数：
// fd: 监听套接字描述符
// addr: 用于存储客户端地址的结构体指针
// len: 客户端地址结构体的长度指针
int accept(int fd, struct sockaddr *addr, socklen_t *len) {
    // 若全局的 accept 函数指针未初始化，则调用 init_hook 函数进行初始化
    if (!accept_f) init_hook();

    // 获取当前协程的调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    // 若调度器为空，直接调用原始的 accept 函数
    if (sched == NULL) {
        return accept_f(fd, addr, len);
    }

    // 新连接的套接字描述符，初始化为 -1
    int sockfd = -1;
    // 超时时间，设置为 1
    int timeout = 1;
    // 获取当前协程
    nty_coroutine *co = nty_coroutine_get_sched()->curr_thread;

    // 循环接受连接，直到成功或出现错误
    while (1) {
        // 定义一个 pollfd 结构体，用于指定要监听的文件描述符和事件
        struct pollfd fds;
        fds.fd = fd;
        fds.events = POLLIN | POLLERR | POLLHUP;

        // 调用 nty_poll_inner 函数，将文件描述符添加到 epoll 实例中进行监听
        nty_poll_inner(&fds, 1, timeout);

        // 调用原始的 accept 函数接受新连接
        sockfd = accept_f(fd, addr, len);
        // 若接受失败，根据不同的错误码进行处理
        if (sockfd < 0) {
            if (errno == EAGAIN) {
                continue;
            } else if (errno == ECONNABORTED) {
                printf("accept : ECONNABORTED\n");
            } else if (errno == EMFILE || errno == ENFILE) {
                printf("accept : EMFILE || ENFILE\n");
            }
            return -1;
        } else {
            break;
        }
    }

    // 将新连接的套接字设置为非阻塞模式
    int ret = fcntl(sockfd, F_SETFL, O_NONBLOCK);
    // 若设置失败，关闭套接字并返回 -1
    if (ret == -1) {
        close(sockfd);
        return -1;
    }

    // 设置监听套接字允许地址重用
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));

    // 返回新连接的套接字描述符
    return sockfd;
}

//关闭连接
int close(int fd) {

	if (!close_f) init_hook();

	return close_f(fd);
}


//客户端连接
int connect(int fd, const struct sockaddr *addr, socklen_t addrlen) {

	if (!connect_f) init_hook();

	nty_schedule *sched = nty_coroutine_get_sched();
	if (sched == NULL) {
		return connect_f(fd, addr, addrlen);
	}

	int ret = 0;

	while (1) {

		struct pollfd fds;
		fds.fd = fd;
		fds.events = POLLOUT | POLLERR | POLLHUP;
		nty_poll_inner(&fds, 1, 1);

		ret = connect_f(fd, addr, addrlen);
		if (ret == 0) break;

		if (ret == -1 && (errno == EAGAIN ||
			errno == EWOULDBLOCK || 
			errno == EINPROGRESS)) {
			continue;
		} else {
			break;
		}
	}

	return ret;
}

#endif