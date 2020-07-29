#include "XFtpLIST.h"
#include <iostream>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <string>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif
using namespace std;

void XFtpLIST::Write(struct bufferevent *bev)
{
	//4 226 Transfer complete发送完成
	ResCMD("226 Transfer complete\r\n");
	//5 关闭连接
	Close();

}
void XFtpLIST::Event(struct bufferevent *bev, short what)
{
	//如果对方网络断掉，或者机器死机有可能收不到BEV_EVENT_EOF数据
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		cout << "XFtpLIST BEV_EVENT_EOF | BEV_EVENT_ERROR |BEV_EVENT_TIMEOUT" << endl;
		Close();
	}
	else if(what&BEV_EVENT_CONNECTED)
	{
		cout << "XFtpLIST BEV_EVENT_CONNECTED" << endl;
	}
}
//解析协议
void XFtpLIST::Parse(std::string type, std::string msg)
{
	string resmsg = "";

	if (type == "PWD")
	{
		//257 "/" is current directory.
		resmsg = "257 \"";
		resmsg += cmdTask->curDir;
		resmsg += "\" is current dir.\r\n";

		ResCMD(resmsg);
	}
	else if (type == "LIST")
	{
		//1连接数据通道 2 150 3 发送目录数据通道 4 发送完成226 5 关闭连接
		//命令通道回复消息 使用数据通道发送目录
		//-rwxrwxrwx 1 root group 64463 Mar 14 09:53 101.jpg\r\n
		//1 连接数据通道 
		ConnectPORT();
		//2 1502 150
		ResCMD("150 Here comes the directory listing.\r\n");
		//string listdata = "-rwxrwxrwx 1 root group 64463 Mar 14 09:53 101.jpg\r\n";
        std::string listdata = GetListData(cmdTask->rootDir + cmdTask->curDir);
		//3 数据通道发送
		Send(listdata);
	}
    else if (type == "CWD")//切换目录
    {
        //取出命令中的路径
        //CWD test\r\n
        int pos = msg.rfind(" ") + 1;
        //去掉结尾的\r\n
        std::string path = msg.substr(pos, msg.size() - pos - 2);
        if (path[0] == '/')
        {
            cmdTask->curDir = path;
        }
        else
        {
            if (cmdTask->curDir[cmdTask->curDir.size() - 1] != '/')
            {
                cmdTask->curDir += "/";
            }
            cmdTask->curDir += path + "/";
        }
        ResCMD("250 Directory changed success.\r\n");
    }
    else if (type == "CDUP")//回到上层目录
    {
        // /Debug/test /Debug /Debug/
        std::string cur_path = cmdTask->curDir;
        if (cur_path == "/")
        {
            ResCMD("250 Directory changed success.\r\n");
        }

        //统一去掉结尾的 /
        if (cur_path[cur_path.size() - 1] == '/')
        {
            cur_path = cur_path.substr(0, cur_path.size() - 1);
        }

        // /Debug/test.xxx /Debug 
        int pos = cur_path.rfind("/");
        cur_path = cur_path.substr(0, pos);
        cmdTask->curDir = cur_path;
        ResCMD("250 Directory changed success.\r\n");
    }
}

std::string XFtpLIST::GetListData(std::string path)
{
    //string listdata = "-rwxrwxrwx 1 root group 64463 Mar 14 09:53 101.jpg\r\n";
    std::string data = "";
#ifdef _WIN32
    //存储文件信息
    _finddata_t file;
    //目录上下文
    path += "/*.*";
    intptr_t dir = _findfirst(path.c_str(), $file);
    if (dir < 0)
    {
        return data;
    }

	do
    {
        std::string tmp;
		//是否是目录去掉 . 和 ..
		if (file.attrib & _A_SUBDIR)
        {
            if (strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0)
            {
                continue;
            }
            tmp = "drwxrwxrwx 1 root group ";
        }
        else
        {
            tmp = "-rwxrwxrwx 1 root group ";
        }

        //文件大小
        char buf[1024];
        sprintf(buf, "%u", file.size);
        tmp += buf;

        //日期时间
        strftime(buf, sizeof(buf) - 1, "%b %d %H:%M ", localtime(file.time_write));
        tmp += buf;
        tmp += file.name;
        data += tmp;
        data += "\r\n";

    }while (_findNext(dir, $file) == 0);

    
#else
    DIR* dir;
    struct dirent* ptr;

    if ((dir = opendir(path.c_str())) == NULL)
	{
        perror("Open dir error...");
		return data;
    }
 
    while ((ptr = readdir(dir)) != NULL)
    {
        std::string temp;
		if (strcmp(ptr->d_name, ".")==0 || strcmp(ptr->d_name, "..") == 0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == DT_REG)    ///file
        {
            temp = "-rwxrwxrwx 1 root group ";
        }
        else if(ptr->d_type == DT_LNK)    ///link file
        {
            temp = "lrwxrwxrwx 1 root group ";
        }
		else if(ptr->d_type == DT_DIR)    ///dir
        {
            temp = "drwxrwxrwx 1 root group ";
        }
        //文件大小
        struct stat st;
        std::string file_path = path + "/" + ptr->d_name;
        int res = stat(file_path.c_str(), &st);
        if (res < 0)
        {
            std::string error("error open file ");
            error += file_path;
            perror(error.c_str());
            return data;
        }

        char buf[1024];
        sprintf(buf, "%ld", st.st_size);
        temp += buf;

        //日期时间
        strftime(buf, sizeof(buf) - 1, "%b %d %H:%M ", localtime(&st.st_mtime));
        temp += buf;
        temp += ptr->d_name;
        data += temp;
        data += "\r\n";
    }
    closedir(dir);
#endif

    return data;
}
