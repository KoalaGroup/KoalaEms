/*
 * histoarr.hxx
 * 
 * $ZEL: histoarr.hxx,v 1.6 2004/11/18 12:35:48 wuestner Exp $
 * 
 * created 19.02.96 PW
 */

#ifndef _histoarr_hxx_
#define _histoarr_hxx_

#define __STDC__ 1

#include <tcl.h>
#include <sys/time.h>
#include <String.h>

/*****************************************************************************/

class E_histoarray;

class C_histoarrays
  {
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

class E_histoarray
  {
  public:
    E_histoarray(Tcl_Interp*, C_histoarrays*, int, char*[]);
    ~E_histoarray();
    String tclprocname() const;
    String origtclprocname() const {return origprocname;}
    String name() const {return arrname;}
  protected:
    Tcl_Interp* interp;
    C_histoarrays* harrs;
    Tcl_Command tclcommand;
    String origprocname;
    String arrname;
    struct callback
      {
      void(*proc)(E_histoarray*, ClientData);
      ClientData data;
      };
    callback* datacallbacks;
    int numdatacallbacks;
    callback* deletecallbacks;
    int numdeletecallbacks;
    callback* rangecallbacks;
    int numrangecallbacks;
    struct pair
      {
      double time;
      double val;
      };
    pair* arr;
    int arrlimit; // max. Groesse ist 2^arrlimit
    int arrmask;  // 2^arrlimit-1
    int arrsize;  // Gesamtgroesse des Arrays
    int idx_a;    // realer Index des ersten Elements
    int idx_e;    // realer Index des naechsten zu schreibenden Elements
    void print(ostream&);
    int e_command(int, char*[]);
    int e_delete(int, char*[]);
    int e_add(int, char*[]);
    int e_limit(int, char*[]);
    int e_size(int, char*[]);
    int e_getval(int, char*[]);
    int e_gettime(int, char*[]);
    int e_getsec(int, char*[]);
    int e_get(int, char*[]);
    int e_leftidx(int, char*[]);
    int e_rightidx(int, char*[]);
    int e_clear(int, char*[]);
    int e_name(int, char*[]);
    int e_print(int, char*[]);
    int e_dump(int, char*[]);
    int e_restore(int, char*[]);
  public:
    static int create(ClientData, Tcl_Interp*, int, char*[]);
    static void destroy(ClientData);
    static int command(ClientData, Tcl_Interp*, int, char*[]);
    int checkidx(int);
    int size();
    int notempty() const {return arr!=0;}
    int empty() const {return arr==0;}
    void add(double, double);
    void clear();
    void limit(int);
    const pair* operator[](int);
    int leftidx(double, double, int);
    int rightidx(double, double, int);
    double time(int);
    double maxtime();
    double val(int);
    int getmaxxvals(double&, double&);
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
