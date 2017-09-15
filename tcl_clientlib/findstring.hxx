/*
 * findstring.hxx
 * 
 * $ZEL: findstring.hxx,v 1.6 2006/02/14 21:38:28 wuestner Exp $
 * 
 * created: 24.01.96 PW
 * 16.07.1999 PW: interface changed
 */

#ifndef _findstring_hxx_
#define _findstring_hxx_

/*****************************************************************************/

class strarr {
  public:
    virtual ~strarr() {};
    virtual const char* str(int) const=0;
    virtual int num() const=0;
    virtual int lookup(const char*) const=0;
};

int findstring(Tcl_Interp* interp, const char* given, const char* arr[],
        int* ival=0);
int findstring(Tcl_Interp* interp, const char* given, strarr& arr,
        int* ival=0);
int findstring(Tcl_Interp* interp, Tcl_Obj* const given, const char* arr[],
        int* ival=0);

#endif

/*****************************************************************************/
/*****************************************************************************/
