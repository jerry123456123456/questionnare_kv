#include "nty_coroutine.h"

// 定义一个线程特定数据键，用于存储调度器指针
pthread_key_t global_sched_key;
// 用于确保调度器键的创建函数只被调用一次
/*
pthread_once_t 是一个数据类型，用于表示一次性初始化的控制变量。PTHREAD_ONCE_INIT 是一个宏，用于初始化 pthread_once_t 变量。
pthread_once 函数可以确保指定的初始化函数只被调用一次，无论有多少个线程同时调用 pthread_once 函数
*/
static pthread_once_t sched_key_once = PTHREAD_ONCE_INIT;


#ifdef _USE_UCONTEXT
// 当使用 ucontext 机制时，保存协程的栈数据
/*
_save_stack(nty_coroutine *co)
功能：当使用 ucontext 机制时，保存协程的栈数据。
参数：co 为要保存栈数据的协程指针。
*/
static void _save_stack(nty_coroutine *co){
    //计算栈的最高地址的指针(栈底),把当前调度器的栈信息保存到自己的栈信息中
    char *top = co->sched->stack + co->sched->stack_size;
    // 定义一个临时变量，用于确定当前栈指针位置
    char dummy = 0;
    // 确保栈的大小不超过最大限制
    //dummy分配在栈中，&取地址就是当前栈顶指针的地址
    assert(top - &dummy <= NTY_CO_MAX_STACKSIZE);
    // 如果当前栈大小小于实际使用的栈大小
    if (co->stack_size < top - &dummy) {
        // 重新分配内存以容纳更大的栈
        co->stack = realloc(co->stack, top - &dummy);
        // 确保内存分配成功
        assert(co->stack != NULL);
    }
    // 更新栈的大小
    co->stack_size = top - &dummy;
    // 将当前栈的数据复制到协程的栈缓冲区
    memcpy(co->stack, &dummy, co->stack_size);
}

/*
_load_stack(nty_coroutine *co)
功能：当使用 ucontext 机制时，加载协程的栈数据。
参数：co 为要加载栈数据的协程指针。
*/
// 当使用 ucontext 机制时，加载协程的栈数据
static void
_load_stack(nty_coroutine *co) {
    // 将协程栈缓冲区的数据复制回调度器的栈
    memcpy(co->sched->stack + co->sched->stack_size - co->stack_size, co->stack, co->stack_size);
}

/*
_exec(void *lt)
功能：协程的执行函数，协程启动时会调用此函数。
参数：lt 为传入的协程指针。
*/
// 协程的执行函数，协程启动时会调用此函数
static void _exec(void *lt) {
    // 将传入的参数转换为协程指针
    nty_coroutine *co = (nty_coroutine*)lt;
    // 调用协程的实际执行函数，并传入参数
    co->func(co->arg);
    //任务已经执行完了
    // 设置协程的状态为已退出、文件描述符关闭和分离状态
    co->status |= (BIT(NTY_COROUTINE_STATUS_EXITED) | BIT(NTY_COROUTINE_STATUS_FDEOF) | BIT(NTY_COROUTINE_STATUS_DETACH));
    // 让出协程的执行权
    nty_coroutine_yield(co);
}

