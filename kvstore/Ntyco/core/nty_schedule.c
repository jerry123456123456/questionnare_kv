#include "nty_coroutine.h"

// 宏定义，用于生成一个 64 位的键，将文件描述符 f 和事件 e 组合在一起
#define FD_KEY(f,e) (((int64_t)(f) << (sizeof(int32_t) * 8)) | e)
// 宏定义，用于从 64 位键中提取事件部分
#define FD_EVENT(f) ((int32_t)(f))
// 宏定义，用于从 64 位键中提取文件描述符部分
#define FD_ONLY(f) ((f) >> ((sizeof(int32_t) * 8)))

//依据时间比较红黑树的键
static inline int nty_coroutine_sleep_cmp(nty_coroutine *co1, nty_coroutine *co2) {
	if (co1->sleep_usecs < co2->sleep_usecs) {
		return -1;
	}
	if (co1->sleep_usecs == co2->sleep_usecs) {
		return 0;
	}
	return 1;
}

/**
 * @brief 比较两个协程等待的文件描述符
 * @param co1 第一个协程指针
 * @param co2 第二个协程指针
 * @return 如果 co1 等待的文件描述符小于 co2 等待的文件描述符，返回 -1；如果相等，返回 0；否则返回 1
 */
static inline int nty_coroutine_wait_cmp(nty_coroutine *co1, nty_coroutine *co2) {
#if CANCEL_FD_WAIT_UINT64
    if (co1->fd < co2->fd) return -1;
    else if (co1->fd == co2->fd) return 0;
    else return 1;
#else
    if (co1->fd_wait < co2->fd_wait) {
        return -1;
    }
    if (co1->fd_wait == co2->fd_wait) {
        return 0;
    }
#endif
    return 1;
}

//优雅！！！
// 生成红黑树相关的操作函数，用于管理睡眠中的协程,最后一个参数是比较规则，也就是键的规则
RB_GENERATE(_nty_coroutine_rbtree_sleep, _nty_coroutine, sleep_node, nty_coroutine_sleep_cmp);
// 生成红黑树相关的操作函数，用于管理等待中的协程
RB_GENERATE(_nty_coroutine_rbtree_wait, _nty_coroutine, wait_node, nty_coroutine_wait_cmp);

/**
 * @brief 将协程放入睡眠队列
 * @param co 要放入睡眠队列的协程指针
 * @param msecs 协程要睡眠的毫秒数
 */
void nty_schedule_sched_sleepdown(nty_coroutine *co, uint64_t msecs) {
    // 将毫秒转换为微秒
    uint64_t usecs = msecs * 1000u;

    // 在睡眠红黑树中查找该协程
    //第一个参数是名字，第二个是根节点，第三个是查找的元素
    nty_coroutine *co_tmp = RB_FIND(_nty_coroutine_rbtree_sleep, &co->sched->sleeping, co);
    if (co_tmp != NULL) {
        // 如果找到，从睡眠红黑树中移除该协程
        RB_REMOVE(_nty_coroutine_rbtree_sleep, &co->sched->sleeping, co_tmp);
    }

    // 计算协程应该唤醒的时间,这个函数功能是第二个参数 - 第一个参数
    co->sleep_usecs = nty_coroutine_diff_usecs(co->sched->birth, nty_coroutine_usec_now()) + usecs;

    while (msecs) {
        // 将协程插入到睡眠红黑树中
        co_tmp = RB_INSERT(_nty_coroutine_rbtree_sleep, &co->sched->sleeping, co);
        if (co_tmp) {
            // 如果插入失败，说明有冲突，增加睡眠时间并重试
            printf("1111 sleep_usecs %"PRIu64"\n", co->sleep_usecs);
            co->sleep_usecs ++;
            continue;
        }
        // 插入成功，设置协程状态为睡眠状态
        co->status |= BIT(NTY_COROUTINE_STATUS_SLEEPING);
        break;
    }
}

/**
 * @brief 将协程从睡眠队列中移除
 * @param co 要从睡眠队列中移除的协程指针
 */
