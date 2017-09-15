/*
 * bong.hxx
 */
/* $ZEL: bong.hxx,v 1.2 2003/09/30 16:33:16 wuestner Exp $ */

#ifndef _bong_hxx_
#define _bong_hxx_

class bang
  {
  public:
    bang *prev, *next;
    bang(bang* prev, float y, float v, float m, float mst);
  protected:
    static int map;
    static float g;
    static float f;
    struct state
      {
      float y, v, mt, mst;
      int pos;
      };
    typedef enum {
      crash_none, crash_ceiling, crash_floor, crash_upper, crash_lower,
      crash_upspalt, crash_downspalt
      } crash_type;
    int id;
    crash_type crashtype, crashed;
    float crash_time;
    state crashstate;
    state status;
    int crash_fix(char*, float y, float* time);
    int crash_moving(state* st, float* time);
  public:
    void move(float time, bang** hit);
    float check();
    static float gg(float);
    static float ff(float);
    static void clearmap() {map=0;}
    static int getmap() {return map;}
  };

#endif
