/*
 * procs/fastbus/sis3100/fb_dspprocs.h
 * $ZEL: fb_dspprocs.h,v 1.1 2004/04/02 13:50:48 wuestner Exp $
 * created 2004-03-25 PW
 */

#ifndef _fb_dspprocs_h_
#define _fb_dspprocs_h_

/* MUST match sis3100_dsp/asm/vme_commu.h */
struct dsp_info {
    u_int32_t in_progress;  /* written from DSP */
    u_int32_t stopcode;     /* written from DSP */
    u_int32_t eventcounter; /* written from DSP */
    u_int32_t bufferstart;  /* read and written from DSP */
    u_int32_t buffersize;   /* written from DSP */
    u_int32_t triggermask;
    u_int32_t errorsource;
    u_int32_t reserved[7];
    u_int32_t testtrigger;
    u_int32_t debugbuffer;
    u_int32_t triggerlist[128];
};

struct descr_block {
    /* 0 */ u_int32_t ID;       /* ignored */
    /* 1 */ u_int32_t procedure;
    union {
        struct {
            u_int32_t pa;       /* pa of primary link */
            u_int32_t num;      /* max. count */
        } ph10CX;
        struct {
            u_int32_t pa;       /* pa of primary link */
            u_int32_t pat;      /* module pattern */
            u_int32_t bc;       /* broadcast class (0x5|(BC<<4)) */
            u_int32_t num;      /* max. count */
        } lc1881;
        struct {
            u_int32_t pa;       /* pa */
        } lc1875;
    } extra;
};

struct fb_dspproc {
    int modultype;
    int procid;
};

extern struct fb_dspproc fb_dspprocs[];

#endif