#else
// 声明一个切换上下文的函数，用于手动切换协程上下文
int _switch(nty_cpu_ctx *new_ctx, nty_cpu_ctx *cur_ctx);
/*
_switch(nty_cpu_ctx *new_ctx, nty_cpu_ctx *cur_ctx)
功能：手动切换协程上下文。
参数：new_ctx 为新的上下文指针，cur_ctx 为当前的上下文指针。
*/
//把当前cpu的寄存器值保存的switch函数第二个参数中，并把第一个参数的值加载到cpu寄存器中
#ifdef __i386__
// 32 位 x86 架构下的上下文切换汇编代码
__asm__ (
"    .text                                  \n"  // 声明代码段，告诉汇编器将后续代码放入代码段
"    .p2align 2,,3                          \n"  // 按 4 字节（2^2）对齐代码，提高内存访问效率
".globl _switch                             \n"  // 声明 _switch 为全局符号，使其可以在其他文件中被引用
"_switch:                                   \n"  // 定义 _switch 函数入口标签
"__switch:                                  \n"  // 定义 __switch 函数入口标签，与 _switch 等价
"movl 8(%esp), %edx      # fs->%edx         \n"  // %esp 是栈指针寄存器，指向当前栈的栈顶。在 x86 架构中，函数调用时，参数是通过栈来传递的。这里从栈指针 %esp 偏移 8 字节处取出第一个参数，这个参数是当前上下文结构体的指针，将其存储到 %edx 寄存器中
"movl %esp, 0(%edx)      # save esp         \n"  // 将当前栈指针 %esp 的值保存到当前上下文结构体偏移 0 字节处
"movl %ebp, 4(%edx)      # save ebp         \n"  // 将当前基址指针 %ebp 的值保存到当前上下文结构体偏移 4 字节处
"movl (%esp), %eax       # save eip         \n"  // 从栈顶取出返回地址（即当前指令指针）到 %eax 寄存器
"movl %eax, 8(%edx)                         \n"  // 将 %eax 中的返回地址保存到当前上下文结构体偏移 8 字节处
"movl %ebx, 12(%edx)     # save ebx,esi,edi \n"  // 将 %ebx 寄存器的值保存到当前上下文结构体偏移 12 字节处
"movl %esi, 16(%edx)                        \n"  // 将 %esi 寄存器的值保存到当前上下文结构体偏移 16 字节处
"movl %edi, 20(%edx)                        \n"  // 将 %edi 寄存器的值保存到当前上下文结构体偏移 20 字节处
"movl 4(%esp), %edx      # ts->%edx         \n"  // 从栈指针 %esp 偏移 4 字节处取出第二个参数（新上下文结构体指针）到 %edx 寄存器
"movl 20(%edx), %edi     # restore ebx,esi,edi      \n"  // 从新上下文结构体偏移 20 字节处恢复 %edi 寄存器的值
"movl 16(%edx), %esi                                \n"  // 从新上下文结构体偏移 16 字节处恢复 %esi 寄存器的值
"movl 12(%edx), %ebx                                \n"  // 从新上下文结构体偏移 12 字节处恢复 %ebx 寄存器的值
"movl 0(%edx), %esp      # restore esp              \n"  // 从新上下文结构体偏移 0 字节处恢复栈指针 %esp
"movl 4(%edx), %ebp      # restore ebp              \n"  // 从新上下文结构体偏移 4 字节处恢复基址指针 %ebp
"movl 8(%edx), %eax      # restore eip              \n"  // 从新上下文结构体偏移 8 字节处恢复返回地址到 %eax 寄存器
"movl %eax, (%esp)                                  \n"  // 将恢复的返回地址压入栈顶
"ret                                                \n"  // 从栈顶弹出返回地址并跳转到该地址执行，完成上下文切换
);

