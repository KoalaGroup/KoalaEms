/*
 * tcl_clientlib/newhistoarr.hxx
 * 
 * $ZEL: newhistoarr.hxx,v 1.8 2014/07/14 15:13:26 wuestner Exp $
 * 
 * created 19.02.96
 */

#ifndef _histoarr_hxx_
#define _histoarr_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <tcl.h>
#include "pairarr.hxx"

using namespace std;

/*****************************************************************************/

class E_histoarray;

class C_histoarrays {
  public:
    C_histoarrays(Tcl_Interp*);
    ~C_histoarrays();
  protected:
    Tcl_Interp* interp;
    E_histoarray** list;
    int size;
  public:
    static void destroy(ClientData, Tcl_Interp*);
    void add(E_histoarray*);
    int sub(E_histoarray*);
    int test(E_histoarray*) const;
};

class E_histoarray {
  public:
    E_histoarray(Tcl_Interp*, C_histoarrays*, int, Tcl_Obj* const[]);
    ~E_histoarray();
    string tclprocname() const;
    string origtclprocname() const {return origprocname;}
    string name() const {return dispname;}
  protected:
    Tcl_Interp* interp;
    C_histoarrays* harrs;
    Tcl_Command tclcommand;
    string origprocname;
    string dispname;
    struct callback {
        void(*proc)(E_histoarray*, ClientData);
        ClientData data;
    };
    callback* datacallbacks;
    int numdatacallbacks;
    callback* deletecallbacks;
    int numdeletecallbacks;
    callback* rangecallbacks;
    int numrangecallbacks;
    C_pairarr* arr; // hier sind die eigentlichen Daten drin
    void print(ostream& os) const {arr->print(os);}
    int e_command(int, Tcl_Obj* const[]);
    int e_delete(int, Tcl_Obj* const[]);
    int e_add(int, Tcl_Obj* const[]);
    int e_limit(int, Tcl_Obj* const[]);
    int e_size(int, Tcl_Obj* const[]);
    int e_getval(int, Tcl_Obj* const[]);
    int e_gettime(int, Tcl_Obj* const[]);
    int e_getsec(int, Tcl_Obj* const[]);
    int e_get(int, Tcl_Obj* const[]);
    int e_leftidx(int, Tcl_Obj* const[]);
    int e_rightidx(int, Tcl_Obj* const[]);
    int e_clear(int, Tcl_Obj* const[]);
    int e_name(int, Tcl_Obj* const[]);
    int e_print(int, Tcl_Obj* const[]);
    int e_dump(int, Tcl_Obj* const[]);
    int e_restore(int, Tcl_Obj* const[]);
    int e_flush(int, Tcl_Obj* const[]);
    int e_truncate(int, Tcl_Obj* const[]);
  public:
    static int create(ClientData, Tcl_Interp*, int, Tcl_Obj* const[]);
    static void destroy(ClientData);
    static int command(ClientData, Tcl_Interp*, int, Tcl_Obj* const[]);
    static int xcommand(ClientData, Tcl_Interp*, int, const char*[]);

    int checkidx(int idx) const {return arr->checkidx(idx);}
    int size() const {return arr->size();}
    int notempty() const {return arr->notempty();}
    int empty() const {return arr->empty();}
    void add(double time, double val);
    void clear();
    int limit(void) const {return arr->limit();}
    void limit(int limit);
    void truncate(void);
    const vt_pair* operator[](int i) {return (*arr)[i];}
    int leftidx(double time, int exact) {return arr->leftidx(time, exact);}
    int rightidx(double time, int exact) {return arr->rightidx(time, exact);}
    double time(int i) {return arr->time(i);}
    double maxtime() {return arr->maxtime();}
    double val(int i) {return arr->val(i);}
    int getmaxxvals(double& min, double& max) {return arr->getmaxxvals(min, max);}
    void installdatacallback(void(*)(E_histoarray*, ClientData), ClientData);
    void deletedatacallback(ClientData);
    void installdeletecallback(void(*)(E_histoarray*, ClientData), ClientData);
    void deletedeletecallback(ClientData);
    void installrangecallback(void(*)(E_histoarray*, ClientData), ClientData);
    void deleterangecallback(ClientData);
};

#endif

/*****************************************************************************/
/*****************************************************************************/
