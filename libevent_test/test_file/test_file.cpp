#include <event2/event.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;
static void read_file(evutil_socket_t fd, short event, void* arg)
{
    char buf[1024] = { 0 };
    int len = read(fd, buf, sizeof(buf) - 1);
    if(len > 0)
    {
        cout << buf << endl;
    }
    else
    {
        //cout << "." << endl;
    }
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
    event_config* conf = event_config_new();
    //设置支持文件描述符
    event_config_require_features(conf, EV_FEATURE_FDS);
    event_base* base = event_base_new_with_config(conf);
    event_config_free(conf);
    
    if(!base)
    {
        cerr << "event_base_new_with_config failed!" << endl;
        return -1;
    }

    //打开文件，指针移到文件末尾
    /*int sock = open("/var/log/audit/audit.log", O_RDONLY | O_NONBLOCK, 0);
    lseek(sock, 0, SEEK_END);
    if(sock <= 0)
    {
        cerr << "open error!" << endl;
        return -2;
    }*/

    //监听文件数据
    //event* fev = event_new(base, sock, EV_READ | EV_PERSIST, read_file, 0);

    //监听标准输入
    event* fev = event_new(base, 0, EV_READ | EV_PERSIST, read_file, 0);

    //添加事件
    event_add(fev, NULL);

     //进入事件主循环
    event_base_dispatch(base);
    event_base_free(base);
    
    return 0;
}
