#include "utility.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

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
