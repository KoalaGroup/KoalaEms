#include <sconf.h>

#ifndef NOVOLATILE
#define VOLATILE volatile
#else
#define VOLATILE
#endif

struct inpinfo{
    int letztes_event;
    int use_driver,poll_driver;
    int path,space; /* war rtp,rtsp */
    VOLATILE int *ip;
    int base,*bufstart;
};
