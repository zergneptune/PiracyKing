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
#define FILE_PATH "001.txt"
//#define FILE_PATH "001.bmp"
struct ClientStatus
{
    FILE* fp = NULL;
    bool fileend = false;
    bool startSend = false;
    z_stream* z_output = NULL;
    int readNum = 0;
    int sendNum = 0;
    ~ClientStatus()
    {
        if(fp)
            fclose(fp);
        fp = NULL;
        if(z_output)
            deflateEnd(z_output);
        z_output = NULL;
    }
};

//过滤器输出buffer有数据进入
static bufferevent_filter_result filter_out(evbuffer* source, evbuffer* dest, ev_ssize_t limit, bufferevent_flush_mode mode, void* arg)
{
    cout << "client filter_out ";
    ClientStatus* s = (ClientStatus*)arg;
    //压缩文件
    //发送文件名消息001去掉
    if(!s->startSend)
    {
        char data[1024] = {0};
        int len = evbuffer_remove(source, data, sizeof(data));
        cout << len << endl;
        evbuffer_add(dest, data, len);
        return BEV_OK;
    }

    //开始压缩文件
    //取出buffer中数据的引用
    evbuffer_iovec v_in[1];
    int n = evbuffer_peek(source, -1, 0, v_in, 1);
    if(n <= 0)
    {
        //调用write回调，清理空间
        if(s->fileend)
        {
            return BEV_OK;
        }
        //没有数据返回BEV_NEED_MORE 不会进入写入回调
        return BEV_NEED_MORE;
    }
    z_stream* p = s->z_output;
    if(!p)
    {
        return BEV_ERROR;
    }
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

    //zlib压缩
    int re = deflate(p, Z_SYNC_FLUSH);
    if(re != Z_OK)
    {
        cout << "deflate failed!" << endl;
    }

    //压缩用了多少数据，从source中移除
    //p->avail_in 未处理的数据大小
    int nread = v_in[0].iov_len - p->avail_in;
    
    //压缩后数据大小，传入dest
    //p->avail_out 剩余空间大小
    int nwrite = v_out[0].iov_len - p->avail_out;

    //移除source evbuufer中数据
    evbuffer_drain(source, nread);

    //传入 dest
    v_out[0].iov_len = nwrite;
    evbuffer_commit_space(dest, v_out, 1);
    cout << "nread " << nread << " nwrite " << nwrite << endl;

    s->readNum += nread;
    s->sendNum += nwrite;

    //返回BEV_OK 进入写入回调 
    return BEV_OK;
}

static void client_read_cb(bufferevent* bev, void* arg)
{
    ClientStatus* s = (ClientStatus*)arg;
    //002接受服务端发送的OK回复
    cout << "client read_cb" << endl;
    char data[1024] = {0};
    int len = bufferevent_read(bev, data, sizeof(data) - 1);
    if(strcmp(data, "OK") == 0)
    {
        cout << "client recv " << data << endl;
        s->startSend = true;
        //开始发送文件,触发写入回调, 注意：不进入filter_out
        cout << "client trigger write" << endl;
        bufferevent_trigger(bev, EV_WRITE, 0);
    }
    else
    {
        bufferevent_free(bev);
    }
}

//过滤器绑定的原始buffer有数据进入
static void client_write_cb(bufferevent* bev, void* arg)
{
    cout << "client_write_cb" << endl;
    ClientStatus* s = (ClientStatus*)arg;
    if(!s->fp) return;

    //判断什么时候清理资源
    if(s->fileend)
    {
        //判断缓冲是否有数据，如果有刷新
        //获取过滤器绑定的bufferevent
        bufferevent* be = bufferevent_get_underlying(bev);
        //获取输出缓冲及其大小
        evbuffer* evb = bufferevent_get_output(be);
        int len = evbuffer_get_length(evb);
        if(len <=0)
        {
            cout << "TOTAL: Cient read " << s->readNum << " write " << s->sendNum << endl;
            //立刻清理,如果写缓冲区有数据，不会发送
            bufferevent_free(bev);
            delete s;
        }
        cout << "过滤器绑定的buffer(创建过滤器依赖的那个bufferevent)中还有数据 " << len << endl;
        //刷新缓冲, 如果缓冲区有数据，那么进入filter_out，再进入write_cb
        bufferevent_flush(bev, EV_WRITE, BEV_FINISHED);
        return;
    }
    
    //读取文件
    char data[1024] = {0};
    int len = fread(data, 1, sizeof(data), s->fp);
    if(len <= 0)
    {
        s->fileend = true;
        //刷新缓冲
        cout << "do bufferevent_flush" << endl;
        //如果缓冲有数据，进入filter_out，再进入write_cb
        bufferevent_flush(bev, EV_WRITE, BEV_FINISHED);
        return;
    }

    //发送文件，注意：如果有输出过滤器，先进入filter_out，在filter_out中调用evbuffer_add发送
    cout << "do bufferevent_write" << endl;
    bufferevent_write(bev, data, len);
}

static void client_event_cb(bufferevent* be, short events, void* arg)
{
    cout << "client event_cb " << events << endl;
    if(events & BEV_EVENT_CONNECTED)
    {
        cout << "BEV_EVENT_CONNECTED" << endl;
        //001发送文件名
        bufferevent_write(be, FILE_PATH, strlen(FILE_PATH));

        FILE* fp = fopen(FILE_PATH, "rb");
        if(!fp)
        {
            cout << "oepn file " << FILE_PATH << " failed!" << endl;
        }
        ClientStatus* s = new ClientStatus();
        s->fp = fp;
        //初始化zlib上下文
        s->z_output = new z_stream();
        deflateInit(s->z_output, Z_DEFAULT_COMPRESSION);

        //创建输出过滤
        //filter事件延迟调用，避免频繁进入filter_out
        bufferevent* bev_filter = bufferevent_filter_new(be, 0, filter_out,\
                                                  BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS, 0, s);
        //bufferevent* bev_filter = bufferevent_filter_new(be, 0, filter_out,\
                                                  BEV_OPT_CLOSE_ON_FREE, 0, 0);
        
        //设置读取、写入和事件的回调
        bufferevent_setcb(bev_filter, client_read_cb, client_write_cb, client_event_cb, s);
        bufferevent_enable(bev_filter, EV_READ | EV_WRITE);

    }
}

void Client(event_base* base)
{
    cout << "begin client" << endl;
    //连接服务端
    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(5001);
    evutil_inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr.s_addr);

    bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    //只绑定连接事件回调，用来确认连接成功
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    bufferevent_setcb(bev, 0, 0, client_event_cb, 0);

    bufferevent_socket_connect(bev, (sockaddr*)&sin, sizeof(sin));
}
