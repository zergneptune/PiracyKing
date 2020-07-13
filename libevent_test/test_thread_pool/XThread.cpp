#include "XThread.h"
#include <thread>
#include <iostream>
#include <event2/event.h>
#include <event2/util.h>
#include "XTask.h"
#ifdef _WIN32
#else
#include <unistd.h>
#endif

//激活线程任务的回调函数
static void NotifyCB(evutil_socket_t fd, short which, void* arg)
{
    XThread* t = (XThread*)arg;
    t->Notify(fd, which);
}

void XThread::Notify(evutil_socket_t fd, short which)
{
    //水平触发 只要没有接受完全，会再次进来
    char buf[2] = {0};
#ifdef _WIN32
    int re = recv(fd, buf, 1, 0);
#else
    //linux中是管道，不能用recv
    int re = read(fd, buf, 1);
#endif
    if(re <=0 )
    {
        return;
    }
    std::cout << m_nId << " thread " << buf << std::endl;
    //获取任务，并初始化任务
    m_mtx_task.lock();
    if(m_lst_task.empty())
    {
        m_mtx_task.unlock();
        return;
    }

    XTask* task = NULL;
    task = m_lst_task.front();
    m_lst_task.pop_front();
    m_mtx_task.unlock();
    task->Init();

}

void XThread::Activate()
{
#ifdef _WIN32
    int re = send(m_notity_send_fd, "c", 1, 0);
#else
    int re = write(m_notity_send_fd, "c", 1);
#endif
    if(re <= 0)
    {
        std::cerr << "XThread::Activate() failed!" << std::endl;
    }
}


void XThread::Start()
{
    Setup();
    std::thread th(&XThread::Main, this);

    th.detach();
}

void XThread::Main()
{
    std::cout << m_nId << " thread begin" << std::endl;
    event_base_dispatch(m_base);
    event_base_free(m_base);
    std::cout << m_nId << " thread end" << std::endl;
}

bool XThread::Setup()
{
    //windows用socketpair linux用管道
#ifdef _WIN32
    //创建socketpair
    evutil_socket_t fds[2]; //fds[0]:read fds[1]:write
    if(evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds) < 0)
    {
        std::cout << "evutil_socketpair failed!" << std::endl;
        return false;
    }

    //设置非阻塞
    evutil_make_socket_nonblocking(fds[0]);
    evutil_make_socket_nonblocking(fds[1]);
#else
    //创建管道
    int fds[2];
    if(pipe(fds))
    {
        std::cerr << "pipe failed!" << std::endl;
        return false;
    }

#endif

    //读取绑定到event事件，写入要保存
    m_notity_send_fd = fds[1];

    //创建libevent上下文(无锁)
    event_config* ev_config = event_config_new();
    event_config_set_flag(ev_config, EVENT_BASE_FLAG_NOLOCK);
    this->m_base = event_base_new_with_config(ev_config);
    event_config_free(ev_config);
    if(!m_base)
    {
        std::cerr << "event_base_new_with_config failed in thread!" << std::endl;
    }

    //创建管道监听事件，用于激活线程
    
    event* ev = event_new(m_base, fds[0], EV_READ | EV_PERSIST, NotifyCB, this);
    event_add(ev, 0);


    return true;
}

void XThread::AddTask(XTask* task)
{
    if(!task) return;
    task->m_base = m_base;
    m_mtx_task.lock();
    m_lst_task.push_back(task);
    m_mtx_task.unlock();
}

