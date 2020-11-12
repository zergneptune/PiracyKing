#include "test_http_client.h"

#include <event2/dns.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
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
    RequestContext* req_ctx = (RequestContext*)ctx;
    event_base* base = req_ctx->base;
    //服务端响应错误
    if(req == NULL)
    {
        int errcode = EVUTIL_SOCKET_ERROR();
        std::string str_err = evutil_socket_error_to_string(errcode);
        cout << "请求出错: " <<  str_err << std::endl;
        req_ctx->prom_rsp.set_value(str_err);
        return;
    }

    //获取path
    const char* path = evhttp_request_get_uri(req);
    cout << "request path is " << path << endl;

    //请求
    cout << "=====request headers======" << endl;
    evkeyvalq* headers = evhttp_request_get_output_headers(req);
    for(evkeyval* p = headers->tqh_first; p != NULL; p = p->next.tqe_next)
    {
        cout << p->key << ": " << p->value << endl;
    }
    cout << "=====response headers======" << endl;
    headers = evhttp_request_get_input_headers(req);
    for(evkeyval* p = headers->tqh_first; p != NULL; p = p->next.tqe_next)
    {
        cout << p->key << ": " << p->value << endl;
    }

    char buf[1024] = {0};
    //获取返回的code
    cout << "Response code : " << evhttp_request_get_response_code(req) << std::endl;

    //返回body
    cout << "=====response body======" << endl;
    std::string str_rsp;
    evbuffer* input = evhttp_request_get_input_buffer(req);
    for(;;)
    {
        int len = evbuffer_remove(input, buf, sizeof(buf) -1);
        if(len <= 0) break;
        buf[len] = 0;
        str_rsp += buf;
        cout << buf << std::endl;
    }
    req_ctx->prom_rsp.set_value(str_rsp);
    event_base_loopbreak(base);
}

int TestGetHttp(const std::string& http_url, RequestContext& req_ctx)
{
    event_base* base = event_base_new();
    evdns_base* dnsbase = evdns_base_new(base, 1);

    //生成请求信息 GET
    //string http_url = "http://ffmpeg.club/index.html";
    //http_url = "http://ffmpeg.club/101.jpg";

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
    if(port == -1)
    {
        port = 80;
    }
    cout << "port is " << port << endl;

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
    //bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    evhttp_connection* evcon = evhttp_connection_base_new(base, dnsbase, host, port);

    //http client 请求
    req_ctx.base = base;
    evhttp_request* req = evhttp_request_new(http_client_cb, &req_ctx);

    // 设置请求的header信息
    evkeyvalq* output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "Host", host);

    //设置超时时间
    //evhttp_connection_set_timeout(evcon, 60);

    //发起请求
    std::string uri_path = path;
    if (query) {
        uri_path += "?";
        uri_path += query;
    }
    evhttp_make_request(evcon, req, EVHTTP_REQ_GET, uri_path.c_str());

    //进入事件主循环
    if(base) {
        event_base_dispatch(base);
        event_base_free(base);
    }
    if (dnsbase) {
        evdns_base_free(dnsbase, 0);
    }
    if (evcon) {
        evhttp_connection_free(evcon);
    }
    return 0;
}

int TestPostHttp(const std::string& http_url, std::string& data, RequestContext& req_ctx)
{
    event_base* base = event_base_new();
    evdns_base* dnsbase = evdns_base_new(base, 1);

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
    //bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    evhttp_connection* evcon = evhttp_connection_base_new(base, dnsbase, host, port);

    //http client 请求
    req_ctx.base = base;
    evhttp_request* req = evhttp_request_new(http_client_cb, &req_ctx);

    // 设置请求的header信息
    evkeyvalq* output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "Host", host);
    evhttp_add_header(output_headers, "Content-Type", "application/json");

    //发送post数据
    cout << "post data: " << data << std::endl;
    evbuffer* output = evhttp_request_get_output_buffer(req);
    int res = evbuffer_add(output, data.c_str(), data.size());
    if (res != 0) {
        std::cout << "evbuffer_add error" << std::endl;
    }

    //设置超时时间
    //evhttp_connection_set_timeout(evcon, 60);

    //发起请求
    std::string uri_path = path;
    if (query) {
        uri_path += "?";
        uri_path += query;
    }
    evhttp_make_request(evcon, req, EVHTTP_REQ_POST, uri_path.c_str());

    //进入事件主循环
    if(base) {
        event_base_dispatch(base);
        event_base_free(base);
    }
    if (dnsbase) {
        evdns_base_free(dnsbase, 0);
    }
    if (evcon) {
        evhttp_connection_free(evcon);
    }
    return 0;
}

