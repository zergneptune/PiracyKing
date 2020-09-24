#include "test.h"

void haha() {
    av_log_set_level(AV_LOG_DEBUG);
    av_log(NULL, AV_LOG_DEBUG, "hello, world!\n");
    return;
}

int main()
{
    haha();
    return 0;
}

