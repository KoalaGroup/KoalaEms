/*
 * findstring.cc
 * 
 * created: 24.01.96 PW
 * 16.07.1999 PW: interface changed
 * 2010.06.20 PW: sort function rewritten to avoid Tcl_LsortObjCmd
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <string.h>
#include <tcl.h>
#include "findstring.hxx"
#include <errors.hxx>
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("2010.06.20", __FILE__, __DATE__, __TIME__,
"$ZEL: findstring.cc,v 1.13 2010/06/20 22:17:41 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
/*
 * findstring compares a string (given) with a list of other strings (arr).
 * If an exact match is found the index of the found string in arr is
 * returned.
 * If an exact match is not found but the given string matches the start of
 * exactly one string of the array, the index of this string is returned.
 * If no match is found but ival is not zero the string is interpreted as
 * integer.
 * In this case the integer value is written to *ival and -2 is returned.
 * Otherwise an error message is generated and -1 is returned.
 */
int findstring(Tcl_Interp* interp, const char* given,
        const char* arr[], int* ival /* default 0*/)
{
    int anum=0;
    while (arr[anum]!=0) anum++;

    // string exakt?
    for (int i=0; i<anum; i++) {
        if (strcmp(given, arr[i])==0)
            return i;
    }

    // string abgekuerzt?
    int k, j=0, n, num;
    n=strlen(given);
    for (num=0, k=0; k<anum; k++) {
        if (strncmp(given, arr[k], n)==0) {
            j=k; 
            num++;
        }
    }

    // trotzdem eindeutig?
    if (num==1) {
        return j;
    } else if ((num==0) && (ival!=0)) {
        if (Tcl_GetInt(interp, given, ival)==TCL_OK)
            return -2;
    }

// generate error message
/*
 * In the old version the list of possible strings was sorted.
 * Is this really necessary?
 */
    Tcl_ResetResult(interp);
    if (num>1) {
        OSTRINGSTREAM st;
        st<<"ambigious arg "<<given<<"; possible args are: ";
        for (int i=0; i<anum; i++) {
            if (strncmp(given, arr[i], n)==0)
                st<<' '<<arr[i];
        }
        Tcl_SetResult_Stream(interp, st);
    } else { // num==0
        OSTRINGSTREAM st;
        st<<"unknown arg "<<given<<"; possible args are: ";
        for (int i=0; i<anum; i++)
            st<<' '<<arr[i];
        if (ival)
            st<<" or integer value";
        Tcl_SetResult_Stream(interp, st);
    }
    return -1;
}
/*****************************************************************************/
int findstring(Tcl_Interp* interp, Tcl_Obj* const given,
        const char* arr[], int* ival)
{
    return findstring(interp, Tcl_GetString(given), arr, ival);
}
/*****************************************************************************/
// error: num=24
// 0 dummy
// ...
// 22 string
// 23 confirmation
// 
// default former is domevent
int findstring(Tcl_Interp* interp, const char* given, strarr& sarr, int* ival)
{
    int res=-1;

    // create a real string array from strarr&
    const char *arr[sarr.num()+1];
    for (int i=0; i<sarr.num(); i++) {
        arr[i]=sarr.str(i);
    }
    arr[sarr.num()]=0;

    res=findstring(interp, given, arr, ival);

    return res;
}
/*****************************************************************************/
/*****************************************************************************/
