#include <event2/event.h>
#include <event2/http.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>
#include <string.h>
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;

static void http_client_cb(struct evhttp_request* req, void* ctx)
{
    cout << "http_client_cb" << endl;
    event_base* base = (event_base*)ctx;
    //服务端响应错误
    if(req == NULL)
    {
        int errcode = EVUTIL_SOCKET_ERROR();
        evutil_socket_error_to_string(errcode);
        return;
    }

    //获取path
    const char* path = evhttp_request_get_uri(req);
    cout << "request path is " << path << endl;

    string filepath = ".";
    filepath += path;
    cout << "filepath is " << filepath << endl;
    FILE* fp = fopen(filepath.c_str(), "wb");
    if(!fp)
    {
        cout << "open file " << filepath << " failed!" << endl;
    }

    //获取返回的code
    cout << "Response : " << evhttp_request_get_response_code(req) << " ";
    cout << evhttp_request_get_response_code_line(req) << endl;
    char buf[1024] = {0};
    evbuffer* input = evhttp_request_get_input_buffer(req);
    for(;;)
    {
        int len = evbuffer_remove(input, buf, sizeof(buf) -1);
        if(len <= 0) break;
        buf[len] = 0;
        //cout << buf << fflush;
        if(!fp)
            continue;
        fwrite(buf, 1, len, fp);
    }
    if(fp)
        fclose(fp);
    
    event_base_loopbreak(base);
}

int TestGetHttp()
{
    event_base* base = event_base_new();

    //生成请求信息 GET
    string http_url = "http://ffmpeg.club/index.html";
    http_url = "http://ffmpeg.club/101.jpg";

    //分析url地址
    //uri
    evhttp_uri* uri = evhttp_uri_parse(http_url.c_str());
    const char* scheme = evhttp_uri_get_scheme(uri);
    if(!scheme)
    {
        cerr << "scheme is null" << endl;
        return -1;
    }
    cout << "scheme is " << scheme << endl;

    int port = evhttp_uri_get_port(uri);
    cout << "port is " << port << endl;
    if(port == -1)
    {
        port = 80;
    }

    //host
    const char* host = evhttp_uri_get_host(uri);
    if(!host)
    {
        cerr << "scheme is null" << endl;
        return -1;
    }
    cout << "host is " << host << endl;
    
    const char* path = evhttp_uri_get_path(uri);
    if(!path || strlen(path) == 0)
    {
        path = "/";
    }
    cout << "path is " << path << endl;

    const char* query = evhttp_uri_get_query(uri);
    if(query)
    {
        cout << "query is " << query << endl;
    }
    else
    {
        cout << "query is null" << endl;
    }

    //bufferevent 连接http服务器
    bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    evhttp_connection* evcon = evhttp_connection_base_bufferevent_new(base, NULL, bev, host, port);

    //http client 请求
    evhttp_request* req = evhttp_request_new(http_client_cb, base);

    // 设置请求的header信息
    evkeyvalq* output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "Host", host);

    //发起请求
    evhttp_make_request(evcon, req, EVHTTP_REQ_GET, path);
    

    //进入事件主循环
    if(base)
    {
        event_base_dispatch(base);
        event_base_free(base);
    }
    return 0;
}

int TestPostHttp()
{
     event_base* base = event_base_new();

    //生成请求信息 POST
    string http_url = "http://127.0.0.1:8080/index.html";

    //分析url地址
    //uri
    evhttp_uri* uri = evhttp_uri_parse(http_url.c_str());
    const char* scheme = evhttp_uri_get_scheme(uri);
    if(!scheme)
    {
        cerr << "scheme is null" << endl;
        return -1;
    }
    cout << "scheme is " << scheme << endl;

    int port = evhttp_uri_get_port(uri);
    cout << "port is " << port << endl;
    if(port == -1)
    {
        port = 80;
    }

    //host
    const char* host = evhttp_uri_get_host(uri);
    if(!host)
    {
        cerr << "scheme is null" << endl;
        return -1;
    }
    cout << "host is " << host << endl;
    
    const char* path = evhttp_uri_get_path(uri);
    if(!path || strlen(path) == 0)
    {
        path = "/";
    }
    cout << "path is " << path << endl;

    const char* query = evhttp_uri_get_query(uri);
    if(query)
    {
        cout << "query is " << query << endl;
    }
    else
    {
        cout << "query is null" << endl;
    }

    //bufferevent 连接http服务器
    bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    evhttp_connection* evcon = evhttp_connection_base_bufferevent_new(base, NULL, bev, host, port);

    //http client 请求
    evhttp_request* req = evhttp_request_new(http_client_cb, base);

    // 设置请求的header信息
    evkeyvalq* output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "Host", host);

    //发送post数据
    evbuffer* output = evhttp_request_get_output_buffer(req);
    evbuffer_add_printf(output, "xcj=%d&b=%d", 1, 2);

    //发起请求
    evhttp_make_request(evcon, req, EVHTTP_REQ_POST, path);
    

    //进入事件主循环
    if(base)
    {
        event_base_dispatch(base);
        event_base_free(base);
    }
    return 0;
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
    TestGetHttp();

    //TestPostHttp();
   
    return 0;
}
