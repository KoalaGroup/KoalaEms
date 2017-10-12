/*
 * events/evmanip.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL$";

#include <stdio.h>
#include <sys/types.h>
#include "ev2ev.h"

static int
manipulate_subevent_1(struct subevent *subevent)
{
    return 0;
}

static int
manipulate_subevent_2(struct subevent *subevent)
{
    int i;
    u_int32_t *r_ptr, *w_ptr;

    /* very trivial zero suppresion */
    r_ptr=w_ptr=subevent->data;
    for (i=0; i<subevent->length; i++) {
        if (*r_ptr!=0) {
            *w_ptr++=*r_ptr;
        }
        r_ptr++;
    }
    subevent->length=w_ptr-subevent->data;

    return 0;
}

int
manipulate_event(struct event* event)
{
    int i;
    for (i=0; i<event->nr_subevents; i++) {
        switch (event->subevents[i].ID) {
        case 1:
            manipulate_subevent_1(event->subevents+i);
            break;
        case 2:
            manipulate_subevent_2(event->subevents+i);
            break;
        default:
            fprintf(stderr, "unknown subevent %d\n", event->subevents[i].ID);
            return -1;
        }
    }
    return 0;
}
