#ifndef _do_tape_
#define _do_tape_

#if 0
int low_tape_write_mark(int p);
int low_tape_fsf(int p, int num);
int low_tape_bsf(int p, int num);
int low_tape_fsr(int p, int num);
int low_tape_bsr(int p, int num);
int low_tape_rewind(int p);
int low_tape_seod(int p);
int low_tape_offline(int p);
int low_tape_retension(int p);
#ifdef __osf__
int low_tape_online(int p);
int low_tape_load(int p);
int low_tape_unload(int p);
int low_tape_clear_error(int p);
#endif
#endif
int low_tape_nop(int p);
int do_tape_get(int p);

#endif
