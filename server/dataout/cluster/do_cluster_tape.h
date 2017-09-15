#ifndef _do_cluster_tape_h_
#define _do_cluster_tape_h_

plerrcode tape_inquiry(int do_idx);
plerrcode tape_load(int do_idx, int action, int immediate);
plerrcode tape_erase(int do_idx, int immediate);
plerrcode tape_locate(int do_idx, int location);
plerrcode tape_space(int do_idx, int code, int count);
plerrcode tape_rewind(int do_idx, int location);
plerrcode tape_clearlog(int do_idx);
plerrcode tape_readpos(int do_idx);
plerrcode tape_prevent_allow_removal(int do_idx, int prevent);
plerrcode tape_modeselect(int do_idx, int density);
plerrcode tape_modesense(int do_idx);
plerrcode tape_write(int do_idx, int len, ems_u32* data);
plerrcode tape_writefilemarks(int do_idx, int num, int immediate, int force);
plerrcode tape_debug(int do_idx, int debug);

#endif
