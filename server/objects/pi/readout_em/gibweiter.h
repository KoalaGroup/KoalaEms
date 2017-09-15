#ifndef _gibweiter_h_
#define _gibweiter_h_

void initgibweiter(void);
void gib_weiter(volatile int* buf, struct eventheader evh);
void gib_weiter_d(struct inpinfo* info, volatile int* buf, struct eventheader evh);

#endif
