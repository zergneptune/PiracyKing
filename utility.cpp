#include "utility.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

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
