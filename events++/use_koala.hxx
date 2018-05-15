/*
 * ems/events++/use_koala.hxx
 * 
 * created 2018-May-14 Yong.Z
 * $ZEL: use_koala.hxx$
 */

#ifndef _use_koala_hxx_
#define _use_koala_hxx_

#include "TH1F.h"

// The following functions are to be invoked in parse_koala.cc
void use_koala_setup(const char* filename);
int use_koala_event(koala_event *koala);
int use_koala_event(koala_event *koala, TH1F** h);
void use_koala_init(void);
void use_koala_done(void);

#endif