void nty_schedule_desched_sleepdown(nty_coroutine *co) {
    if (co->status & BIT(NTY_COROUTINE_STATUS_SLEEPING)) {
        // 如果协程处于睡眠状态，从睡眠红黑树中移除该协程
        RB_REMOVE(_nty_coroutine_rbtree_sleep, &co->sched->sleeping, co);

        // 清除睡眠状态标志
        co->status &= CLEARBIT(NTY_COROUTINE_STATUS_SLEEPING);
        // 设置协程状态为就绪状态
        co->status |= BIT(NTY_COROUTINE_STATUS_READY);
        // 清除过期状态标志
        co->status &= CLEARBIT(NTY_COROUTINE_STATUS_EXPIRED);
    }
}

/**
 * @brief 根据文件描述符在等待红黑树中查找协程
 * @param fd 要查找的文件描述符
 * @return 找到的协程指针，如果未找到则返回 NULL
 */
nty_coroutine *nty_schedule_search_wait(int fd) {
    // 创建一个临时协程对象，用于查找
    //这里使用对象，一是简化代码，二是分配在栈无需手动释放
    nty_coroutine find_it = {0};
    find_it.fd = fd;

    // 获取当前的协程调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    
    // 在等待红黑树中查找该协程
    //这里用结构体指针而不是对象，一是对象无法表示NULL，二是无需拷贝
    nty_coroutine *co = RB_FIND(_nty_coroutine_rbtree_wait, &sched->waiting, &find_it);
    // 清除协程状态
    co->status = 0;

    return co;
}

/**
 * @brief 根据文件描述符从等待红黑树中移除协程
 * @param fd 要移除协程对应的文件描述符
 * @return 移除的协程指针，如果未找到则返回 NULL
 */
nty_coroutine* nty_schedule_desched_wait(int fd) {
    // 创建一个临时协程对象，用于查找
    nty_coroutine find_it = {0};
    find_it.fd = fd;

    // 获取当前的协程调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    
    // 在等待红黑树中查找该协程
    nty_coroutine *co = RB_FIND(_nty_coroutine_rbtree_wait, &sched->waiting, &find_it);
    if (co != NULL) {
        // 如果找到，从等待红黑树中移除该协程
        RB_REMOVE(_nty_coroutine_rbtree_wait, &co->sched->waiting, co);
    }
    // 清除协程状态
    co->status = 0;
    // 将协程从睡眠队列中移除
    nty_schedule_desched_sleepdown(co);
    
    return co;
}

/**
 * @brief 将协程放入等待队列和睡眠队列
 * @param co 要放入等待队列和睡眠队列的协程指针
 * @param fd 协程等待的文件描述符
 * @param events 协程等待的事件
 * @param timeout 超时时间
 */
//将协程同时加入等待队列和睡眠队列，主要是为了实现超时机制。协程在等待特定文件描述符的事件时，可能会出现长时间无响应的情况。通过将协程加入睡眠队列，能够设定一个超时时间，当超时时间到达时，协程会被唤醒，从而避免无限期等待
void nty_schedule_sched_wait(nty_coroutine *co, int fd, unsigned short events, uint64_t timeout) {
    // 检查协程是否已经处于等待读或等待写状态
    if (co->status & BIT(NTY_COROUTINE_STATUS_WAIT_READ) ||
        co->status & BIT(NTY_COROUTINE_STATUS_WAIT_WRITE)) {
        printf("Unexpected event. lt id %"PRIu64" fd %"PRId32" already in %"PRId32" state\n",
            co->id, co->fd, co->status);
        assert(0);
    }

    if (events & POLLIN) {
        // 如果事件包含 POLLIN，设置协程状态为等待读
        co->status |= NTY_COROUTINE_STATUS_WAIT_READ;
    } else if (events & POLLOUT) {
        // 如果事件包含 POLLOUT，设置协程状态为等待写
        co->status |= NTY_COROUTINE_STATUS_WAIT_WRITE;
    } else {
        printf("events : %d\n", events);
        assert(0);
    }

    // 设置协程等待的文件描述符和事件
    co->fd = fd;
    co->events = events;
    // 将协程插入到等待红黑树中
    nty_coroutine *co_tmp = RB_INSERT(_nty_coroutine_rbtree_wait, &co->sched->waiting, co);

    assert(co_tmp == NULL);

    // 如果超时时间为 1，认为是错误情况，直接返回
    if (timeout == 1) return ; 

    // 将协程放入睡眠队列
    nty_schedule_sched_sleepdown(co, timeout);
}

