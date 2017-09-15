/*
 * procs/unixvme/sis3100/vme_dspprocs.h
 * $ZEL: vme_dspprocs.h,v 1.4 2005/02/11 17:58:21 wuestner Exp $
 * created 25.Jun.2003 PW
 */

#ifndef _vme_dspprocs_h_
#define _vme_dspprocs_h_

/* MUST match sis3100_dsp/asm/vme_commu.h */
struct dsp_info {
      int32_t in_progress;  /* written from DSP */
    u_int32_t stopcode;     /* written from DSP */
      int32_t eventcounter; /* written from DSP */
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
    /* 0 */ u_int32_t ID;       /* written to event stream */
    /* 1 */ u_int32_t procedure;
    /* 2 */ u_int32_t addr;
    union {
        struct {
            u_int32_t ident; /* module identification */
            u_int32_t num;   /* number of datawords */
        } block;
        struct {
            u_int32_t ident; /* module identification */
        } v550;
        struct {
            /* 3 */ u_int32_t ident; /* module identification */
            /* 4 */ u_int32_t upper_threshold_0;
            /* 5 */ u_int32_t pedestal_0;
            /* 6 */ u_int32_t lower_threshold_0;
            /* 7 */ u_int32_t upper_threshold_1;
            /* 8 */ u_int32_t pedestal_1;
            /* 9 */ u_int32_t lower_threshold_1;
        } v550common;
        struct {
            u_int32_t ident; /* module identification */
            /* empty */
        } v775;
        struct {
            u_int32_t ident; /* module identification */
            u_int32_t read_addr; /* address to be used for readout */
                                 /* addr+0x200: read shadow */
                                 /* addr+0x280: read counter */
                                 /* addr+0x300: read and clear counter */
        } sis3800;
    } extra;
};

struct vme_dspproc {
    int modultype;
    int procid;
};

#define CAEN_V550_function_common_mode 5

extern struct vme_dspproc vme_dspprocs[];

#endif
