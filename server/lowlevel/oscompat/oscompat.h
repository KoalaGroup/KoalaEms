/*
 * lowlevel/oscompat/oscompat.h
 * 
 * $ZEL: oscompat.h,v 1.5 2005/12/04 21:59:22 wuestner Exp $
 */

#ifndef _oscompat_h_
#define _oscompat_h_

#include <stdio.h>
#include <errorcodes.h>

#ifdef OSK
#include <module.h>

typedef mod_exec *modresc;

typedef struct {
    int *start;
    int len;
} mmapresc;

#else /* unix || LynxOS */

typedef struct {
    int shmid;
    int *shmaddr;
    int len;
} modresc;

typedef struct {
  char *name;
  int *start;
  int len;
} mmapresc;

void tsleep(int hunds);

#endif

int *init_datamod(char*, int, modresc*);
void done_datamod(modresc* ref);
int *link_datamod(char*, modresc*);
void unlink_datamod(modresc*);
int *map_memory(int*, int, mmapresc*);
void unmap_memory(mmapresc*);
int oscompat_low_printuse(FILE* outfilepath);
errcode oscompat_low_init(char* arg);
errcode oscompat_low_done(void);

#endif
