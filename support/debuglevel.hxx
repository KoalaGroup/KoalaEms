/*
 * debuglevel.hxx
 * 
 * created: ??.??.1998 PW
 */

#ifndef _debuglevel_hxx_
#define _debuglevel_hxx_

class debuglevel
  {
  public:
    static const int 
      D_COMM,  /* Kommunikation */
      D_TR,    /* Transport */
      D_DUMP,  /* Dump */
      D_MEM,   /* Speicherverwaltung */
      D_EV,    /* Eventgroessen und -Header */
      D_PL,    /* Prozedurlisten */
      D_USER,  /* Userprozeduren */
      D_REQ,   /* Requestbearbeitung */
      D_TRIG,  /* Readout ... */
      D_TIME,  /* Timing */
      D_FORK,  /* extra processes (tapehandler...) */
      D_LOW;   /* Lowlevel (Initalisierung) */
    static int verbose_level;
    static int debug_level;
    static int trace_level;
  };

#endif
