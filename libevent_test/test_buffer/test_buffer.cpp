#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <string.h>
#include <iostream>
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;

//错误，超时 （连接断开会进入）（可以处理心跳包）
static void event_cb(bufferevent* be, short events, void* arg)
{
    cout << "[E]" << flush;
    //读取超时事件发生后，数据读取停止
    if(events & BEV_EVENT_TIMEOUT && events & BEV_EVENT_READING)
    {
        cout << "BEV_EVENT_TIMEOUT BEV_EVENT_READING)" << endl;
        //bufferevent_enable(be, EV_READ);
        bufferevent_free(be);
    }
    else if(events & BEV_EVENT_ERROR)
    {
        bufferevent_free(be);
    }
    else
    {
        cout << "OTHERS" << endl;
    }
}

static void write_cb(bufferevent* be, void* arg)
{
    cout << "[W]" << flush;
}

static void read_cb(bufferevent* be, void* arg)
{
    cout << "[R]" << flush;
    char data[1024] = {0};
    //读取输入缓冲
    int len = bufferevent_read(be, data, sizeof(data) - 1);
    cout << "[" << data << "]" << endl;
    if(len <= 0) return;
    if(strstr(data, "quit") != NULL)
    {
        cout << "quit";
        //退出并关闭socket
        bufferevent_free(be);
    }

    //发送数据, 写入到输出缓冲
    bufferevent_write(be, "OK", 3);//会触发写回调事件
}

static void listen_cb(evconnlistener* ev, evutil_socket_t s, sockaddr* sin, int slen, void* arg)
{
    cout << "listen_cb" << endl;
    event_base* base = (event_base*)arg;
    //创建bufferevent上下文
    //清理bufferevent时关闭socket
    bufferevent* bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);

    //添加监控事件
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    //设置水位
    //读取水位
    bufferevent_setwatermark(bev,
                             EV_READ,
                             5, //低水位 0是无限制（默认是0）
                             10//高水位 0是无限制
                            );

    //写入水位
    bufferevent_setwatermark(bev,
                             EV_WRITE,
                             5, //低水位 0是无限制（默认是0）缓冲数据低于5 写入回掉被调用
                             0//高水位无效
                            );

    //超时时间设置
    timeval t1 = {3, 0};
    bufferevent_set_timeouts(bev, &t1, 0);

    //添加回掉函数nn
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, base);
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
    //创建网络服务器
    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(5001);

    evconnlistener* ev = evconnlistener_new_bind(base,
                            listen_cb,
                            base,
                            LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
                            10, //listen back
                            (sockaddr*)&sin,
                            sizeof(sin));


    //添加事件
    //evconnlistener_add(ev, 0);

    //进入事件主循环
    event_base_dispatch(base);

    //清理
    event_base_free(base);
    evconnlistener_free(ev);
    
    return 0;
}
