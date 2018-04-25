/*
 * ems/events++/read_saved_beam.hxx
 * 
 * created 2015-Nov-25 PeWue
 * $ZEL: read_saved_beam.hxx,v 1.1 2015/11/26 01:05:12 wuestner Exp $
 */

#ifndef _read_saved_beam_hxx_
#define _read_saved_beam_hxx_

int get_beamdata(struct timeval *tv, int idx, float *val);
int init_beamdata(const char *directory);

#endif
