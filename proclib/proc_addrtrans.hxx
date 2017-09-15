/******************************************************************************
*                                                                             *
* proc_addtrans.hxx                                                           *
*                                                                             *
* created: 11.06.97                                                           *
* last changed: 11.06.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _proc_addtrans_hxx_
#define _proc_addtrans_hxx_

/*****************************************************************************/

class C_add_trans
  {
  friend class C_VED;
  public:
    typedef enum{VCC2117_512, VCC2117, VCC2118, E6, E7, FIC8232, FIC8234,
        STR330, STR340, FVSBI} cpu;
    C_add_trans(cpu slave);
    C_add_trans(cpu master, cpu slave);
    ~C_add_trans(){;}

  private:
    int ok;
    int masterdriver;
    cpu dest;
    cpu source;
    int offs;

  protected:
    void setoffs(int offset);

  public:
    operator int();
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
