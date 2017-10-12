/*
 * events/ev2ev.h
 */

#ifndef _ev2ev_h_
#define _ev2ev_h_

/* (not used)
struct eventheader {
    u_int32_t size;
    u_int32_t evno;
    u_int32_t trigno;
    u_int32_t nr_subevents;
};
*/

struct subevent {
    u_int32_t ID;
    u_int32_t length;
    u_int32_t *data;
};

struct event {
    u_int32_t *data;
    int eventnumber;
    int nr_subevents;
    struct subevent *subevents;
};

int manipulate_event(struct event*);

#endif
