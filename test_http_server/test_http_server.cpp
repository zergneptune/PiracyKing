#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <string.h>
#include <jsoncpp/json/json.h>
#include "test_http_client.h"
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;
#define WEBROOT "."
#define DEFAULTINDEX "index.html"

static Json::Value get_text(const std::string& str_content) {
    Json::Value root;
    root["type"] = "TEXT";
    root["content"] = str_content;
    return root;
}

static Json::Value get_link(const std::string& str_link) {
    Json::Value root;
    root["type"] = "LINK";
    root["href"] = str_link;
    return root;
}

static std::string get_send_msg(Json::Value& input) {
    Json::Value br_elem;
    br_elem["type"] = "TEXT";
    br_elem["content"] = "\n";
    std::string summary = input["annotations"]["summary"].asString();
    std::string link = "http://10.12.74.65:8300/d/mFEJ854Mz/zhu-ji-jian-kong?orgId=1&refresh=5s";

    Json::Value userids;
    userids.append("changhuan");
    userids.append("xiangyu_cd");
    Json::Value at_elem;
    at_elem["type"] = "AT";
    at_elem["atall"] = false;
    at_elem["atuserids"] = userids;

    Json::Value root;
    Json::Value message;
    Json::Value body;
    body.append(get_text("告警信息: "));
    body.append(get_text(summary));
    body.append(br_elem);
    body.append(get_text("仪表盘: "));
    body.append(get_link(link));
    body.append(br_elem);
    body.append(at_elem);
    message["body"] = body;
    root["message"] = message;

	std::ostringstream ostr;
    Json::StreamWriterBuilder builder;
    builder["emitUTF8"] = true;//output utf-8
    builder["indentation"] = "";//no indent
    std::unique_ptr<Json::StreamWriter> s_writer(builder.newStreamWriter());
    s_writer->write(root, &ostr);
    return ostr.str();
}

static void send_infoflow(struct evhttp_request* request, std::string& input_data) {
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(input_data, root)) {
        std::cout << "json解析输入数据失败" << std::endl;
        evhttp_send_reply(request, HTTP_BADREQUEST, "", 0);
        return;
    }

    if (root.isMember("alerts")) {
        if (root["alerts"].isArray()) {
            for (auto& one_alert : root["alerts"]) {
                std::string send_msg = get_send_msg(one_alert);
                TestPostHttp("http://apiin.im.baidu.com/api/msg/groupmsgsend?access_token=d55f306154a9c4425cf35161e220384b1", send_msg);
            }
        }
    }

    evbuffer* outbuf = evhttp_request_get_output_buffer(request);
    evbuffer_add(outbuf, "ok", 2);
    evhttp_send_reply(request, HTTP_OK, "", outbuf);
}

static void do_uri(struct evhttp_request* request, const char* uri, std::string& input_data) {
    char buf[1024] = {0};
    string filepath = WEBROOT;
    if (strcmp(WEBROOT, ".") == 0) {
        getcwd(buf, 1024);
        filepath = buf;
    }
    filepath += uri;
    if(strcmp(uri, "/") == 0) {
        filepath += DEFAULTINDEX;
    }
    int pos = filepath.find_last_of("/\\");
    string filename = filepath.substr(pos + 1);
    std::cout << "filename = " << filename << std::endl;
    pos = filename.rfind(".");
    if (pos != std::string::npos) {
        string file_postfix = filename.substr(pos + 1);
        evhttp_send_reply(request, HTTP_NOTFOUND, "", 0);
    }

    if (strcmp(uri, "/send_infoflow") == 0) {
        std::cout << "do send_infoflow" << std::endl;
        send_infoflow(request, input_data);
    } else {
        evhttp_send_reply(request, HTTP_NOTFOUND, "", 0);
    }
}

static void http_cb(struct evhttp_request* request, void* arg)
{
    cout << "\nhttp_server_cb" <<endl;
    //1 获取游览器的请求信息
    //uri
    const char* uri = evhttp_request_get_uri(request);
    cout << "\n=====request======" << endl;
    cout << "uri: " << uri << endl;

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
    cout << "method: " << cmdtype << endl;

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
    std::string input_data;
    while(evbuffer_get_length(inbuf))
    {
        int n = evbuffer_remove(inbuf, buf, sizeof(buf) - 1);
        if(n > 0)
        {
            buf[n] = '\0';
            cout << buf << endl;
            input_data += buf;
        }
    }
    std::cout << "\n";
    do_uri(request, uri, input_data);
}

int main(int argc, char* argv[])
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
    std::string str_ip = "0.0.0.0";
    int n_port = 8081;
    if (argc > 1) {
        n_port = stoi(argv[1]);
    }
    if(evhttp_bind_socket(evh, str_ip.c_str(), n_port) != 0)
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
    std::cout << "listening on " << str_ip << ":" << n_port << std::endl;
    event_base_dispatch(base);
    event_base_free(base);
    evhttp_free(evh);
    
    return 0;
}
