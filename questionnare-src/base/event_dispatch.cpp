#include "event_dispatch.h"
#include "base_socket.h"
#include<iostream>

#define MIN_TIMER_DURATION 100 // 100 miliseconds
CEventDispatch *CEventDispatch::event_dispatch_ = NULL;

//初始时唯一实例为空

CEventDispatch::CEventDispatch(){
    running_=false;
#ifdef _WIN32
    FD_ZERO(&m_read_set);
    FD_ZERO(&m_write_set);
    FD_ZERO(&m_excep_set);
#elif __APPLE__
    m_kqfd = kqueue();
    if (m_kqfd == -1) {
        printf("kqueue failed");
    }
#else
    epfd_=epoll_create(1024);
    if(epfd_==-1){
        printf("epoll_create failed!");
    }
#endif
}

CEventDispatch::~CEventDispatch(){
#ifdef _WIN32

#elif __APPLE__
    close(m_kqfd);
#else
    close(epfd_);
#endif
}

//这个函数用于向 `timer_list_` 中添加定时器项 `TimerItem`
void CEventDispatch::AddTimer(callback_t callback,void *user_data,uint64_t interval){
    list<TimerItem *>::iterator it;
    for(it=timer_list_.begin();it!=timer_list_.end();it++){
        TimerItem *pItem=*it;
        if(pItem->callback==callback&&pItem->user_data==user_data){
            //如果找到具有相同回调函数和用户数据的项，则更新其时间间隔和下一个触发时间
            pItem->interval=interval;
            pItem->next_tick=GetTickCount() + interval;
            return;
        }
    }
    //如果循环结束时没有找到匹配项，则创建一个新的 `TimerItem` 对象，设置其属性，并将其添加到 `timer_list_` 列表的末尾
    TimerItem *pItem = new TimerItem;
    pItem->callback=callback;
    pItem->user_data=user_data;
    pItem->interval=interval;
    pItem->next_tick=GetTickCount()+interval;
    std::cout << "Before assignment: " << std::endl;
    std::cout << "pItem->callback: " << pItem->callback << ", expected callback: " << callback << std::endl;
    std::cout << "pItem->user_data: " << pItem->user_data << ", expected user_data: " << user_data << std::endl;
    std::cout << "pItem->interval: " << pItem->interval << ", expected interval: " << interval << std::endl;
    std::cout << "pItem->next_tick: " << pItem->next_tick << ", expected next_tick calculation: " << GetTickCount() + interval << std::endl;
    timer_list_.push_back(pItem);
}

void CEventDispatch::RemoveTimer(callback_t callback,void *user_data){
    list<TimerItem *>::iterator it;
    for(it=timer_list_.begin();it!=timer_list_.end();it++){
        TimerItem *pItem=*it;
        if(pItem->callback==callback&&pItem->user_data==user_data){
            timer_list_.erase(it);
            delete pItem;
            return;
        }
    }
}

//判断定时器是否应该触发
void CEventDispatch::_CheckTimer() {
    uint64_t curr_tick = GetTickCount();
    list<TimerItem *>::iterator it;

    for (it = timer_list_.begin(); it != timer_list_.end();) {
        TimerItem *pItem = *it;
        it++; // iterator maybe deleted in the callback, so we should increment
              // it before callback
        if (curr_tick >= pItem->next_tick) {
            pItem->next_tick += pItem->interval;
            pItem->callback(pItem->user_data, NETLIB_MSG_TIMER, 0, NULL);
        }
    }
}

void CEventDispatch::AddLoop(callback_t callback,void *user_data){
    TimerItem *pItem = new TimerItem;
    pItem->callback=callback;
    pItem->user_data=user_data;
    loop_list_.push_back(pItem);
}

void CEventDispatch::_CheckLoop(){
    for(list<TimerItem*>::iterator it=loop_list_.begin();it!=loop_list_.end();it++){
        TimerItem *pItem =*it;
        pItem->callback(pItem->user_data,NETLIB_MSG_LOOP,0,NULL);        
    }
}

