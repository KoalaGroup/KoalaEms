/*
 * proclib/proc_veds.hxx
 * 
 * created: 11.06.97
 * 
 * $ZEL: proc_veds.hxx,v 2.3 2014/07/14 15:11:54 wuestner Exp $
 */

#ifndef _proc_veds_hxx_
#define _proc_veds_hxx_

#include <proc_ved.hxx>

/*****************************************************************************/

class C_veds
  {
  public:
    C_veds();
    ~C_veds();
  private:
    typedef struct
      {
      C_VED* ved;
      C_VED::VED_prior prior;
      } entry;
    entry* list;
    int listsize;
    int list_idx;
  protected:
    friend class C_VED;
    void add(C_VED* ved, C_VED::VED_prior prior);
    void remove(const C_VED* ved);
    conf_mode confmode;
  public:
    C_VED::VED_prior getprior(const C_VED* ved);
    void setprior(C_VED* ved, C_VED::VED_prior prior);
    void changeprior(C_VED* ved, int offs);
    void ResetVED_all();
    void ResetVED();
    void ResetVED_R();
    void StartReadout();
    void ResetReadout();
    void StopReadout();
    void ResumeReadout();
    void SetUnsol(int val);
    int  GetReadoutStatus(ems_u32* mineventcount, ems_u32* maxeventcount);
    void close();
    int  count() const {return(list_idx);}
    const char* operator[] (int idx) const;
    C_VED* find(int) const;
    void printlist() const;
    conf_mode def_confmode() const {return confmode;}
    conf_mode def_confmode(conf_mode);
    void set_confmode(conf_mode);
  };

extern C_veds veds;

#endif

/*****************************************************************************/
/*****************************************************************************/
