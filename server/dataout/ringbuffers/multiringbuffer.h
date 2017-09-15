/******************************************************************************
*                                                                             *
* multiringbuffer.h                                                           *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 12.05.93                                                                    *
*                                                                             *
******************************************************************************/

#ifndef _multiringbuffer_h_
#define _multiringbuffer_h_

#include <sconf.h>
#include "../../lowlevel/profile/profile.h"

#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

/*
#ifdef OSK
#include <module.h>
#else
struct shmref{
    int shmid;
    int *shmaddr;
};
#endif
*/

#include "../../lowlevel/oscompat/oscompat.h"

    typedef struct
    {
	/* public: */
	int* next_databuf;
	int buffer_free;
	/* private: */
	int *bufbeg,*bufend;
	VOLATILE int *sp,*lp;
	int *last_databuf;
	int diff;
/*
#ifdef OSK
	mod_exec *ref;
#else
	struct shmref ref;
#endif
*/
	modresc ref;
#ifdef PROFILE
	int xref; /* wird nur hier gebraucht */
#endif
    } DATAOUT;

#endif

/*****************************************************************************/
/*****************************************************************************/