/**
 * @brief 取消协程的等待状态
 * @param co 要取消等待状态的协程指针
 */
void nty_schedule_cancel_wait(nty_coroutine *co) {
    // 从等待红黑树中移除该协程
    RB_REMOVE(_nty_coroutine_rbtree_wait, &co->sched->waiting, co);
}

/*
功能：释放调度器结构体及其相关资源，包括关闭文件描述符和释放内存。
参数：
sched：指向 nty_schedule 结构体的指针，代表要释放的调度器。
*/
void nty_schedule_free(nty_schedule *sched) {
    // 检查 poller_fd 是否大于 0，如果是则关闭它
    if (sched->poller_fd > 0) {
        close(sched->poller_fd);
    }
    // 检查 eventfd 是否大于 0，如果是则关闭它
    if (sched->eventfd > 0) {
        close(sched->eventfd);
    }
    // 检查 stack 是否不为 NULL，如果是则释放该内存
    if (sched->stack != NULL) {
        free(sched->stack);
    }
    // 释放调度器结构体占用的内存
    free(sched);
    // 将全局调度器键对应的线程特定数据设置为 NULL
    assert(pthread_setspecific(global_sched_key, NULL) == 0);
}

/*
功能：创建并初始化一个调度器结构体，包括分配内存、创建 epoller、初始化数据结构等。
参数：
stack_size：指定调度器使用的栈大小，如果为 0 则使用默认值。
*/
int nty_schedule_create(int stack_size) {
    // 如果传入的 stack_size 为 0，则使用默认的最大栈大小
    int sched_stack_size = stack_size ? stack_size : NTY_CO_MAX_STACKSIZE;
    // 为调度器结构体分配内存并初始化为 0
    nty_schedule *sched = (nty_schedule*)calloc(1, sizeof(nty_schedule));
    if (sched == NULL) {
        // 内存分配失败，输出错误信息并返回 -1
        printf("Failed to initialize scheduler\n");
        return -1;
    }
    // 将当前线程的特定数据设置为新创建的调度器
    assert(pthread_setspecific(global_sched_key, sched) == 0);
    // 创建 epoller 实例，获取其文件描述符
    sched->poller_fd = nty_epoller_create();
    if (sched->poller_fd == -1) {
        // epoller 创建失败，输出错误信息，释放已分配的调度器内存并返回 -2
        printf("Failed to initialize epoller\n");
        nty_schedule_free(sched);
        return -2;
    }
    // 注册 epoller 事件触发
    nty_epoller_ev_register_trigger();
    // 设置调度器的栈大小
    sched->stack_size = sched_stack_size;
    // 获取系统的页面大小
    sched->page_size = getpagesize();
    //ucontext调度器中统一管理栈，而不使用则需要各自管理栈
#ifdef _USE_UCONTEXT
    // 如果使用 ucontext 机制，使用 posix_memalign 分配对齐的内存作为栈
    int ret = posix_memalign(&sched->stack, sched->page_size, sched->stack_size);
    assert(ret == 0);
#else
    // 不使用 ucontext 机制，将栈指针置为 NULL，并清空 CPU 上下文
    sched->stack = NULL;
    bzero(&sched->ctx, sizeof(nty_cpu_ctx));
#endif
    // 初始化已生成的协程数量为 0
    sched->spawned_coroutines = 0;
    // 设置默认的超时时间
    sched->default_timeout = 3000000u;
    // 初始化睡眠红黑树
    RB_INIT(&sched->sleeping);
    // 初始化等待红黑树
    RB_INIT(&sched->waiting);
    // 记录调度器的创建时间
    sched->birth = nty_coroutine_usec_now();
    // 初始化就绪队列
    //存储准备好可以立即执行的协程（nty_coroutine）。当协程的等待条件（如定时器到期、IO 事件就绪）满足时，会被加入此队列，等待调度器分配执行时间。
    /*
    假设一个协程 co1 在等待网络套接字 fd=10 的读事件：
    co1 调用 read 时发现数据未就绪，主动挂起，注册到 epoll 等待读事件，此时不在 ready 队列。
    当 fd=10 有数据可读时，epoll 触发事件，调度器将 co1 加入 ready 队列。
    调度器遍历 ready 队列时，取出 co1 并恢复执行，此时 co1 可以读取数据。
    */
    TAILQ_INIT(&sched->ready);
    // 初始化延迟队列
    //存储需要延迟到下一个调度周期处理的协程。通常用于避免在当前调度循环中重复处理同一协程，或在当前操作完成后再执行（比如协程主动请求延迟执行）
    /*
    假设协程 co2 处理完一次网络请求后，需要等待一个异步回调，但不希望立即重新调度：
    co2 完成当前逻辑后，主动将自己加入 defer 队列。
    调度器处理完所有 ready 队列中的协程后，在下一个循环周期将 defer 中的协程移动到 ready 队列或直接执行。
    */
    TAILQ_INIT(&sched->defer);
    // 初始化忙碌列表
    //存储正在执行或即将执行的协程，用于标记协程处于 “忙碌” 状态，避免被重复调度或错误释放
    /*
    假设协程 co3 正在执行用户逻辑：
    co3 从 ready 队列取出后，被加入 busy 列表。
    若 co3 在执行过程中主动挂起（如等待新的 IO 事件），则从 busy 列表移除，并根据等待条件加入其他队列（如 epoll 等待队列或睡眠红黑树）。
    若 co3 执行完毕（状态标记为结束），则从 busy 列表移除并释放资源。
    */
    LIST_INIT(&sched->busy);

    return 0;
}