#elif defined(__x86_64__)
// 64 位 x86 架构下的上下文切换汇编代码
__asm__ (
"    .text                                  \n"  // 声明代码段，告诉汇编器将后续代码放入代码段
"       .p2align 4,,15                                   \n"  // 按 16 字节（2^4）对齐代码，提高内存访问效率
".globl _switch                                          \n"  // 声明 _switch 为全局符号，使其可以在其他文件中被引用
".globl __switch                                         \n"  // 声明 __switch 为全局符号，使其可以在其他文件中被引用
"_switch:                                                \n"  // 定义 _switch 函数入口标签
"__switch:                                               \n"  // 定义 __switch 函数入口标签，与 _switch 等价
"       movq %rsp, 0(%rsi)      # save stack_pointer     \n"  // 将当前栈指针 %rsp 的值保存到当前上下文结构体偏移 0 字节处
"       movq %rbp, 8(%rsi)      # save frame_pointer     \n"  // 将当前基址指针 %rbp 的值保存到当前上下文结构体偏移 8 字节处
"       movq (%rsp), %rax       # save insn_pointer      \n"  // 从栈顶取出返回地址（即当前指令指针）到 %rax 寄存器
"       movq %rax, 16(%rsi)                              \n"  // 将 %rax 中的返回地址保存到当前上下文结构体偏移 16 字节处
"       movq %rbx, 24(%rsi)     # save rbx,r12-r15       \n"  // 将 %rbx 寄存器的值保存到当前上下文结构体偏移 24 字节处
"       movq %r12, 32(%rsi)                              \n"  // 将 %r12 寄存器的值保存到当前上下文结构体偏移 32 字节处
"       movq %r13, 40(%rsi)                              \n"  // 将 %r13 寄存器的值保存到当前上下文结构体偏移 40 字节处
"       movq %r14, 48(%rsi)                              \n"  // 将 %r14 寄存器的值保存到当前上下文结构体偏移 48 字节处
"       movq %r15, 56(%rsi)                              \n"  // 将 %r15 寄存器的值保存到当前上下文结构体偏移 56 字节处
"       movq 56(%rdi), %r15                              \n"  // 从新上下文结构体偏移 56 字节处恢复 %r15 寄存器的值
"       movq 48(%rdi), %r14                              \n"  // 从新上下文结构体偏移 48 字节处恢复 %r14 寄存器的值
"       movq 40(%rdi), %r13     # restore rbx,r12-r15    \n"  // 从新上下文结构体偏移 40 字节处恢复 %r13 寄存器的值
"       movq 32(%rdi), %r12                              \n"  // 从新上下文结构体偏移 32 字节处恢复 %r12 寄存器的值
"       movq 24(%rdi), %rbx                              \n"  // 从新上下文结构体偏移 24 字节处恢复 %rbx 寄存器的值
"       movq 8(%rdi), %rbp      # restore frame_pointer  \n"  // 从新上下文结构体偏移 8 字节处恢复基址指针 %rbp
"       movq 0(%rdi), %rsp      # restore stack_pointer  \n"  // 从新上下文结构体偏移 0 字节处恢复栈指针 %rsp
"       movq 16(%rdi), %rax     # restore insn_pointer   \n"  // 从新上下文结构体偏移 16 字节处恢复返回地址到 %rax 寄存器
"       movq %rax, (%rsp)                                \n"  // 将恢复的返回地址压入栈顶
"       ret                                              \n"  // 从栈顶弹出返回地址并跳转到该地址执行，完成上下文切换
);
#endif

// 协程的执行函数，协程启动时会调用此函数
static void _exec(void *lt) {
    #if defined(__lvm__) && defined(__x86_64__)
        // 在特定环境下，从栈中获取参数
        __asm__("movq 16(%%rbp), %[lt]" : [lt] "=r" (lt));
    #endif
        // 将传入的参数转换为协程指针
        nty_coroutine *co = (nty_coroutine*)lt;
        // 调用协程的实际执行函数，并传入参数
        co->func(co->arg);
        // 设置协程的状态为已退出、文件描述符关闭和分离状态
        co->status |= (BIT(NTY_COROUTINE_STATUS_EXITED) | BIT(NTY_COROUTINE_STATUS_FDEOF) | BIT(NTY_COROUTINE_STATUS_DETACH));
    #if 1
        // 让出协程的执行权
        nty_coroutine_yield(co);
    #else
        co->ops = 0;
        // 手动切换上下文
        _switch(&co->sched->ctx, &co->ctx);
    #endif
}

/*
nty_coroutine_madvise(nty_coroutine *co)
功能：对协程的栈进行内存管理建议，用于释放未使用的栈内存。
参数：co 为要进行内存管理建议的协程指针。
*/
// 对协程的栈进行内存管理建议，用于释放未使用的栈内存
static inline void nty_coroutine_madvise(nty_coroutine *co) {
    // 计算当前栈的使用大小
    size_t current_stack = (co->stack + co->stack_size) - co->ctx.esp;
    // 确保当前栈使用大小不超过栈的总大小
    assert(current_stack <= co->stack_size);
    // 如果当前栈使用大小小于上次记录的栈大小，且上次记录的栈大小大于页面大小
    if (current_stack < co->last_stack_size &&
        co->last_stack_size > co->sched->page_size) {
        // 计算需要释放的栈内存大小
        size_t tmp = current_stack + (-current_stack & (co->sched->page_size - 1));
        // 建议操作系统释放未使用的栈内存
        /*
        madvise 函数允许程序向内核提供关于如何处理指定内存区域的建议。内核可以根据这些建议来优化内存管理，例如预读数据、释放不再使用的内存等。
        参数：
        addr：指向要提供建议的内存区域的起始地址。
        length：指定内存区域的长度（以字节为单位）。
        advice：提供的建议类型，常见的建议类型包括：
        MADV_NORMAL：使用默认的内存管理策略。
        MADV_RANDOM：表示内存访问是随机的，内核可以减少预读操作。
        MADV_SEQUENTIAL：表示内存访问是顺序的，内核可以增加预读操作。
        MADV_DONTNEED：表示程序不再需要指定的内存区域，内核可以释放该区域的物理内存。
        返回值：
        成功时返回 0。
        失败时返回 -1，并设置 errno 以指示错误类型。
        */
        assert(madvise(co->stack, co->stack_size-tmp, MADV_DONTNEED) == 0);
    }
    // 更新上次记录的栈大小
    co->last_stack_size = current_stack;
}

