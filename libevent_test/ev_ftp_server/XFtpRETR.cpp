#include "XFtpRETR.h"
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <iostream>

void XFtpRETR::Write(struct bufferevent *bev)
{
    if (!fp) return;
    int len = fread(buf, 1, sizeof(buf), fp);
    if (len <= 0)
    {
        fclose(fp);
        fp = 0;
        ResCMD("226 Tranfer complete\r\n");
        Close();
        return;
    }
    std::cout << "[" << len << "]" << std::flush;
    Send(buf, len);
}

void XFtpRETR::Event(struct bufferevent *bev,short what)
{
    //如果对方网络断掉，或者机器死机有可能收不到BEV_EVENT_EOF数据
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
        std::cout << "XFtpRETR BEV_EVENT_EOF | BEV_EVENT_ERROR |BEV_EVENT_TIMEOUT" << std::endl;
		Close();
        if (fp)
        {
            fclose(fp);
            fp = 0;
        }
	}
	else if(what&BEV_EVENT_CONNECTED)
    {
        std::cout << "XFtpRETR BEV_EVENT_CONNECTED" << std::endl;
	}
}

void XFtpRETR::Parse(std::string type, std::string msg)
{
    //取出文件名
    int pos = msg.rfind(" ") + 1;
    std::string filename = msg.substr(pos, msg.size() - pos - 2);
    std::string path = cmdTask->rootDir;
    path += cmdTask->curDir;
    path += filename;

    fp = fopen(path.c_str(), "rb");
    if (fp)
    {
        //连接数据通道
        ConnectPORT();

        //发送开始下载文件的指令
        ResCMD("150 File OK\r\n");

        //激活写入事件
        bufferevent_trigger(bev, EV_WRITE, 0);
    }
    else
    {
        ResCMD("450 file open failed!\r\n");
    }

}
