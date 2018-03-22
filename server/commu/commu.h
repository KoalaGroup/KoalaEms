/*
 * commu/commu.h
 * 
 * $ZEL: commu.h,v 1.11 2017/10/20 23:21:31 wuestner Exp $
 */

#ifndef _commu_h_
#define _commu_h_

#include <stdio.h>
#include <sys/time.h>
#include <emsctypes.h>
#include "../main/scheduler.h"
#include <msg.h>
#include <errorcodes.h>

#define complain(fmt, arg...)           \
    do {                                \
        char s[8192];                   \
        snprintf(s, 8192, fmt, ## arg); \
        printf("%s\n", s);              \
        send_unsol_text(s, 0);          \
    } while (0)

typedef enum {conn_no, conn_opening, conn_ok, conn_closing} connstates;

/* in commu.c */
extern connstates connstate;

/* in unsol.c */
int send_unsolicited(UnsolMsg type, ems_u32* body, unsigned int size);
int send_unsol_var(UnsolMsg type, unsigned int num, ...);
int send_unsol_warning(int code, unsigned int num, ...);
int send_unsol_patience(int code, unsigned int num, ...);
int send_unsol_text(const char *text, ...);
int send_unsol_text2(const char *text, ...);
int send_unsol_do_filename(int do_idx, char *name);
#if 0
int send_unsol_alarm_0(int severity, struct timeval,
        int i_counter, ems_u32 *i_vector,
        int s_counter, const char **s_vector);
#endif

void select_command(int, enum select_types, union callbackdata);
Request wait_indication(ems_u32** reqbuf, unsigned int* size);
void send_response(unsigned int size);
void free_indication(ems_u32* ptr);
Request poll_indication(ems_u32** reqbuf, unsigned int* size);
Request select_indication(ems_u32** reqbuf, unsigned int* size);
void conn_abort(char* msg);
errcode init_comm(char* port);
errcode done_comm(void);
int commu_outsize(void);

/* in socket/sockselectcomm.c */
int init_select_comm(void);
void done_select_comm(void);
void restart_accept(void);
void abort_conn(char* s);

/* in socket/socketcomm.c */
void close_connection(void);
errcode _init_comm(char* port);
errcode _done_comm(void);
int recv_data(ems_u32* buf, unsigned int len);
int send_data(ems_u32* buf, unsigned int len);
int open_connection(void);
int poll_connection(void);
int poll_data(ems_u32* buf, unsigned int len);
int printuse_port(FILE* outfilepath);

/* in command_interp.c */
int init_commandinterp(void);
int done_commandinterp(void);

#endif