#endif

// 声明一个外部函数，用于创建调度器
extern int nty_schedule_create(int stack_size);

/*
nty_coroutine_free(nty_coroutine *co)
功能：释放协程占用的内存。
参数：co 为要释放内存的协程指针。
*/
// 释放协程占用的内存
void nty_coroutine_free(nty_coroutine *co) {
    // 如果协程指针为空，直接返回
    if (co == NULL) return ;
    // 减少调度器中已创建的协程数量
    co->sched->spawned_coroutines --;
#if 1
    // 如果协程的栈内存存在
    if (co->stack) {
        // 释放协程的栈内存
        free(co->stack);
        co->stack = NULL;
    }
#endif
    // 释放协程结构体的内存
    if (co) {
        free(co);
    }
}

/*
nty_coroutine_init(nty_coroutine *co)
功能：初始化协程。
参数：co 为要初始化的协程指针。
*/
// 初始化协程
// 初始化协程
static void nty_coroutine_init(nty_coroutine *co) {
#ifdef _USE_UCONTEXT
    // 获取当前上下文
    getcontext(&co->ctx);
    // 设置上下文的栈指针
    co->ctx.uc_stack.ss_sp = co->sched->stack;
    // 设置上下文的栈大小
    co->ctx.uc_stack.ss_size = co->sched->stack_size;
    // 设置上下文的链接指针
    co->ctx.uc_link = &co->sched->ctx;
    // 创建一个新的上下文，指定执行函数和参数
    //uc_link 是一个指向另一个 ucontext_t 结构体的指针，它的作用是当当前上下文执行完毕（例如，从 makecontext 设定的函数返回）时，系统会自动恢复到 uc_link 所指向的上下文继续执行
    makecontext(&co->ctx, (void (*)(void)) _exec, 1, (void*)co);
#else
    // 计算栈顶指针
    void **stack = (void **)(co->stack + co->stack_size);
    // 设置栈上的一些值
    stack[-3] = NULL;
    stack[-2] = (void *)co;
    // 设置协程上下文的栈指针
    co->ctx.esp = (void*)stack - (4 * sizeof(void*));
    // 设置协程上下文的帧指针
    co->ctx.ebp = (void*)stack - (3 * sizeof(void*));
    // 设置协程上下文的指令指针
    co->ctx.eip = (void*)_exec;
#endif
    // 设置协程的状态为就绪状态
    co->status = BIT(NTY_COROUTINE_STATUS_READY);
}

/*
功能：暂停当前协程的执行，将控制权交还给调度器。
参数：
co：指向要暂停的协程的指针。
*/
void nty_coroutine_yield(nty_coroutine *co) {
    // 将协程的操作计数器置为 0
    co->ops = 0;
#ifdef _USE_UCONTEXT
    // 如果协程的状态不是已退出
    if ((co->status & BIT(NTY_COROUTINE_STATUS_EXITED)) == 0) {
        // 保存协程的栈信息
        _save_stack(co);
    }
    // 切换到调度器的上下文，暂停当前协程的执行
    /*
    &co->ctx：表示协程（coroutine）的上下文。co 是一个指向 nty_coroutine 结构体的指针，ctx 是该结构体中的一个 ucontext_t 类型的成员，用于保存协程的上下文信息。当调用 swapcontext 时，当前协程的上下文会被保存到 co->ctx 中。
    &co->sched->ctx：表示调度器（scheduler）的上下文。co->sched 是协程所属的调度器，ctx 是调度器的 ucontext_t 类型的成员，用于保存调度器的上下文信息。调用 swapcontext 后，程序会暂停当前协程的执行，切换到调度器的上下文中继续执行。
    */
    swapcontext(&co->ctx, &co->sched->ctx);
#else
    // 直接切换到调度器的上下文，暂停当前协程的执行
    _switch(&co->sched->ctx, &co->ctx);
#endif
}