CEventDispatch *CEventDispatch::Instance(){
    if(event_dispatch_==NULL){
        event_dispatch_=new CEventDispatch();
    }
    return event_dispatch_;
}

#ifdef _WIN32

void CEventDispatch::AddEvent(SOCKET fd, uint8_t socket_event) {
    CAutoLock func_lock(&m_lock);

    if ((socket_event & SOCKET_READ) != 0) {
        FD_SET(fd, &m_read_set);
    }

    if ((socket_event & SOCKET_WRITE) != 0) {
        FD_SET(fd, &m_write_set);
    }

    if ((socket_event & SOCKET_EXCEP) != 0) {
        FD_SET(fd, &m_excep_set);
    }
}

void CEventDispatch::RemoveEvent(SOCKET fd, uint8_t socket_event) {
    CAutoLock func_lock(&m_lock);

    if ((socket_event & SOCKET_READ) != 0) {
        FD_CLR(fd, &m_read_set);
    }

    if ((socket_event & SOCKET_WRITE) != 0) {
        FD_CLR(fd, &m_write_set);
    }

    if ((socket_event & SOCKET_EXCEP) != 0) {
        FD_CLR(fd, &m_excep_set);
    }
}

void CEventDispatch::StartDispatch(uint32_t wait_timeout) {
    fd_set read_set, write_set, excep_set;
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = wait_timeout * 1000; // 10 millisecond

    if (running)
        return;
    running = true;

    while (running) {
        _CheckTimer();
        _CheckLoop();

        if (!m_read_set.fd_count && !m_write_set.fd_count &&
            !m_excep_set.fd_count) {
            Sleep(MIN_TIMER_DURATION);
            continue;
        }

        m_lock.lock();
        memcpy(&read_set, &m_read_set, sizeof(fd_set));
        memcpy(&write_set, &m_write_set, sizeof(fd_set));
        memcpy(&excep_set, &m_excep_set, sizeof(fd_set));
        m_lock.unlock();

        int nfds = select(0, &read_set, &write_set, &excep_set, &timeout);

        if (nfds == SOCKET_ERROR) {
            printf("select failed, error code: %d", GetLastError());
            Sleep(MIN_TIMER_DURATION);
            continue; // select again
        }

        if (nfds == 0) {
            continue;
        }

        for (u_int i = 0; i < read_set.fd_count; i++) {
            // printf("select return read count=%d\n", read_set.fd_count);
            SOCKET fd = read_set.fd_array[i];
            CBaseSocket *pSocket = FindBaseSocket((net_handle_t)fd);
            if (pSocket) {
                pSocket->OnRead();
                pSocket->ReleaseRef();
            }
        }

        for (u_int i = 0; i < write_set.fd_count; i++) {
            // printf("select return write count=%d\n", write_set.fd_count);
            SOCKET fd = write_set.fd_array[i];
            CBaseSocket *pSocket = FindBaseSocket((net_handle_t)fd);
            if (pSocket) {
                pSocket->OnWrite();
                pSocket->ReleaseRef();
            }
        }

        for (u_int i = 0; i < excep_set.fd_count; i++) {
            // printf("select return exception count=%d\n", excep_set.fd_count);
            SOCKET fd = excep_set.fd_array[i];
            CBaseSocket *pSocket = FindBaseSocket((net_handle_t)fd);
            if (pSocket) {
                pSocket->OnClose();
                pSocket->ReleaseRef();
            }
        }
    }
}

void CEventDispatch::StopDispatch() { running = false; }

#elif __APPLE__

void CEventDispatch::AddEvent(SOCKET fd, uint8_t socket_event) {
    struct kevent ke;

    if ((socket_event & SOCKET_READ) != 0) {
        EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
        kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
    }

    if ((socket_event & SOCKET_WRITE) != 0) {
        EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
        kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
    }
}

