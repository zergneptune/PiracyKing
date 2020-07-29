#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>
#include <string.h>
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;

//当缓冲有数据可读时，先进入filter_in事件，再进入read_cb事件
bufferevent_filter_result filter_in(evbuffer* source, evbuffer* dest, ev_ssize_t limit,
                                    bufferevent_flush_mode mode, void* arg)
{
    cout << "filter_in" << endl;
    //读取并清理源数据
    char data[1024] = {0};
    int len = evbuffer_remove(source, data, sizeof(data) - 1);

    //所有字母转成大写
    for(int i = 0; i < len; ++i)
    {
        data[i] = toupper(data[i]);
    }

    evbuffer_add(dest, data, len);
    return BEV_OK;
}

//调用bufferevent_write 先进入filter_out事件，再进入write_cb事件
bufferevent_filter_result filter_out(evbuffer* source, evbuffer* dest, ev_ssize_t limit,
                                    bufferevent_flush_mode mode, void* arg)
{
    cout << "filter_out" << endl;
    //读取并清理源数据
    char data[1024] = {0};
    int len = evbuffer_remove(source, data, sizeof(data) - 1);

    string str = "";
    str += "==================\n";
    str += data;
    str += "==================\n";

    evbuffer_add(dest, str.c_str(), str.size());
    return BEV_OK;
}

static void write_cb(bufferevent* be, void* arg)
{
    cout << "write_cb" << endl;
}

static void read_cb(bufferevent* be, void* arg)
{
    cout << "read_cb" << endl;
    char data[1024] = {0};
    int len = bufferevent_read(be, data, sizeof(data) - 1);
    cout << data << endl;

    //回复客户消息，经过filter_out
    bufferevent_write(be, data, len);
}

static void event_cb(bufferevent* bev, short events, void* arg)
{
    cout << "event_cb" << endl;
}

static void listen_cb(evconnlistener* ev, evutil_socket_t s, sockaddr* sin, int slen, void* arg)
{
    cout << "listen_cb" << endl;
    //创建bufferevent 
    event_base* base = (event_base*)arg;
    bufferevent* bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);
    //绑定bufferevent filter
    bufferevent* bev_filter = bufferevent_filter_new(bev,
                                                     filter_in,
                                                     filter_out,
                                                     BEV_OPT_CLOSE_ON_FREE,
                                                     0,//清理的回调函数
                                                     0//传递给回调的参数
                                                    );

    //设置bufferevent filter的回调
    bufferevent_setcb(bev_filter, read_cb, write_cb, event_cb, 0);

    bufferevent_enable(bev_filter, EV_READ | EV_WRITE);

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