/*
功能：恢复指定协程的执行。
参数：
co：指向要恢复执行的协程的指针。
返回值：
0：协程正常恢复执行。
-1：协程已退出。
*/
int nty_coroutine_resume(nty_coroutine *co) {
    // 如果协程是新创建的状态
    if (co->status & BIT(NTY_COROUTINE_STATUS_NEW)) {
        // 初始化协程
        nty_coroutine_init(co);
    } 
#ifdef _USE_UCONTEXT	
    // 如果协程不是新创建的状态
    else {
        // 加载协程的栈信息
        _load_stack(co);
    }
#endif
    // 获取当前的调度器
    nty_schedule *sched = nty_coroutine_get_sched();
    // 将当前执行的协程设置为传入的协程
    sched->curr_thread = co;
#ifdef _USE_UCONTEXT
    // 从调度器的上下文切换到协程的上下文，恢复协程的执行
    swapcontext(&sched->ctx, &co->ctx);
#else
    // 从调度器的上下文切换到协程的上下文，恢复协程的执行
    _switch(&co->ctx, &co->sched->ctx);
    // 对协程的栈进行内存建议，释放不再使用的内存
    nty_coroutine_madvise(co);
#endif
    // 将当前执行的协程置为 NULL
    //当上面调用swap的时候切到具体函数，当该函数yield则切回swap继续执行，此时就是调度器运行因此为NULL
    sched->curr_thread = NULL;

#if 1
    // 如果协程的状态是已退出
    if (co->status & BIT(NTY_COROUTINE_STATUS_EXITED)) {
        // 如果协程的状态是已分离
        if (co->status & BIT(NTY_COROUTINE_STATUS_DETACH)) {
            // 释放协程的资源
            nty_coroutine_free(co);
        }
        // 返回 -1 表示协程已退出
        return -1;
    } 
#endif
    // 返回 0 表示协程正常恢复执行
    return 0;
}

/*
功能：降低协程的调度优先级，当协程的操作计数器达到一定值时，将协程插入到就绪队列尾部并暂停执行。
参数：
co：指向要调整优先级的协程的指针。
*/
void nty_coroutine_renice(nty_coroutine *co) {
    // 协程的操作计数器加 1
    co->ops ++;
#if 1
    // 如果操作计数器小于 5
    if (co->ops < 5) {
        // 直接返回，不做其他操作
        return ;
    }
#endif
    // 将协程插入到调度器的就绪队列尾部
    TAILQ_INSERT_TAIL(&nty_coroutine_get_sched()->ready, co, ready_next);
    // 暂停当前协程的执行
    nty_coroutine_yield(co);
}

/*
功能：让当前协程睡眠指定的时间。
参数：
msecs：要睡眠的时间（毫秒）。
*/
void nty_coroutine_sleep(uint64_t msecs) {
    // 获取当前正在执行的协程
    nty_coroutine *co = nty_coroutine_get_sched()->curr_thread;

    // 如果睡眠时间为 0
    if (msecs == 0) {
        // 将协程插入到调度器的就绪队列尾部
        TAILQ_INSERT_TAIL(&co->sched->ready, co, ready_next);
        // 暂停当前协程的执行
        nty_coroutine_yield(co);
    } else {
        // 将协程设置为睡眠状态，并设置睡眠时间
        nty_schedule_sched_sleepdown(co, msecs);
    }
}

/*
功能：将当前协程标记为已分离状态，当协程退出时，自动释放其资源。
参数：无
*/
void nty_coroutine_detach(void) {
    // 获取当前正在执行的协程
    nty_coroutine *co = nty_coroutine_get_sched()->curr_thread;
    // 将协程的状态设置为已分离
    co->status |= BIT(NTY_COROUTINE_STATUS_DETACH);
}