/*
功能：检查睡眠红黑树中是否有协程的睡眠时间已经到期，如果有则将其从树中移除并返回该协程的指针。
参数：
sched：指向 nty_schedule 结构体的指针，代表要检查的调度器。
*/
static nty_coroutine *nty_schedule_expired(nty_schedule *sched) {
    // 计算从调度器创建到现在的时间差（微秒）
    uint64_t t_diff_usecs = nty_coroutine_diff_usecs(sched->birth, nty_coroutine_usec_now());
    // 从睡眠红黑树中获取最小的协程节点
    nty_coroutine *co = RB_MIN(_nty_coroutine_rbtree_sleep, &sched->sleeping);
    if (co == NULL) {
        // 睡眠红黑树为空，返回 NULL
        return NULL;
    }
    // 检查该协程的睡眠时间是否已经到期
    if (co->sleep_usecs <= t_diff_usecs) {
        // 睡眠时间到期，从睡眠红黑树中移除该协程
        RB_REMOVE(_nty_coroutine_rbtree_sleep, &co->sched->sleeping, co);
        return co;
    }
    // 睡眠时间未到期，返回 NULL
    return NULL;
}

/*
功能：判断调度器是否已经完成所有任务，即所有数据结构都为空。
参数：
sched：指向 nty_schedule 结构体的指针，代表要检查的调度器。
*/
static inline int nty_schedule_isdone(nty_schedule *sched) {
    // 检查等待红黑树、忙碌列表、睡眠红黑树和就绪队列是否都为空
    return (RB_EMPTY(&sched->waiting) && 
            LIST_EMPTY(&sched->busy) &&
            RB_EMPTY(&sched->sleeping) &&
            TAILQ_EMPTY(&sched->ready));
}

/*
功能：计算睡眠红黑树中最小协程的剩余睡眠时间，如果没有协程则返回默认超时时间。
参数：
sched：指向 nty_schedule 结构体的指针，代表要检查的调度器。
*/
static uint64_t nty_schedule_min_timeout(nty_schedule *sched) {
    // 计算从调度器创建到现在的时间差（微秒）
    uint64_t t_diff_usecs = nty_coroutine_diff_usecs(sched->birth, nty_coroutine_usec_now());
    // 获取调度器的默认超时时间
    uint64_t min = sched->default_timeout;
    // 从睡眠红黑树中获取最小的协程节点
    nty_coroutine *co = RB_MIN(_nty_coroutine_rbtree_sleep, &sched->sleeping);
    if (!co) {
        // 睡眠红黑树为空，返回默认超时时间
        return min;
    }
    // 获取最小协程的睡眠时间
    min = co->sleep_usecs;
    if (min > t_diff_usecs) {
        // 睡眠时间大于当前时间差，返回剩余的睡眠时间
        return min - t_diff_usecs;
    }
    // 睡眠时间已经到期，返回 0
    return 0;
} 

