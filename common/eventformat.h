/* $Id: eventformat.h,v 2.2 1996/01/16 13:44:20 drochner Exp $ */

#ifndef _eventformat_h_
#define _eventformat_h_

struct eventinfo{
  int evno;
  int trigno;
  int numofsubevs;
};

struct eventheader{
  int len;
  struct eventinfo info;
};

struct subeventheader{
  int isid;
  int subevlen;
};

#define PADDING_VAL -1

#endif