/*
功能：线程特定数据的析构函数，用于释放线程特定数据的内存。
参数：
data：指向要释放的内存的指针。
*/
static void nty_coroutine_sched_key_destructor(void *data) {
    // 释放传入的数据指针所指向的内存
    free(data);
}

/*
功能：在程序启动时创建一个线程特定数据的键，并将其初始值设置为 NULL。
参数：无
*/
static void __attribute__((constructor(1000))) nty_coroutine_sched_key_creator(void) {
    // 创建一个线程特定数据的键，指定析构函数为 nty_coroutine_sched_key_destructor
    assert(pthread_key_create(&global_sched_key, nty_coroutine_sched_key_destructor) == 0);
    // 将线程特定数据的键的值设置为 NULL
    assert(pthread_setspecific(global_sched_key, NULL) == 0);
    
    return ;
}

/*
功能：创建一个新的协程。
参数：
new_co：指向指针的指针，用于返回新创建的协程的指针。
func：协程要执行的函数。
arg：传递给协程执行函数的参数。
返回值：
0：创建成功。
-1：创建调度器失败。
-2：分配协程结构体内存失败。
-3：分配协程栈内存失败。
*/
int nty_coroutine_create(nty_coroutine **new_co, proc_coroutine func, void *arg) {
    // 确保线程特定数据的键只创建一次
    assert(pthread_once(&sched_key_once, nty_coroutine_sched_key_creator) == 0);
    // 获取当前的调度器
    nty_schedule *sched = nty_coroutine_get_sched();

    // 如果调度器不存在
    if (sched == NULL) {
        // 创建一个新的调度器
        nty_schedule_create(0);
        
        // 再次获取调度器
        sched = nty_coroutine_get_sched();
        // 如果调度器仍然不存在
        if (sched == NULL) {
            // 输出错误信息
            printf("Failed to create scheduler\n");
            // 返回 -1 表示创建失败
            return -1;
        }
    }

    // 分配一个新的协程结构体的内存
    nty_coroutine *co = calloc(1, sizeof(nty_coroutine));
    // 如果内存分配失败
    if (co == NULL) {
        // 输出错误信息
        printf("Failed to allocate memory for new coroutine\n");
        // 返回 -2 表示创建失败
        return -2;
    }

#ifdef _USE_UCONTEXT
    // 如果使用 ucontext 机制，将协程的栈指针置为 NULL，栈大小置为 0
    /*
    使用 ucontext 时：协程共享调度器分配的同一块栈空间，在协程切换时，需要保存和恢复栈数据。
    ，协程切换时，直接切换到各自的栈空间执行。
    */
    co->stack = NULL;
    co->stack_size = 0;
#else
    // 分配协程的栈内存，保证栈内存按页对齐
    int ret = posix_memalign(&co->stack, getpagesize(), sched->stack_size);
    // 如果内存分配失败
    if (ret) {
        // 输出错误信息
        printf("Failed to allocate stack for new coroutine\n");
        // 释放协程结构体的内存
        free(co);
        // 返回 -3 表示创建失败
        return -3;
    }
    // 设置协程的栈大小
    co->stack_size = sched->stack_size;
#endif
    // 设置协程所属的调度器
    co->sched = sched;
    // 设置协程的状态为新创建
    co->status = BIT(NTY_COROUTINE_STATUS_NEW); 
    // 为协程分配一个唯一的 ID
    co->id = sched->spawned_coroutines ++;
    // 设置协程要执行的函数
    co->func = func;
#if CANCEL_FD_WAIT_UINT64
    // 如果定义了 CANCEL_FD_WAIT_UINT64，将协程的文件描述符置为 -1，事件置为 0
    co->fd = -1;
    co->events = 0;
#else
    // 否则，将协程的文件描述符等待值置为 -1
    co->fd_wait = -1;
#endif
    // 设置协程要执行的函数的参数
    co->arg = arg;
    // 记录协程的创建时间
    co->birth = nty_coroutine_usec_now();
    // 将新创建的协程指针赋值给传入的指针
    *new_co = co;

    // 将协程插入到调度器的就绪队列尾部
    TAILQ_INSERT_TAIL(&co->sched->ready, co, ready_next);

    // 返回 0 表示创建成功
    return 0;
}