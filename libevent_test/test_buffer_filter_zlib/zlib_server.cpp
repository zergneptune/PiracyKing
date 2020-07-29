#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>
#include <string.h>
#include <zlib.h>
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;
struct Status
{
    bool start = false;
    FILE* fp = NULL;
    z_stream* p;
    int recvNum = 0;
    int writeNum = 0;
    string filename;
    ~Status()
    {
        if(fp)
            fclose(fp);
        fp = 0;
        if(p)
            inflateEnd(p);
        p = 0;
    }
};

bufferevent_filter_result filter_in(evbuffer* source, evbuffer* dest, ev_ssize_t limit, bufferevent_flush_mode mode, void* arg)
{
    Status* status = (Status*)arg;
    cout << "server filter_in" << endl;
    //接受客户端发送的文件名
    if(!status->start)
    {
        char data[1024] = {0};
        int len = evbuffer_remove(source, data, sizeof(data) - 1);
        cout << "server recv " << data << endl;
        evbuffer_add(dest, data, len);
        return BEV_OK;
    }

    //解压
    evbuffer_iovec v_in[1];
    int n = evbuffer_peek(source, -1, NULL, v_in, 1);
    if(n <= 0)
        return BEV_NEED_MORE;

    //解压上下文
    z_stream* p = status->p;

    //zlib 输入数据大小
    p->avail_in = v_in[0].iov_len;
    //zlib 输入数据地址
    p->next_in = (Byte*)v_in[0].iov_base;

    //申请输出空间大小
    evbuffer_iovec v_out[1];
    evbuffer_reserve_space(dest, 4096, v_out, 1);

    //zlib 输出空间大小
    p->avail_out = v_out[0].iov_len;
    //zlib 输出空间地址
    p->next_out = (Byte*)v_out[0].iov_base;

    //解压数据
    int re = inflate(p, Z_SYNC_FLUSH);
    if(re != Z_OK)
    {
        cout << "server nflate failed!" << endl;
    }

    //解压缩用了多少数据，从source中移除
    //p->avail_in 未处理的数据大小
    int nread = v_in[0].iov_len - p->avail_in;
    
    //解压缩后数据大小，传入dest
    //p->avail_out 剩余空间大小
    int nwrite = v_out[0].iov_len - p->avail_out;

    //移除source evbuufer中数据
    evbuffer_drain(source, nread);

    //传入 dest
    v_out[0].iov_len = nwrite;
    evbuffer_commit_space(dest, v_out, 1);
    cout << "server nread " << nread << " nwrite " << nwrite << endl;
    status->recvNum += nread;
    status->writeNum += nwrite;

    return BEV_OK;
}

static void read_cb(bufferevent* bev, void* arg)
{
    cout << "server read_cb" << endl;
    Status* status = (Status*)arg;
    if(!status->start)
    {
        //接受文件名
        char data[1024] = { 0 };
        bufferevent_read(bev, data, sizeof(data) - 1);
        status->filename = data;
        string out = "out/";
        out += data;
        status->fp = fopen(out.c_str(), "wb");
        if(!status->fp)
        {
            cout << "server oepn " << out << " failed!" << endl;
        }

        //回复OK
        bufferevent_write(bev, "OK", 2);
        status->start = true;
    }

    //写入文件
    do{
        char data[1024] = { 0 };
        int len = bufferevent_read(bev, data, sizeof(data));
        if(len >=0)
        {
            fwrite(data, 1, len, status->fp);
            fflush(status->fp);
        }
    }while(evbuffer_get_length(bufferevent_get_input(bev)) > 0);
}

static void event_cb(bufferevent* bev, short events, void* arg)
{
    cout << "server event_cb" << endl;
    Status* status = (Status*)arg;
    if(events & BEV_EVENT_EOF)
    {
        cout << "server event BEV_EVENT_EOF" << endl;
        delete status;
        bufferevent_free(bev);
        cout << "TOTAL: Server receive " << status->recvNum;
        cout << " write " << status->writeNum << endl;
    }

}

static void listen_cb(evconnlistener* ev, evutil_socket_t s, sockaddr* sin, int slen, void* arg)
{
    cout << "listen_cb" << endl;
    event_base* base = (event_base*)arg;
    //1 创建一个bufferevent 用来通信
    bufferevent* bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);
    Status* status = new Status();
    status->p = new z_stream();
    inflateInit(status->p);

    //2 添加输入过滤器，并设置输入回调
    bufferevent* bev_filter = bufferevent_filter_new(bev,
                                                     filter_in,//输入过滤函数
                                                     0,//输出过滤函数
                                                     BEV_OPT_CLOSE_ON_FREE,
                                                     0,//清理回调
                                                     status//参数
                                                    );

    //3 设置回调 读，事件（用来处理连接断开）
    bufferevent_setcb(bev_filter, read_cb, 0, event_cb, status);

    bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
}


void Server(event_base* base)
{
    cout << "begin server" << endl;
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


}
