#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <iostream>
#include <string.h>
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;
#define WEBROOT "."
#define DEFAULTINDEX "index.html"

static void http_cb(struct evhttp_request* request, void* arg)
{
    cout << "http_cb" <<endl;
    //1 获取游览器的请求信息
    //uri
    const char* uri = evhttp_request_get_uri(request);
    cout << "\n\nuri: " << uri << endl;

    // 请求类型 GET POST
    string cmdtype;
    switch(evhttp_request_get_command(request))
    {
    case EVHTTP_REQ_GET:
        cmdtype = "GET";
        break;
    case EVHTTP_REQ_POST:
        cmdtype = "POST";
        break;
    }
    cout << "cmdtype: " << cmdtype << endl;

    //消息头
    evkeyvalq* headers = evhttp_request_get_input_headers(request);
    cout << "=====headers======" << endl;
    for(evkeyval* p = headers->tqh_first; p != NULL; p = p->next.tqe_next)
    {
        cout << p->key << ": " << p->value << endl;
    }
    //请求正文
    evbuffer* inbuf = evhttp_request_get_input_buffer(request);
    char buf[1024] = {0};
    cout << "======Input data======" << endl;
    while(evbuffer_get_length(inbuf))
    {
        int n = evbuffer_remove(inbuf, buf, sizeof(buf) - 1);
        if(n > 0)
        {
            buf[n] = '\0';
            cout << buf << endl;
        }
    }
    

    //2 回复游览器
    //状态行 消息报头 响应正文
    

    //分析出请求的文件 uri
    // 设置根目录 WEBROOT
    string filepath = WEBROOT;
    filepath += uri;
    if(strcmp(uri, "/") == 0)
    {
        filepath += DEFAULTINDEX;
    }
    //返回消息报头
    evkeyvalq* outhead = evhttp_request_get_output_headers(request);
    //要支持图片 js css 下载zip文件
    //获取文件的后缀名
    int pos = filepath.rfind(".");
    string postfix = filepath.substr(pos + 1);
    if(postfix == "jpg" || postfix == "gif" || postfix == "png" || postfix == "bmp")
    {
        string temp = "image/" + postfix;
        evhttp_add_header(outhead, "Content-Type", temp.c_str());
    }
    else if(postfix == "zip")
    {
        evhttp_add_header(outhead, "Content-Type", "application/zip)");
    }
    else if(postfix == "html")
    {
        //evhttp_add_header(outhead, "Content-Type", "text/html;charset=utf8");
        evhttp_add_header(outhead, "Content-Type", "text/html");
    }
    else if(postfix == "css")
    {
        evhttp_add_header(outhead, "Content-Type", "text/css");
    }

    FILE* fp = fopen(filepath.c_str(), "rb");
    if(!fp)
    {
        evhttp_send_reply(request, HTTP_NOTFOUND, "", 0);
        return;
    }
    evbuffer* outbuf = evhttp_request_get_output_buffer(request);
    int len = fread(buf, 1, sizeof(buf), fp);
    evbuffer_add(outbuf, buf, len);
    fclose(fp);
    evhttp_send_reply(request, HTTP_OK, "", outbuf);

    
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

    //http服务器
    //1. 创建evhttp上下文
    evhttp* evh = evhttp_new(base);

    //2. 绑定端口和IP
    if(evhttp_bind_socket(evh, "0.0.0.0", 8080) != 0)
    {
        cout << "evhttp_bind_socket failed!" << endl;
    }

    //3. 设定回调函数
    evhttp_set_gencb(evh,
                     http_cb,
                     0);//回调参数

    //event服务器
    cout << "test http server" << endl;

    //进入事件主循环
    event_base_dispatch(base);
    event_base_free(base);
    evhttp_free(evh);
    
    return 0;
}
