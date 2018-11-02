#pragma once
#define IF_EXIT(predict, err) if(predict){ perror(err); exit(1); }

void setnonblocking(int sock);

void setreuseaddr(int sock);

