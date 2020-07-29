#include "XFtpSTOR.h"
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <iostream>


void XFtpSTOR::Read(struct bufferevent *bev)
{
    if (!fp) return;
    for(;;)
    {
        int len = bufferevent_read(bev, buf, sizeof(buf));
        if (len <= 0)
        {
            return;
        }
        int size = fwrite(buf, 1, len, fp);
        std::cout << "<" << len << ":"<< size << ">" << std::flush;
    }
}

void XFtpSTOR::Event(struct bufferevent *bev,short what)
{
    //如果对方网络断掉，或者机器死机有可能收不到BEV_EVENT_EOF数据
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
        std::cout << "XFtpSTOR BEV_EVENT_EOF | BEV_EVENT_ERROR |BEV_EVENT_TIMEOUT" << std::endl;
		Close();
        if (fp)
        {
            fclose(fp);
            fp = 0;
        }
        ResCMD("226 Transfer complete\r\n");
	}
	else if(what&BEV_EVENT_CONNECTED)
    {
        std::cout << "XFtpSTOR BEV_EVENT_CONNECTED" << std::endl;
	}

}

void XFtpSTOR::Parse(std::string type, std::string msg)
{
    //取出文件名
    int pos = msg.rfind(" ") + 1;
    std::string filename = msg.substr(pos, msg.size() - pos - 2);
    std::string path = cmdTask->rootDir;
    path += cmdTask->curDir;
    path += filename;

    fp = fopen(path.c_str(), "wb");
    if (fp)
    {
        //连接数据通道
        ConnectPORT();

        //发送开始下载文件的指令
        ResCMD("125 File OK\r\n");

        //激活读取事件
        bufferevent_trigger(bev, EV_READ, 0);
    }
    else
    {
        ResCMD("450 file open failed!\r\n");
    }

}