void CEventDispatch::RemoveEvent(SOCKET fd, uint8_t socket_event) {
    struct kevent ke;

    if ((socket_event & SOCKET_READ) != 0) {
        EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
    }

    if ((socket_event & SOCKET_WRITE) != 0) {
        EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
        kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
    }
}

void CEventDispatch::StartDispatch(uint32_t wait_timeout) {
    struct kevent events[1024];
    int nfds = 0;
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = wait_timeout * 1000000;

    if (running)
        return;
    running = true;

    while (running) {
        nfds = kevent(m_kqfd, NULL, 0, events, 1024, &timeout);

        for (int i = 0; i < nfds; i++) {
            int ev_fd = events[i].ident;
            CBaseSocket *pSocket = FindBaseSocket(ev_fd);
            if (!pSocket)
                continue;

            if (events[i].filter == EVFILT_READ) {
                // printf("OnRead, socket=%d\n", ev_fd);
                pSocket->OnRead();
            }

            if (events[i].filter == EVFILT_WRITE) {
                // printf("OnWrite, socket=%d\n", ev_fd);
                pSocket->OnWrite();
            }

            pSocket->ReleaseRef();
        }

        _CheckTimer();
        _CheckLoop();
    }
}

void CEventDispatch::StopDispatch() { running = false; }

#else

void CEventDispatch::AddEvent(SOCKET fd,uint8_t socket_event){
    struct epoll_event ev;
    /*
    `EPOLLIN`：表示对应的文件描述符可以读（包括对端的连接关闭）
    `EPOLLOUT`：表示对应的文件描述符可以写
    `EPOLLET`：表示设置为边缘触发模式（Edge Triggered），在事件发生时只会通知一次，直到下次有事件发生
    `EPOLLPRI`：表示有紧急数据可读
    `EPOLLERR`：表示发生错误，例如连接被对端重置。
    `EPOLLHUP`：表示对应的文件描述符挂起，即对端发生挂起操作
    */
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLPRI | EPOLLERR | EPOLLHUP;
    ev.data.fd=fd;
    if(epoll_ctl(epfd_,EPOLL_CTL_ADD,fd,&ev)!=0){
        printf("epoll_ctl() failed,errno=%d",errno);
    }
}

void CEventDispatch::RemoveEvent(SOCKET fd,uint8_t socket_event){
    if(epoll_ctl(epfd_,EPOLL_CTL_DEL,fd,NULL)!=0){
        printf("epoll_ctl() failed,errno=%d",errno);
    }
}

void CEventDispatch::StartDispatch(uint32_t wait_timeout){
    struct epoll_event events[1024];
    int nfds=0;

    if(running_)return;

    running_=true;

    while(running_){
        nfds=epoll_wait(epfd_,events,1024,wait_timeout);
        for(int i=0;i<nfds;i++){
            int ev_fd=events[i].data.fd;
            CBaseSocket *pSocket = FindBaseSocket(ev_fd);//这里对引用计数加1

            if(!pSocket)continue;

#ifdef EPOLLRDHUP  //对等方关闭了连接的事件

            if (events[i].events & EPOLLRDHUP) {
                // printf("On Peer Close, socket=%d, ev_fd);
                pSocket->OnClose();
            }
#endif
            if(events[i].events & EPOLLIN){
                pSocket->OnRead();
            }

            if (events[i].events & EPOLLOUT) {
                // printf("OnWrite, socket=%d\n", ev_fd);
                pSocket->OnWrite();
            }

            if (events[i].events & (EPOLLPRI | EPOLLERR | EPOLLHUP)) {
                // printf("OnClose, socket=%d\n", ev_fd);
                pSocket->OnClose();
            }

            pSocket->ReleaseRef();
            
        }

        _CheckTimer();
        _CheckLoop();
    } 
}

void CEventDispatch::StopDispatch(){running_=false;}

#endif