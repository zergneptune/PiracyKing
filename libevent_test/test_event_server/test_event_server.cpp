#include <event2/event.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <error.h>
#include <string.h>
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;
//超时、正常断开连接都会进入回掉
static void client_cb(evutil_socket_t s, short w, void* arg)
{
    /*
    event* ev = (event*)arg;
    //判断超时
    if(w & EV_TIMEOUT)
    {
        cout << "timeout" << flush;
        event_free(ev);
        evutil_closesocket(s);
        return;
    }

    char buf[1024] = {0};
    int len = recv(s, buf, sizeof(buf) - 1, 0);
    if(len > 0)
    {
        cout << buf << endl;
        send(s, "ok", 2, 0);
    }
    else
    {
        cout << "event_free" << flush;
        event_free(ev);
        evutil_closesocket(s);
    }
    */
    cout << "*" << flush;
}

static void listen_cb(evutil_socket_t s, short w, void* arg)
{
    cout << "listen_cb" << endl;
    sockaddr_in sin;
    socklen_t size = sizeof(sin);
    //读取连接信息
    evutil_socket_t client = accept(s, (sockaddr*)&sin, &size);
    char ip[16] = { 0 };
    evutil_inet_ntop(AF_INET, &sin.sin_addr, ip, sizeof(ip) - 1);
    cout << "client ip is " << ip << endl;

    //客户端数据读取
    event_base* base = (event_base*)arg;
    //默认水平触发
    event* ev = event_new(base, client, EV_READ | EV_PERSIST, client_cb, event_self_cbarg());
    //边缘触发
    //event* ev = event_new(base, client, EV_READ | EV_PERSIST | EV_ET, client_cb, event_self_cbarg());
    timeval t = { 10, 0 };
    event_add(ev, &t);
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

    //event服务器
    cout << "test event server" << endl;

    evutil_socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock <= 0)
    {
        cerr << "socket error: " << strerror(errno) << endl;
        return -1;
    }
    //设置地址复用和非阻塞
    evutil_make_socket_nonblocking(sock);
    evutil_make_listen_socket_reuseable(sock);

    sockaddr_in sin;
    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(5001);
    int re = ::bind(sock, (sockaddr*)&sin, sizeof(sin));
    if(re != 0)
    {
        cerr << "bind error: " << strerror(errno) << endl;
        return -1;
    }
    //监听
    listen(sock, 10);

    //监听连接事件,默认水平触发
    event* ev = event_new(base, sock, EV_READ | EV_PERSIST, listen_cb, base);
    event_add(ev, 0);

    //进入事件主循环
    event_base_dispatch(base);
    event_base_free(base);
    evutil_closesocket(sock);
    
    return 0;
}
