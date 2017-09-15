/*
 * histoarr.cc
 * 
 * created 13.09.96 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include "newhistoarr.hxx"
#include <cmath>
#include <cfloat>
#include <cerrno>
#include <cstring>
#include "findstring.hxx"
#include "compat.h"
#include "pairarr_mem.hxx"
#include "pairarr_map.hxx"
#include "pairarr_file.hxx"
#include <errors.hxx>
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: newhistoarr.cc,v 1.11 2010/02/03 00:15:51 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
C_histoarrays::C_histoarrays(Tcl_Interp* interp)
:interp(interp), list(0), size(0)
{}
/*****************************************************************************/
C_histoarrays::~C_histoarrays()
{
    delete[] list;
}
/*****************************************************************************/
void C_histoarrays::destroy(ClientData clientdata, Tcl_Interp* interp)
{
    delete (C_histoarrays*)clientdata;
}
/*****************************************************************************/
void C_histoarrays::add(E_histoarray* arr)
{
    if (test(arr)) {
        cerr << "Warning: E_histoarray* " << (void*)arr <<
                " exists in C_histoarrays" << endl;
        return;
    }
    E_histoarray** help=new E_histoarray*[size+1];
    for (int i=0; i<size; i++) help[i]=list[i];
    delete[] list;
    list=help;
    list[size]=arr;
    size++;
}
/*****************************************************************************/
int C_histoarrays::sub(E_histoarray* arr)
{
    int i, j;
    for (i=0; (i<size) && (list[i]!=arr); i++);
    if (i==size) {
        cerr << "Warning: E_histoarray* " << (void*)arr
            << " dosn't exist in C_histoarrays" << endl;
        return -1;
    }
    for (j=i; j<size-1; j++) list[j]=list[j+1];
    size--;
    return 0;
}
/*****************************************************************************/
int C_histoarrays::test(E_histoarray* arr) const
{
    int i;
    for (i=0; (i<size) && (list[i]!=arr); i++);
    return i<size;
}
/*****************************************************************************/
#if 0
static void
dump_obj(Tcl_Obj* o)
{
    cerr<<"object dump:"<<endl;
    cerr<<"  refCount="<<o->refCount<<endl;
    cerr<<"  bytes   ="<<o->bytes<<endl;
    cerr<<"  length  ="<<o->length<<endl;
    cerr<<"  ObjType*="<<o->typePtr<<endl;
    if (o->typePtr) {
        if (o->typePtr->name)
            cerr<<"  ObjType ="<<o->typePtr->name<<endl;
        else
            cerr<<"  ObjType has no name"<<endl;
        cerr<<"  updateStringProc="<<o->typePtr->updateStringProc<<endl;
    } else {
        cerr<<"NO ObjType"<<endl;
    }
}
#endif
/*****************************************************************************/
E_histoarray::E_histoarray(Tcl_Interp* interp, C_histoarrays* harrs,
  int objc, Tcl_Obj* const objv[])
