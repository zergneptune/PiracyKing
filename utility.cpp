#include "utility.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

void setnonblocking(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL);
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