/*
功能：使用 epoller 等待事件，根据最小超时时间设置等待时间，并处理可能的中断。
参数：
sched：指向 nty_schedule 结构体的指针，代表要操作的调度器。
*/
static int nty_schedule_epoll(nty_schedule *sched) {
    // 初始化新事件数量为 0
    sched->num_new_events = 0;
    // 初始化时间结构体
    struct timespec t = {0, 0};
    // 获取最小的超时时间
    uint64_t usecs = nty_schedule_min_timeout(sched);
    if (usecs && TAILQ_EMPTY(&sched->ready)) {
        // 如果有超时时间且就绪队列为空，设置时间结构体
        t.tv_sec = usecs / 1000000u;
        if (t.tv_sec != 0) {
            t.tv_nsec = (usecs % 1000u) * 1000u;
        } else {
            t.tv_nsec = usecs * 1000u;
        }
    } else {
        // 没有超时时间或就绪队列不为空，直接返回 0
        return 0;
    }
    int nready = 0;
    while (1) {
        // 调用 epoller 等待事件，返回就绪的文件描述符数量
        nready = nty_epoller_wait(t);
        if (nready == -1) {
            if (errno == EINTR) {
                // 如果是被信号中断，继续等待
                continue;
            } else {
                // 其他错误，终止程序
                assert(0);
            }
        }
        break;
    }
    // 初始化事件数量为 0
    sched->nevents = 0;
    // 记录新事件的数量
    sched->num_new_events = nready;
    return 0;
}

/*
功能：启动调度器的运行，循环处理睡眠红黑树、就绪队列和等待红黑树中的协程，直到所有任务完成，最后释放调度器。
参数：无
*/
void nty_schedule_run(void) {
    // 获取当前线程的调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    if (sched == NULL) {
        // 调度器为空，直接返回
        return;
    }
    // 循环直到调度器完成所有任务
    while (!nty_schedule_isdone(sched)) {
        // 1. 处理睡眠红黑树中到期的协程
        nty_coroutine *expired = NULL;
        while ((expired = nty_schedule_expired(sched)) != NULL) {
            // 恢复到期协程的执行
            nty_coroutine_resume(expired);
        }
        // 2. 处理就绪队列中的协程
        nty_coroutine *last_co_ready = TAILQ_LAST(&sched->ready, _nty_coroutine_queue);
        while (!TAILQ_EMPTY(&sched->ready)) {
            // 获取就绪队列的第一个协程
            nty_coroutine *co = TAILQ_FIRST(&sched->ready);
            // 从就绪队列中移除该协程
            TAILQ_REMOVE(&co->sched->ready, co, ready_next);
            if (co->status & BIT(NTY_COROUTINE_STATUS_FDEOF)) {
                // 如果协程的状态为文件结束，释放该协程
                nty_coroutine_free(co);
                break;
            }
            // 恢复协程的执行
            nty_coroutine_resume(co);
            if (co == last_co_ready) {
                // 如果是最后一个协程，跳出循环
                break;
            }
        }
        // 3. 处理等待红黑树中的协程
        nty_schedule_epoll(sched);
        while (sched->num_new_events) {
            // 获取新事件的索引
            int idx = --sched->num_new_events;
            // 获取对应的 epoll 事件
            struct epoll_event *ev = sched->eventlist + idx;
            // 获取事件对应的文件描述符
            int fd = ev->data.fd;
            // 检查是否为文件结束事件
            int is_eof = ev->events & EPOLLHUP;
            if (is_eof) {
                // 如果是文件结束事件，设置错误号
                errno = ECONNRESET;
            }
            // 根据文件描述符查找等待的协程
            nty_coroutine *co = nty_schedule_search_wait(fd);
            if (co != NULL) {
                if (is_eof) {
                    // 如果是文件结束事件，设置协程的状态为文件结束
                    co->status |= BIT(NTY_COROUTINE_STATUS_FDEOF);
                }
                // 恢复协程的执行
                nty_coroutine_resume(co);
            }
            is_eof = 0;
        }
    }
    // 释放调度器
    nty_schedule_free(sched);
    return;
}