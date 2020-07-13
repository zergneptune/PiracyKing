#pragma once
#include <event2/event.h>
#include <event2/util.h>
#include <list>
#include <mutex>
class XTask;

class XThread
{
public:
    XThread() {}
    ~XThread() {}

    //启动线程
    void Start();

    //线程入口函数
    void Main();

    //安装线程，初始化event_base和管道监听事件用于激活
    bool Setup();

    //收到主线程发出的激活消息（线程池的分发）
    void Notify(evutil_socket_t fd, short which);

    //线程激活
    void Activate();

    //添加任务
    void AddTask(XTask*);

public:
    //线程编号
    int m_nId = 0;

private:
    int m_notity_send_fd = 0;

    struct event_base* m_base = 0;

    //任务列表
    std::list<XTask*>   m_lst_task;

    std::mutex m_mtx_task;
};

