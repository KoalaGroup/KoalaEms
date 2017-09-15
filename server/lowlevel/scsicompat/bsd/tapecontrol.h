#ifndef _tapecontrol_h_
#define _tapecontrol_h_

extern int pid;
extern char tapename[];

int init_scsi_commands(int path);
int done_scsi_commands();
void make_tapename(int path);
int status_DLT4000(int path, int* m);
int status_EXABYTE(int path, int* m);
int scsi_control_tape(int path, int* in, int* out);

#endif
