#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <string.h>
#include <string>
#include <iostream>
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;
static string recvstr = "";
static int recvCount = 0;
static int sendCount = 0;

//错误，超时 （连接断开会进入）（可以处理心跳包）
static void event_cb(bufferevent* be, short events, void* arg)
{
    cout << "[E]" << flush;
    //读取超时事件发生后，数据读取停止
    if(events & BEV_EVENT_TIMEOUT && events & BEV_EVENT_READING)
    {
        //读取缓冲中遗留数据
        char data[1024] = {0};
        int len = bufferevent_read(be, data, sizeof(data) - 1);
        if(len > 0)
        {
            recvCount += len;
            recvstr += data;
        }

        cout << "BEV_EVENT_TIMEOUT BEV_EVENT_READING" << endl;
        //bufferevent_enable(be, EV_READ);
        bufferevent_free(be);
        cout << recvstr << endl;
        cout << "recvCount = " << recvCount << endl;
        cout << "sendCount = " << sendCount << endl;
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
    //cout << "[" << data << "]" << endl;
    cout << data << flush;
    if(len <= 0) return;
    recvstr += data;
    recvCount += len;
    /*if(strstr(data, "quit") != NULL)
    {
        cout << "quit";
        //退出并关闭socket
        bufferevent_free(be);
    }*/

    //发送数据, 写入到输出缓冲
    bufferevent_write(be, "OK", 3);
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

    //添加回调函数 base是3个回调函数的参数
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, base);
}

//错误，超时 （连接成功或者断开会进入）
static void client_event_cb(bufferevent* be, short events, void* arg)
{
    cout << "[CLIENT E]" << flush;
    //读取超时事件发生后，数据读取停止
    if(events & BEV_EVENT_TIMEOUT && events & BEV_EVENT_READING)
    {
        cout << "BEV_EVENT_TIMEOUT BEV_EVENT_READING" << endl;
        //bufferevent_enable(be, EV_READ);
        bufferevent_free(be);
        return;
    }
    else if(events & BEV_EVENT_ERROR)
    {
        bufferevent_free(be);
        return;
    }
    //服务端的关闭事件
    if(events & BEV_EVENT_EOF)
    {
        cout << "BEV_EVENT_EOF" << endl;
        //触发读事件, 以清理fd
        bufferevent_trigger(be, EV_WRITE, 0);
    }
    if(events & BEV_EVENT_CONNECTED)
    {
        cout << "BEV_EVENT_CONNECTED" << endl;
        //触发write
        bufferevent_trigger(be, EV_WRITE, 0);
    }
}

static void client_write_cb(bufferevent* be, void* arg)
{
    cout << "[CLIENT W]" << flush;
    FILE* fp = (FILE*)arg;
    char data[1024] = {0};
    int len = fread(data, 1, sizeof(data) - 1, fp);
    if(len < 0)
    {
        //读到结尾或者文件出错
        fclose(fp);
        //立刻清理，可能会造成缓冲数据没有发送结束
        //bufferevent_free(be);
        //因为文件发送完毕，关闭写事件，否则一直回调
        bufferevent_disable(be, EV_WRITE);
        return;
    }
    sendCount += len;
    //写入buffer
    bufferevent_write(be, data, len);
}

static void client_read_cb(bufferevent* be, void* arg)
{
    cout << "[CLIENT R]" << flush;
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
                            base,//listen_cb参数
                            LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
                            10, //listen back
                            (sockaddr*)&sin,
                            sizeof(sin));

    {
        //调用客户端
        //-1内部创建socket
        bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
        sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(5001);
        evutil_inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr.s_addr);

        FILE* fp = fopen("test_buffer_client.cpp", "rb");
        //设置回调函数, fp是3个回调函数的参数
        bufferevent_setcb(bev, client_read_cb, client_write_cb, client_event_cb, fp);
        bufferevent_enable(bev, EV_READ | EV_WRITE);
        int re = bufferevent_socket_connect(bev, (sockaddr*)&sin, sizeof(sin));
        if(re == 0)
        {
            cout << "connect" << endl;
        }
    }

    //进入事件主循环
    event_base_dispatch(base);

    //清理
    event_base_free(base);
    evconnlistener_free(ev);
    
    return 0;
}
