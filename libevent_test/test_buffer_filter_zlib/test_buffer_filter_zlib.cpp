#include <event2/event.h>
#include <iostream>
#include <string.h>
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;

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

    void Server(event_base* base);
    Server(base);

    void Client(event_base* base);
    Client(base);


    //进入事件主循环
    if(base)
        event_base_dispatch(base);

    //清理
    if(base)
        event_base_free(base);
    
    return 0;
}