:
    interp(interp),
    harrs(harrs),
    datacallbacks(0),
    numdatacallbacks(0),
    deletecallbacks(0),
    numdeletecallbacks(0),
    rangecallbacks(0),
    numrangecallbacks(0),
    arr(0)
{
// histoarray tclname [-type mem|map|file] [-file {name}] [-name] displayname 
    static const char* switches[]={
        "-type",
        "-file",
        "-name",
        0
    };
    static const char* types[]={
        "memory",
        "mapped",
        "file",
        0
    };
    enum {mem, map, fil} typ;
    const char *type=0, *file=0, *tclname=0, *displayname=0;
    int obj_c=objc-1;
    Tcl_Obj *const*  obj_v=objv+1;

    while (obj_c>0) {
        char* v0=Tcl_GetString(obj_v[0]);
        if ((v0[0]=='-') && (strlen(v0)>1)) {
            if (obj_c<2) {
                goto falsch;
            }
            char* v1=Tcl_GetString(obj_v[1]);
            switch (findstring(interp, v0, switches)) {
            case 0: type=v1; break;
            case 1: file=v1; break;
            case 2: displayname=v1; break;
            default:
                throw TCL_ERROR; break;
            }
            obj_c-=2;
            obj_v+=2;
        } else {
            if (tclname==0) {
                tclname=v0;
            } else if (displayname==0) {
                displayname=v0;
            } else {
                goto falsch;
            }
            obj_c--;
            obj_v++;
        }
    }

    if (type==0) {
        typ=mem;
    } else {
        switch (findstring(interp, type, types)) {
        case 0: typ=mem; break;
        case 1: typ=map; break;
        case 2: typ=fil; break;
        default: throw TCL_ERROR; break;
        }
    }

    if (tclname==0) goto falsch;
    if (displayname==0) displayname=tclname;
    if (file==0) file=tclname;

    dispname=displayname;
    origprocname=tclname;

    switch (typ) {
    case mem: arr=new C_pairarr_mem(file); break;
    case map: arr=new C_pairarr_map(file); break;
    case fil: arr=new C_pairarr_file(file); break;
    }
    if (arr->invalid()) {
        Tcl_SetResult_String(interp, arr->errormsg());
        throw TCL_ERROR;
    }

    tclcommand=Tcl_CreateObjCommand(interp, (char*)origprocname.c_str(), command,
            (void*)this, destroy);

    harrs->add(this);

    return;

falsch:
    Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong args; must be name [-type mem|map|file] "
            "[-file {name}] [-name] displayname"), -1));
    throw TCL_ERROR;
}
/*****************************************************************************/
E_histoarray::~E_histoarray()
{
    delete arr;
    for (int i=0; i<numdeletecallbacks; i++)
        deletecallbacks[i].proc(this, deletecallbacks[i].data);
    delete[] datacallbacks;
    delete[] deletecallbacks;
    harrs->sub(this);
}
/*****************************************************************************/
int E_histoarray::create(ClientData clientdata, Tcl_Interp* interp,
    int objc, Tcl_Obj* const objv[])
{
    int res;
    try {
        new E_histoarray(interp, (C_histoarrays*)clientdata, objc, objv);
        res=TCL_OK;
    } catch (int e) {
        cerr<<"E_histoarray::E_histoarray failed"<<endl;
        res=e;
    }
    return res;
}
/*****************************************************************************/
void E_histoarray::destroy(ClientData clientdata)
{
    delete (E_histoarray*)clientdata;
}
/*****************************************************************************/
int E_histoarray::command(ClientData clientdata, Tcl_Interp* interp,
    int objc, Tcl_Obj* const objv[])
{
    E_histoarray* arr=(E_histoarray*)clientdata;
    return arr->e_command(objc, objv);
}
/*****************************************************************************/
int E_histoarray::xcommand(ClientData clientdata, Tcl_Interp* interp,
    int argc, const char* argv[])
{
    return TCL_OK;
}
/*****************************************************************************/
STRING E_histoarray::tclprocname(void) const
{
    STRING s;
    s=Tcl_GetCommandName(interp, tclcommand);
    return s;
}
/*****************************************************************************/
int E_histoarray::e_command(int objc, Tcl_Obj* const objv[])
{
    // <ARR> command
    int res;
    if (objc<2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("argument required"), -1));
        res=TCL_ERROR;
    } else {
        static const char* names[]={
            "delete",
            "add",
            "limit",
            "size",
            "getval",
            "gettime",
            "get",
            "leftidx",
            "rightindex",
            "clear",
            "name",
            "print",
            "dump",
            "restore",
            "flush",
            "truncate",
            0
        };
        switch (findstring(interp, objv[1], names)) {
            case 0: res=e_delete(objc, objv); break;
            case 1: res=e_add(objc, objv); break;
            case 2: res=e_limit(objc, objv); break;
            case 3: res=e_size(objc, objv); break;
            case 4: res=e_getval(objc, objv); break;
            case 5: res=e_gettime(objc, objv); break;
            case 6: res=e_get(objc, objv); break;
            case 7: res=e_leftidx(objc, objv); break;
            case 8: res=e_rightidx(objc, objv); break;
            case 9: res=e_clear(objc, objv); break;
            case 10: res=e_name(objc, objv); break;
            case 11: res=e_print(objc, objv); break;
            case 12: res=e_dump(objc, objv); break;
            case 13: res=e_restore(objc, objv); break;
            case 14: res=e_flush(objc, objv); break;
            case 15: res=e_truncate(objc, objv); break;
            default: res=TCL_ERROR; break;
        }
    }
    return res;
}
/*****************************************************************************/
int E_histoarray::e_delete(int objc, Tcl_Obj* const objv[])
{
    // <ARR> delete
    if (objc>2) {
        return Tcl_NoArgs(interp, 1, objv);
    }
    return Tcl_DeleteCommand(interp, (char*)tclprocname().c_str());
}
/*****************************************************************************/
int E_histoarray::e_size(int objc, Tcl_Obj* const objv[])
{
    // <ARR> size
    if (objc>2) {
        return Tcl_NoArgs(interp, 1, objv);
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(size()));
    return TCL_OK;
}
/*****************************************************************************/
int E_histoarray::e_add(int objc, Tcl_Obj* const objv[])
{
    // <ARR> add timestamp value
    if (objc!=4) {
        Tcl_WrongNumArgs(interp, 1, objv, "add time value");
        return TCL_ERROR;
    }
    double time;
    double value;
    if ((Tcl_GetDoubleFromObj(interp, objv[2], &time)!=TCL_OK) ||
            (Tcl_GetDoubleFromObj(interp, objv[3], &value)!=TCL_OK))
        return TCL_ERROR;
    add(time, value);
    return TCL_OK;
}
/*****************************************************************************/
void E_histoarray::limit(int limit)
{
    arr->limit(limit);
    for (int i=0; i<numrangecallbacks; i++)
        rangecallbacks[i].proc(this, rangecallbacks[i].data);
}
/*****************************************************************************/
int E_histoarray::e_limit(int objc, Tcl_Obj* const objv[])
{
    // <ARR> limit number
    int res=TCL_OK, num;
    switch (objc) {
    case 2:
        Tcl_SetObjResult(interp, Tcl_NewIntObj(limit()));
        break;
    case 3:
        if (Tcl_GetIntFromObj(interp, objv[2], &num)!=TCL_OK) return TCL_ERROR;
        limit(num);
        break;
    default:
        Tcl_WrongNumArgs(interp, 1, objv, "limit [number]");
        res=TCL_ERROR;
        break;
    }
    return res;
}
/*****************************************************************************/
int E_histoarray::e_truncate(int objc, Tcl_Obj* const objv[])
{
    // <ARR> truncate
    int res=TCL_OK;
    switch (objc) {
    case 2:
        truncate();
        break;
    default:
        Tcl_WrongNumArgs(interp, 1, objv, "truncate");
        res=TCL_ERROR;
        break;
    }
    return res;
}
/*****************************************************************************/
void E_histoarray::truncate(void)
{
    arr->forcelimit();
    for (int i=0; i<numrangecallbacks; i++)
        rangecallbacks[i].proc(this, rangecallbacks[i].data);
}
/*****************************************************************************/
int E_histoarray::e_getval(int objc, Tcl_Obj* const objv[])
{
    // <ARR> getval index [format]
    if ((objc<3) || (objc>4)) {
        Tcl_WrongNumArgs(interp, 1, objv, "getval index [format]");
        return TCL_ERROR;
    }
    int idx;
    if (Tcl_GetIntFromObj(interp, objv[2], &idx)!=TCL_OK)
        return TCL_ERROR;
    if (!checkidx(idx)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong index"), -1));
        return TCL_ERROR;
    }
    int form=0; // float
    if (objc>3) {
        // %d %f
        char* argv3=Tcl_GetString(objv[3]);
        if (strcmp(argv3, "%d")==0) {
            form=1; // integer
        } else if (strcmp(argv3, "%f")==0) {
            form=0; // float
        } else {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong format; must be %d or %f"),
                -1));
            return TCL_ERROR;
        }
    }
    Tcl_SetObjResult(interp, form?
            Tcl_NewIntObj((int)rint(val(idx))):
            Tcl_NewDoubleObj(val(idx)));
    return TCL_OK;
}
/*****************************************************************************/
int E_histoarray::e_gettime(int objc, Tcl_Obj* const objv[])
{
    // <ARR> gettime index [format]
    if ((objc<3) || (objc>4)) {
        Tcl_WrongNumArgs(interp, 1, objv, "gettime index [format]");
        return TCL_ERROR;
    }
    int idx;
    if (Tcl_GetIntFromObj(interp, objv[2], &idx)!=TCL_OK)  return TCL_ERROR;
    if (!checkidx(idx)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong index"), -1));
        return TCL_ERROR;
    }
    int form=0; // float
    if (objc>3) {
        // %d %f
        char* argv3=Tcl_GetString(objv[3]);
        if (strcmp(argv3, "%d")==0) {
            form=1; // integer
        } else if (strcmp(argv3, "%f")==0) {
            form=0; // float
        } else {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong format; must be %d or %f"),
                -1));
            return TCL_ERROR;
        }
    }
    Tcl_SetObjResult(interp, form?
            Tcl_NewIntObj((int)rint(time(idx))):
            Tcl_NewDoubleObj(time(idx)));
    return TCL_OK;
}
/*****************************************************************************/
int E_histoarray::e_get(int objc, Tcl_Obj* const objv[])
{
    // <ARR> get index
    if ((objc<3) || (objc>5)) {
        Tcl_WrongNumArgs(interp, 1, objv, "get index [[format] format]");
        return TCL_ERROR;
    }
    int idx;
    if (Tcl_GetIntFromObj(interp, objv[2], &idx)!=TCL_OK)  return TCL_ERROR;
    if (!checkidx(idx)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong index"), -1));
        return TCL_ERROR;
    }

    int form1, form2;
    if (objc>3) {
        // %d %f
        char* argv3=Tcl_GetString(objv[3]);
        if (strcmp(argv3, "%d")==0) {
            form1=1; // integer
        } else if (strcmp(argv3, "%f")==0) {
            form1=0; // float
        } else {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong format; must be %d or %f"),
                -1));
            return TCL_ERROR;
        }
        if (objc>4) {
            // %d %f
            char* argv4=Tcl_GetString(objv[4]);
            if (strcmp(argv4, "%d")==0) {
                form2=1; // integer
            } else if (strcmp(argv4, "%f")==0) {
                form2=0; // float
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong format; must be %d or %f"),
                    -1));
                return TCL_ERROR;
            }
        } else {
            form2=form1;
        }
    } else {
        form1=form2=0; // float
    }

    OSTRINGSTREAM st;
    switch (form1) {
    case 0:
        st << setprecision(20) << time(idx) << ' ';
        break;
    case 1:
        st << (int)rint(time(idx)) << ' ';
        break;
    }
    switch (form2) {
    case 0:
        st << setprecision(20) << val(idx);
        break;
    case 1:
        st << (int)rint(val(idx));
        break;
    }
    Tcl_SetResult_Stream(interp, st);
    return TCL_OK;
}
/*****************************************************************************/
int E_histoarray::e_leftidx(int objc, Tcl_Obj* const objv[])
{
    // <ARR> leftidx time
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "leftidx time");
        return TCL_ERROR;
    }
    double time;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &time)!=TCL_OK)
        return TCL_ERROR;
    Tcl_SetObjResult(interp, Tcl_NewIntObj(leftidx(time, 0)));
    return TCL_OK;
}
/*****************************************************************************/
int E_histoarray::e_rightidx(int objc, Tcl_Obj* const objv[])
{
    // <ARR> rightidx time
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "rightidx time");
        return TCL_ERROR;
    }
    double time;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &time)!=TCL_OK)
        return TCL_ERROR;
    Tcl_SetObjResult(interp, Tcl_NewIntObj(rightidx(time, 0)));
    return TCL_OK;
}
/*****************************************************************************/
int E_histoarray::e_clear(int objc, Tcl_Obj* const objv[])
{
    // <ARR> clear
    if (objc>2) {
        return Tcl_NoArgs(interp, 1, objv);
    }
    clear();
    return TCL_OK;
}
/*****************************************************************************/
int E_histoarray::e_name(int objc, Tcl_Obj* const objv[])
{
    // <ARR> name [newname]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "name [newname]");
        return TCL_ERROR;
    }

    Tcl_SetResult_String(interp, name());
    if (objc==3) dispname=Tcl_GetString(objv[2]);
    return TCL_OK;
}
/*****************************************************************************/
int E_histoarray::e_print(int objc, Tcl_Obj* const objv[])
{
    // <ARR> print
    if (objc>2) {
        return Tcl_NoArgs(interp, 1, objv);
    }
    if (!arr) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("empty"), -1));
        return TCL_OK;
    }
    OSTRINGSTREAM st;
    print(st);
    Tcl_SetResult_Stream(interp, st);
    return TCL_OK;
}
/*****************************************************************************/
int E_histoarray::e_dump(int objc, Tcl_Obj* const objv[])
{
    // <ARR> dump filename [startidx [endidx]]
    if ((objc<3) || (objc>5)) {
        Tcl_WrongNumArgs(interp, 1, objv, "dump filename [startidx [endidx]]");
        return TCL_OK; 
    }
    ofstream os(Tcl_GetString(objv[2]));
    if (!os) {
        OSTRINGSTREAM st;
        st << "open \"" << Tcl_GetString(objv[2]) << "\" ging schief: errno=" << errno;
        Tcl_SetResult_Stream(interp, st);
        return TCL_ERROR;
    }

    int aidx, eidx;
    if (objc>3) {
        if (Tcl_GetIntFromObj(interp, objv[3], &aidx)!=TCL_OK)  return TCL_ERROR;
        if (!checkidx(aidx)) {
            Tcl_AppendResult(interp, "wrong index " , Tcl_GetString(objv[2]), 0);
            return TCL_ERROR;
        }
        if (objc>4) {
            if (Tcl_GetIntFromObj(interp, objv[4], &eidx)!=TCL_OK)  return TCL_ERROR;
            if (!checkidx(eidx)) {
                Tcl_AppendResult(interp, "wrong index " , Tcl_GetString(objv[3]), 0);
                return TCL_ERROR;
            }
        } else {
            eidx=size()-1;
        }
    } else {
        aidx=0;
        eidx=size()-1;
    }
    os << setprecision(20);
    for (int i=aidx; i<eidx; i++) {
        os << time(i) << '\t' << val(i) << endl;
    }
    return TCL_OK;
}
/*****************************************************************************/
int E_histoarray::e_restore(int objc, Tcl_Obj* const objv[])
{
    // <ARR> restore filename
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "restore filename");
        return TCL_OK; 
    }

    ifstream is(Tcl_GetString(objv[2]));
    if (!is) {
        OSTRINGSTREAM st;
        st << "open \"" << Tcl_GetString(objv[2]) << "\" ging schief: errno=" << errno;
        Tcl_SetResult_Stream(interp, st);
        return TCL_ERROR;
    }
    clear();

    STRING a, b;
    int n=0;
    double last=0;
    while (is >> a) {
        is >> b;
        double va, vb;
        if (Tcl_GetDouble(interp, (char*)a.c_str(), &va)!=TCL_OK) {
            return TCL_ERROR;
        }
        if (Tcl_GetDouble(interp, (char*)b.c_str(), &vb)!=TCL_OK) {
            return TCL_ERROR;
        }
//      if (fabs(va-last)>10)
//          cerr << "last=" << setprecision(20) << last << "; now=" << va << endl;
//      else
            add(va, vb); 
        n++;
        last=va;
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(n));
    for (int i=0; i<numrangecallbacks; i++)
        rangecallbacks[i].proc(this, rangecallbacks[i].data);
    return TCL_OK;
}
/*****************************************************************************/
int E_histoarray::e_flush(int objc, Tcl_Obj* const objv[])
{
    // <ARR> flush
    if (objc!=1) {
        Tcl_WrongNumArgs(interp, 1, objv, "flush");
        return TCL_OK; 
    }
    arr->flush();
    return TCL_OK;
}
/*****************************************************************************/
void E_histoarray::add(double time, double val)
{
    arr->add(time, val);
    int i;
    for (i=0; i<numrangecallbacks; i++)
        rangecallbacks[i].proc(this, rangecallbacks[i].data);
    for (i=0; i<numdatacallbacks; i++)
        datacallbacks[i].proc(this, datacallbacks[i].data);
}
/*****************************************************************************/
void E_histoarray::clear()
{
    arr->clear();
    for (int i=0; i<numrangecallbacks; i++)
        rangecallbacks[i].proc(this, rangecallbacks[i].data);
}
/*****************************************************************************/
void E_histoarray::installdatacallback(void (*proc)(E_histoarray*, void*),
    ClientData data)
{
    callback* help=new callback[numdatacallbacks+1];
    for (int i=0; i<numdatacallbacks; i++)
        help[i]=datacallbacks[i];
    delete[] datacallbacks;
    datacallbacks=help;
    help[numdatacallbacks].proc=proc;
    help[numdatacallbacks].data=data;
    numdatacallbacks++;
}
/*****************************************************************************/
void E_histoarray::installrangecallback(void (*proc)(E_histoarray*, void*),
    ClientData data)
{
    callback* help=new callback[numrangecallbacks+1];
    for (int i=0; i<numrangecallbacks; i++)
        help[i]=rangecallbacks[i];
    delete[] rangecallbacks;
    rangecallbacks=help;
    help[numrangecallbacks].proc=proc;
    help[numrangecallbacks].data=data;
    numrangecallbacks++;
}
/*****************************************************************************/
void E_histoarray::installdeletecallback(void (*proc)(E_histoarray*, void*),
    ClientData data)
{
    callback* help=new callback[numdeletecallbacks+1];
    for (int i=0; i<numdeletecallbacks; i++)
        help[i]=deletecallbacks[i];
    delete[] deletecallbacks;
    deletecallbacks=help;
    help[numdeletecallbacks].proc=proc;
    help[numdeletecallbacks].data=data;
    numdeletecallbacks++;
}
/*****************************************************************************/
void E_histoarray::deletedatacallback(ClientData data)
{
    for (int i=0; i<numdatacallbacks; i++) {
        if (datacallbacks[i].data==data) {
            for (int j=i; j<numdatacallbacks-1; j++)
                datacallbacks[i]=datacallbacks[i+1];
            numdatacallbacks--;
            return;
        }
    }
}
/*****************************************************************************/
void E_histoarray::deleterangecallback(ClientData data)
{
    for (int i=0; i<numrangecallbacks; i++) {
        if (rangecallbacks[i].data==data) {
            for (int j=i; j<numrangecallbacks-1; j++)
                rangecallbacks[i]=rangecallbacks[i+1];
            numrangecallbacks--;
            return;
        }
    }
}
/*****************************************************************************/
void E_histoarray::deletedeletecallback(ClientData data)
{
    for (int i=0; i<numdeletecallbacks; i++) {
        if (deletecallbacks[i].data==data) {
            for (int j=i; j<numdeletecallbacks; j++)
                deletecallbacks[i]=deletecallbacks[i+1];
            numdeletecallbacks--;
            return;
        }
    }
}
/*****************************************************************************/
/*****************************************************************************/
