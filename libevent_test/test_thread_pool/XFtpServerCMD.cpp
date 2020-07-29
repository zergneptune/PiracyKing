#include "XFtpServerCMD.h"
#include <iostream>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <string.h>

static void EventCB(bufferevent* bev, short what, void* arg)
{
    XFtpServerCMD* cmd = (XFtpServerCMD*)arg;
    if(what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
    {
        std::cout << "BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT" << std::endl;
        bufferevent_free(bev);
        delete cmd;
    }
}

static void ReadCB(bufferevent* bev, void* arg)
{
    XFtpServerCMD* cmd = (XFtpServerCMD*)arg;
    char data[1024] = { 0 };
    for(;;)
    {
        int len = bufferevent_read(bev, data, sizeof(data) - 1);
        if(len <= 0)
            break;
        data[len] = '\0';
        std::cout << data << std::flush;

        if(strstr(data, "quit"))
        {
            bufferevent_free(bev);
            delete cmd;
        }
    }
}

XFtpServerCMD::XFtpServerCMD()
{
}

XFtpServerCMD::~XFtpServerCMD()
{

}

bool XFtpServerCMD::Init()
{
    std::cout << "XFtpServerCMD Init" << std::endl;
    //监听socket bufferevent
    //base socket
    bufferevent* bev = bufferevent_socket_new(m_base, m_n_sock, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, ReadCB, 0, EventCB, this);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    //添加超时
    timeval rt = {10, 0};
    bufferevent_set_timeouts(bev, &rt, 0);


    return true;
}


