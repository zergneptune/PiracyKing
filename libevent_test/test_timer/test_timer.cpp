#include <event2/event.h>
#include <iostream>
#include <thread>
#include <chrono>
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;
static timeval t1 = { 1, 0 };
void timer1(int sock, short which, void* arg)
{
    cout << "[timer1]" << flush;
    event* ev = (event*)arg;
    //no pending
    if (!evtimer_pending(ev, &t1))
    {
        evtimer_del(ev);
        evtimer_add(ev, &t1);
    }
}

void timer2(int sock, short which, void* arg)
{
    cout << "[timer2]" << flush;
    this_thread::sleep_for(std::chrono::milliseconds(3000));
}

void timer3(int sock, short which, void* arg)
{
    cout << "[timer3]" << flush;
}

int main()
{
#ifdef _WIN32 
    //初始化socket库
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
#else
    //忽略管道信号，发送数据给已经关闭的socket
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return 1;
#endif
    event_base* base = event_base_new();

    //定时器
    cout << "test timer" << endl;

    //event_new
    /*
    #define evtimer_new(b, cb, arg)     event_new((b), -1, 0, (cb), (arg))
    #define evtimer_add(ev, tv)     event_add((ev), (tv))
    #define evtimer_del(ev)         event_del(ev)
    */
    //定时器 非持久事件，只进入一次
    event* ev1 = evtimer_new(base, timer1, event_self_cbarg());
    if (!ev1)
    {
        cerr << "evtimer_new timer1 failed!" << endl;
        return -1;
    }

    //添加事件
    evtimer_add(ev1, &t1);

    static timeval t2;
    t2.tv_sec = 1;
    t2.tv_usec = 200000;
    event* ev2 = event_new(base, -1, EV_PERSIST, timer2, 0);
    event_add(ev2, &t2);//插入性能 O(logn)

    //超时优化性能优化，默认event 用二叉堆存储，插入删除O(logn)
    //优化到双向队列，插入删除O(1)
    static timeval tv_in = { 3, 0 };
    const timeval* t3;
    t3 = event_base_init_common_timeout(base, &tv_in);
    event* ev3 = event_new(base, -1, EV_PERSIST, timer3, 0);
    event_add(ev3, t3);//插入性能 O(1)

    //进入事件主循环
    event_base_dispatch(base);
    event_free(ev1);
    event_free(ev2);
    event_base_free(base);
    
    return 0;
}
