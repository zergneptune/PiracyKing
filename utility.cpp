#include "utility.hpp"
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <json/json.h>

using std::cin;
using std::cout;

CSemaphore::CSemaphore(){}

CSemaphore::~CSemaphore(){}

void CSemaphore::Wait()
{
    std::unique_lock<std::mutex> lck(m_mtx);
    m_cv.wait(lck);
}

int CSemaphore::WaitFor(int ms)
{
    std::unique_lock<std::mutex> lck(m_mtx);
    std::cv_status res = m_cv.wait_for(lck, std::chrono::milliseconds(ms));
    if(res == std::cv_status::timeout)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

void CSemaphore::NoticeOne()
{
    std::unique_lock<std::mutex> lck(m_mtx);
    m_cv.notify_one();
}

void CSemaphore::NoticeAll()
{
    std::unique_lock<std::mutex> lck(m_mtx);
    m_cv.notify_all();
}

CSnowFlake::CSnowFlake()
{
    m_nMachineId = 0;
    m_nMaxSerialNum = (1 << 12) - 1;
    uint64_t curr_ms_ts = get_ms_ts();
    m_nKey = 0;
    m_nKey |= curr_ms_ts << 22;
    m_nKey |= m_nMachineId << 12;
    m_nLast_ms_ts = curr_ms_ts;
}

CSnowFlake::CSnowFlake(uint64_t machineid)
{
    //机器id大小范围（0~1023）
    m_nMachineId = machineid;
    m_nMaxSerialNum = (1 << 12) - 1;
    uint64_t curr_ms_ts = get_ms_ts();
    m_nKey = 0;
    m_nKey |= curr_ms_ts << 22;
    m_nKey |= m_nMachineId << 12;
    m_nLast_ms_ts = curr_ms_ts;
}

CSnowFlake::~CSnowFlake(){}

uint64_t CSnowFlake::get_sid()
{
    std::lock_guard<std::mutex> lck(m_mtx);
    uint64_t curr_ms_ts = get_ms_ts();
    if( curr_ms_ts > m_nLast_ms_ts )
    {
        m_nKey = 0;
        m_nKey |= curr_ms_ts << 22;
        m_nKey |= m_nMachineId << 12;
        m_nLast_ms_ts = curr_ms_ts;
        return m_nKey;
    }
    else
    {
        uint64_t nCurrSerialNum = m_nKey & m_nMaxSerialNum;
        //1毫秒内并发数超过序列号上限
        if( (nCurrSerialNum + 1) > m_nMaxSerialNum )
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            lck.~lock_guard();
            return get_sid();
        }
        else
        {
            //更新当前序列号
            m_nKey &= (~(m_nMaxSerialNum));
            m_nKey |= nCurrSerialNum + 1;
            return m_nKey;
        }
    }
}

uint64_t CSnowFlake::get_ms_ts()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t ms_ts = tv.tv_sec + tv.tv_usec / 1000;
    return ms_ts;
}

void setnonblocking(int sock)
{
    int opts;
    opts=fcntl(sock, F_GETFL);
    if(opts<0)
    {
        perror("fcntl(sock,GETFL)");
        exit(1);
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock, F_SETFL, opts)<0)
    {
        perror("fcntl(sock, SETFL, opts)");
        exit(1);
    }
}

void setreuseaddr(int sock)
{
    int opt = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
    if(res < 0)
    {
        perror("setsockopt");
        exit(1);
    }
}


int readn(int fd, void *vptr, int n)
{
    int nleft;
    int nread;
    char *ptr;
 
    ptr = (char*)vptr;
    nleft = n;
 
    while(nleft > 0)
    {
        nread = read(fd, ptr, nleft);
        if(nread < 0)
        {
            if(errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return -1;
        }
        else if(nread == 0)
        {
            break;  /*  EOF */
        }
        
        nleft -= nread;
        ptr   += nread;
    }
    return (n - nleft);
}
 
int writen(int fd, const void *vptr, int n)
{
    int nleft;
    int nwritten;
    const char *ptr;
 
    ptr = (const char *)vptr;
    nleft = n;
 
    while(nleft > 0)
    {
        nwritten = write(fd, ptr, nleft);
        if(nwritten <= 0)
        {
            if(nwritten < 0 && errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
 
        nleft -= nwritten;
        ptr   += nwritten;
    }
 
    return n;
}

void enter_any_key_to_continue()
{
    cout << "输入任意键继续...";
    get_input_string();
}

int get_input_number()
{
    int nInput = 0;
    while (1)
    {
        cin >> nInput;
        if (!cin.good())
        {
            cout << "输入错误，请重新输入: ";
            //cin.clear(); 清空输入流状态位
            cin.clear(cin.rdstate() & ~cin.failbit & ~cin.badbit & !cin.goodbit & !cin.eofbit);
            cin.ignore(128, '\n');
        }
        else
        {
            break;
        }
    }

    return nInput;
}

std::string get_input_string()
{
    std::string strInput;
    while (1)
    {
        cin >> strInput;
        if (!cin.good())
        {
            cout << "输入错误，请重新输入: ";
            //cin.clear(); 清空输入流状态位
            cin.clear(cin.rdstate() & ~cin.failbit & ~cin.badbit & !cin.goodbit & !cin.eofbit);
            cin.ignore(128, '\n');
        }
        else
        {
            break;
        }
    }

    return strInput;
}

int recurse_dir(char *basePath)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1000];

    if((dir = opendir(basePath)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }

    while((ptr = readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..")==0)
            continue;
        else if(ptr->d_type == 8)    ///file
            printf("d_name:%s/%s\n", basePath, ptr->d_name);
        else if(ptr->d_type == 10)    ///link file
            printf("d_name:%s/%s\n", basePath, ptr->d_name);
        else if(ptr->d_type == 4)    ///dir
        {
            memset(base, '\0', sizeof(base));
            strcpy(base, basePath);
            strcat(base, "/");
            strcat(base, ptr->d_name);
            recurse_dir(base);
        }
    }
    closedir(dir);
    return 1;
}

std::string w2c(const wchar_t * pw)
{
    std::string val = "";
    if(!pw)
    {
        return val;
    }
    size_t size= wcslen(pw)*sizeof(wchar_t);
    char *pc = NULL;
    if(!(pc = (char*)malloc(size)))
    {
        return val;
    }
    size_t destlen = wcstombs(pc,pw,size);
    /*转换不为空时，返回值为-1。如果为空，返回值0*/
    if (destlen ==(size_t)(0))
    {
        return val;
    }
    val = pc;
    delete pc;
    return val;
}

//读json文件
void readJsonFile(char* jsonFilePath)
{
    Json::Value root;//定义根节点
    Json::Reader reader;
    std::ifstream infile;
    infile.open(jsonFilePath, std::ifstream::in);
    if(infile.is_open())
    {
        if(reader.parse(infile, root))
        {
            Json::Value city_list = root["RECORDS"];
            if(city_list.isArray())
            {
                std::ofstream outfile("result.json", std::ofstream::out | std::ofstream::trunc);
                for(auto iter = city_list.begin(); iter != city_list.end(); ++iter)
                {
                    std::cout << "id = " << (*iter)["id"].asString() << "\t" << "name = " << (*iter)["name"].asString() << std::endl;
                    outfile << "['" << (*iter)["id"].asString() << "'] = '" << (*iter)["name"].asString() << "',\r\n";
                }
                outfile.close();
            }
            std::cout << "size = " << city_list.size() << std::endl;
        }
        infile.close();
    }
    else
    {
        std::cout << "erro open " << jsonFilePath << ": " << strerror(errno) << std::endl;
    }
}
