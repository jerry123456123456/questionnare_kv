#ifndef __EVENT_DISPATCH_H__
#define __EVENT_DISPATCH_H__

#include "ostype.h"
#include "util.h"

#include "lock.h"

enum {
    SOCKET_READ = 0x1,
    SOCKET_WRITE = 0x2,
    SOCKET_EXCEP = 0x4,
    SOCKET_ALL = 0x7
};

//事件调度器，用于管理和调度网络事件、定时器事件以及循环事件（循环回调）
class CEventDispatch{   //这个类才是单例模式
public:
    virtual ~CEventDispatch();

    void AddEvent(SOCKET fd,uint8_t socket_event);
    void RemoveEvent(SOCKET fd,uint8_t socket_event);

    void AddTimer(callback_t callback,void *user_data,uint64_t interval);
    void RemoveTimer(callback_t callback,void *user_data);

    void AddLoop(callback_t callback,void *user_data);

    void StartDispatch(uint32_t wait_timeout=100);
    void StopDispatch();

    bool IsRunning(){return running_;}

    static CEventDispatch *Instance();

protected:
    CEventDispatch();

private:
    void _CheckTimer();
    void _CheckLoop();

     typedef struct{ //这个 TimerItem 结构体是用于描述定时器任务的
        callback_t callback;
        void *user_data;
        uint64_t interval;  // 这是定时器的间隔时间，以毫秒为单位。表示每隔多长时间这个定时器会被触发
        uint64_t next_tick; //这是下次触发的时间戳，通常用来记录定时器任务的下一次触发时间点
    }TimerItem;

private:
#ifdef _WIN32
    fd_set m_read_set;
    fd_set m_write_set;
    fd_set m_excep_set;
#elif __APPLE__
    int m_kqfd;
#else 
    int epfd_;
#endif
    CLock lock_;
    list<TimerItem *> timer_list_; // 定时器
    list<TimerItem *> loop_list_;  // 自定义loop

    static CEventDispatch *event_dispatch_;  //用于存储 CEventDispatch 类的唯一实例，这是单例模式的一部分

    bool running_;
};

#endif