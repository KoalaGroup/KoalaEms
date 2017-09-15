/*
 * histoarr.cc
 * 
 * created 19.02.96 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include "histoarr.hxx"
#include <cmath>
#include <cfloat>
#include <cerrno>
#include "findstring.hxx"
#include "compat.h"
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("Nov 16 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: histomaparr.cc,v 1.7 2010/02/03 00:15:51 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_histoarrays::C_histoarrays(Tcl_Interp* interp)
:list(0), size(0), interp(interp)
{}

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
if (test(arr))
  {
  cerr << "Warning: E_histoarray* " << (void*)arr << " exists in C_histoarrays"
      << endl;
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
if (i==size)
  {
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

E_histoarray::E_histoarray(Tcl_Interp* interp, C_histoarrays* harrs,
  int argc, char* argv[])
:arr(0), arrlimit(16), arrsize(0), interp(interp), harrs(harrs),
    datacallbacks(0), numdatacallbacks(0),
    deletecallbacks(0), numdeletecallbacks(0), rangecallbacks(0),
    numrangecallbacks(0)
{
// histoarray name arrayname
if ((argc!=2) && (argc!=3))
  {
  Tcl_SetResult(interp, "wrong # args; must be name [arrayname]", TCL_STATIC);
  throw TCL_ERROR;
  }
origprocname=argv[1];
if (argc==3)
  arrname=argv[2];
else
  arrname=argv[1];
tclcommand=Tcl_CreateCommand(interp, origprocname, command, ClientData(this),
    destroy);
#ifdef __osf__
write_rnd(FP_RND_RN);
#endif
harrs->add(this);
}

/*****************************************************************************/

E_histoarray::~E_histoarray()
{
delete[] arr;
for (int i=0; i<numdeletecallbacks; i++)
    deletecallbacks[i].proc(this, deletecallbacks[i].data);
delete[] datacallbacks;
delete[] deletecallbacks;
harrs->sub(this);
}

/*****************************************************************************/

int E_histoarray::create(ClientData clientdata, Tcl_Interp* interp,
    int argc, char* argv[])
{
int res;
try
  {
  E_histoarray* hist=new E_histoarray(interp, (C_histoarrays*)clientdata,
  argc, argv);
  res=TCL_OK;
  }
catch (int e)
  {
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
    int argc, char* argv[])
{
E_histoarray* arr=(E_histoarray*)clientdata;
return arr->e_command(argc, argv);
}

/*****************************************************************************/

String E_histoarray::tclprocname(void) const
{
String s;
s=Tcl_GetCommandName(interp, tclcommand);
return s;
}

/*****************************************************************************/

int E_histoarray::e_command(int argc, char* argv[])
{
// <ARR> command
int res;
if (argc<2)
  {
  Tcl_SetResult(interp, "argument required", TCL_STATIC);
  res=TCL_ERROR;
  }
else
  {
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
    0};
  switch (findstring(interp, argv[1], names))
    {
    case 0: res=e_delete(argc, argv); break;
    case 1: res=e_add(argc, argv); break;
    case 2: res=e_limit(argc, argv); break;
    case 3: res=e_size(argc, argv); break;
    case 4: res=e_getval(argc, argv); break;
    case 5: res=e_gettime(argc, argv); break;
    case 6: res=e_get(argc, argv); break;
    case 7: res=e_leftidx(argc, argv); break;
    case 8: res=e_rightidx(argc, argv); break;
    case 9: res=e_clear(argc, argv); break;
    case 10: res=e_name(argc, argv); break;
    case 11: res=e_print(argc, argv); break;
    case 12: res=e_dump(argc, argv); break;
    case 13: res=e_restore(argc, argv); break;
    default: res=TCL_ERROR; break;
    }
  }
return res;
}

/*****************************************************************************/

int E_histoarray::e_delete(int argc, char* argv[])
{
// <ARR> delete
if (argc>2)
  {
  Tcl_SetResult(interp, "no arguments after 'delete' expected", TCL_STATIC);
  return TCL_ERROR;
  }
return Tcl_DeleteCommand(interp, tclprocname());
}

/*****************************************************************************/

int E_histoarray::checkidx(int idx)
{
if ((idx<0) || (arr==0)) return 0;
int d=idx_e-idx_a-1;
if (d<0) d+=arrsize;
return idx<=d;
}

/*****************************************************************************/

int E_histoarray::size()
{
if (arr==0) return 0;
int d=idx_e-idx_a;
if (d<=0) d+=arrsize;
return d;
}

/*****************************************************************************/

int E_histoarray::e_size(int argc, char* argv[])
{
// <ARR> size
if (argc>2)
  {
  Tcl_SetResult(interp, "no arguments after 'size' expected", TCL_STATIC);
  return TCL_ERROR;
  }
OSSTREAM st(interp->result, TCL_RESULT_SIZE);
st << size();
return TCL_OK;
}

/*****************************************************************************/

int E_histoarray::e_add(int argc, char* argv[])
{
// <ARR> add timestamp value
if (argc!=4)
  {
  Tcl_SetResult(interp, "wrong # args; must be add time value", TCL_STATIC);
  return TCL_ERROR;
  }
double time;
double value;
if ((Tcl_GetDouble(interp, argv[2], &time)!=TCL_OK) ||
    (Tcl_GetDouble(interp, argv[3], &value)!=TCL_OK))
  return TCL_ERROR;
add(time, value);
return TCL_OK;
}

/*****************************************************************************/

void E_histoarray::add(double time, double value)
{
//cerr << "E_histoarray::add(" << time << ", " << value << ")" << endl;
//print(cerr);
if (arr==0) // Array existiert nicht
	{
//	cerr << "erzeuge array" << endl;
 	arrsize=1<<4;
	arrmask=arrsize-1;
	if ((arr=new pair[arrsize])==0)
		{
		cerr << "E_histoarray::add: no memory to create array" << endl;
		return;
		}
	idx_a=0;
	idx_e=1;
	arr[0].time=time;
	arr[0].val=value;
	}
else if (idx_a==idx_e) // Array ist voll
	{
//	cerr << "array ist voll";
	if (arrsize<(1<<arrlimit)) // Array kann vergroessert werden
	  {
//	  cerr << ", kann aber wachsen" << endl;
//print(cerr);
    int newsize=2*arrsize;
    int newmask=newsize-1;
    pair* help;
    if ((help=new pair[newsize])==0)
      {
       cerr << "E_histoarray::add: kein Speicher, um Array zu vergroessern"
          << endl;
	    arr[idx_e].time=time;
	    arr[idx_e].val=value;
	    idx_e=(idx_e+1)&arrmask;
	    idx_a=idx_e;
      }
    else
      {
      int i, l;
      i=idx_e-1;
      if (i<idx_a) i+=arrsize;
//       cerr << "idx_a=" << idx_a << "; idx_e=" << idx_e << endl;
//       cerr << "i=" << i << "; l=" << l << endl;
       for (l=newsize-1; i>=idx_a; l--, i--)
         {
//         cerr << "copy arr["<<(i&arrmask)<<"] --> help["<<l<<"]" << endl;
         help[l]=arr[i & arrmask];
         }
      arrmask=newmask;
      arrsize=newsize;
      idx_a=l+1;
      idx_e=1;
      delete[] arr;
      arr=help;
      arr[0].time=time;
	    arr[0].val=value;
// print(cerr);
      }
	  }
	else // Array kann nicht vergroessert werden
	  {
// 	  cerr << " und kann nicht wachsen" << endl;
	  arr[idx_e].time=time;
	  arr[idx_e].val=value;
	  idx_e=(idx_e+1)&arrmask;
	  idx_a=idx_e;
	  }
	}
else // Es ist noch Platz
  {
//  cerr << "da war noch platz" << endl;
	arr[idx_e].time=time;
	arr[idx_e].val=value;
	idx_e=(idx_e+1)&arrmask;
  }
int i;
for (i=0; i<numrangecallbacks; i++)
  {
  //cerr << "call " << (void*)rangecallbacks[i].proc << "(" <<
  //    (void*)rangecallbacks[i].data << ")" << endl; 
    rangecallbacks[i].proc(this, rangecallbacks[i].data);
  }
for (i=0; i<numdatacallbacks; i++)
    datacallbacks[i].proc(this, datacallbacks[i].data);
}

/*****************************************************************************/

int E_histoarray::e_limit(int argc, char* argv[])
{
// <ARR> limit number [unit]
int res=TCL_OK, unit=0, num;
if (argc>4)
  {
  Tcl_SetResult(interp, "wrong # of arguments; must be limit number [unit]",
      TCL_STATIC);
  return TCL_ERROR;
  }
if (argc==2)
  {
  OSSTREAM st;
  st << (1<<arrlimit);
  Tcl_SetResult(interp, st.str(), ch_del);
  return TCL_OK;
  }
if (argc==4)
  {
  switch (argv[3][0])
    {
    case 'b':
    case 'B': unit=1; break;
    case 'k':
    case 'K': unit=1024; break;
    case 'm':
    case 'M': unit=1048576; break;
    default:
      Tcl_SetResult(interp, "unit must be one of b, B, k, K, m, M",
        TCL_STATIC);
      return TCL_ERROR;
      break;
    }
  }
if (Tcl_GetInt(interp, argv[2], &num)!=TCL_OK) return TCL_ERROR;
int newlimit;
if (unit)
  newlimit=(unit*num)/sizeof(pair);
else
  newlimit=num;
limit(newlimit);
return TCL_OK;
}


/*****************************************************************************/

void E_histoarray::limit(int limit)
{
arrlimit=2;
while ((1<<arrlimit)<limit) arrlimit++;
// cerr << "E_histoarray::limit: new limit: " << arrlimit << endl;
// print(cerr);
if (arrsize>(1<<arrlimit))
  {
  int newsize=1<<arrlimit;
  int newmask=newsize-1;
  pair* help;
  if ((help=new pair[newsize])==0)
    {
    cerr << "E_histoarray::limit: kein Speicher, um Array zu verkleinern"
        << endl;
    return;
    }
  int i, l;
  i=idx_e-1;
  if (idx_e<=idx_a) i+=arrsize;
  for (l=newsize-1; (l>=0) && (i>=idx_a); l--, i--)
    help[l]=arr[i & arrmask];
  arrmask=newmask;
  arrsize=newsize;
  idx_a=l+1;
  idx_e=0;
  delete[] arr;
  arr=help;
//   print(cerr);
  for (i=0; i<numrangecallbacks; i++)
      rangecallbacks[i].proc(this, rangecallbacks[i].data);
  }
}

/*****************************************************************************/

int E_histoarray::e_getval(int argc, char* argv[])
{
// <ARR> getval index [format]
if ((argc<3) || (argc>4))
  {
  Tcl_SetResult(interp, "wrong # arguments; must be getval index [format]",
      TCL_STATIC);
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[2], &idx)!=TCL_OK)  return TCL_ERROR;
if (!checkidx(idx))
  {
  Tcl_SetResult(interp, "wrong index", TCL_STATIC);
  return TCL_ERROR;
  }
int form=0; // float
if (argc>3)
  {
  // %d %f
  if (strcmp(argv[3], "%d")==0)
    {
    form=1; // integer
    }
  else if (strcmp(argv[3], "%f")==0)
    {
    form=0; // float
    }
  else
    {
    Tcl_SetResult(interp, "wrong format; must be %d or %f", TCL_STATIC);
    return TCL_ERROR;
    }
  }

OSSTREAM st(interp->result, TCL_RESULT_SIZE);
switch (form)
  {
  case 0:
    st << setprecision(20) << val(idx);
    break;
  case 1:
    st << (int)rint(val(idx));
    break;
  }
return TCL_OK;
}

/*****************************************************************************/

int E_histoarray::e_gettime(int argc, char* argv[])
{
// <ARR> gettime index [format]
if ((argc<3) || (argc>4))
  {
  Tcl_SetResult(interp, "wrong # arguments; must be gettime index [format]",
      TCL_STATIC);
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[2], &idx)!=TCL_OK)  return TCL_ERROR;
if (!checkidx(idx))
  {
  Tcl_SetResult(interp, "wrong index", TCL_STATIC);
  return TCL_ERROR;
  }
int form=0; // float
if (argc>3)
  {
  // %d %f
  if (strcmp(argv[3], "%d")==0)
    form=1; // integer
  else if (strcmp(argv[3], "%f")==0)
    form=0; // float
  else
    {
    Tcl_SetResult(interp, "wrong format; must be %d or %f", TCL_STATIC);
    return TCL_ERROR;
    }
  }
OSSTREAM st(interp->result, TCL_RESULT_SIZE);
switch (form)
  {
  case 0:
    st << setprecision(20) << time(idx);
    break;
  case 1:
    st << (int)rint(time(idx));
    break;
  }
return TCL_OK;
}

/*****************************************************************************/

int E_histoarray::e_get(int argc, char* argv[])
{
// <ARR> get index
if ((argc<3) || (argc>5))
  {
  Tcl_SetResult(interp,
      "wrong # arguments; must be get index [[format] format]" , TCL_STATIC);
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[2], &idx)!=TCL_OK)  return TCL_ERROR;
if (!checkidx(idx))
  {
  Tcl_SetResult(interp, "wrong index", TCL_STATIC);
  return TCL_ERROR;
  }

int form1, form2;
if (argc>3)
  {
  // %d %f
  if (strcmp(argv[3], "%d")==0)
    form1=1; // integer
  else if (strcmp(argv[3], "%f")==0)
    form1=0; // float
  else
    {
    Tcl_SetResult(interp, "wrong format; must be %d or %f", TCL_STATIC);
    return TCL_ERROR;
    }
  if (argc>4)
    {
    // %d %f
    if (strcmp(argv[4], "%d")==0)
      form2=1; // integer
    else if (strcmp(argv[4], "%f")==0)
      form2=0; // float
    else
      {
      Tcl_SetResult(interp, "wrong format; must be %d or %f", TCL_STATIC);
      return TCL_ERROR;
      }
    }
  else
    form2=form1;
  }
else
  form1=form2=0; // float
  
OSSTREAM st(interp->result, TCL_RESULT_SIZE);
switch (form1)
  {
  case 0:
    st << setprecision(20) << time(idx) << ' ';
    break;
  case 1:
    st << (int)rint(time(idx)) << ' ';
    break;
  }
switch (form2)
  {
  case 0:
    st << setprecision(20) << val(idx);
    break;
  case 1:
    st << (int)rint(val(idx));
    break;
  }
return TCL_OK;
}

/*****************************************************************************/

int E_histoarray::leftidx(double time, double st, int exact)
{
int result=-1;
if (arr==0)
  {
  cerr << "E_histoarray::leftidx: array empty" << endl;
  return 0;
  }
int ii=size()-1;
if (exact)
  {
  while ((ii>0) && (arr[(ii+idx_a) & arrmask].time>time)) ii--;
  }
else
  {
  while ((ii>0) && (arr[(ii+idx_a) & arrmask].time>=time)) ii--;
  }
return ii;
}

/*****************************************************************************/

int E_histoarray::rightidx(double time, double st, int exact)
{
int ii=0;
if (exact)
  while ((ii<size()-1) && (arr[(ii+idx_a) & arrmask].time<time)) ii++;
else
  while ((ii<size()-1) && (arr[(ii+idx_a) & arrmask].time<=time)) ii++;
return ii;
}

/*****************************************************************************/

int E_histoarray::getmaxxvals(double& x0, double& x1)
{
if (empty())
  {
  cerr << "E_histoarray::getmaxvals("<<name()<<") empty" << endl;
  return -1;
  }

x0=arr[idx_a].time;
x1=idx_e?arr[idx_e-1].time:arr[arrsize-1].time;
return 0;
}

/*****************************************************************************/

double E_histoarray::maxtime()
{
if (empty())
  {
  cerr << "E_histoarray::maxtime(" << name() << ") empty" << endl;
  return 0;
  }
return idx_e?arr[idx_e-1].time:arr[arrsize-1].time;
}

/*****************************************************************************/

double E_histoarray::val(int idx)
{
return arr[(idx+idx_a) & arrmask].val;
}

/*****************************************************************************/

double E_histoarray::time(int idx)
{
return arr[(idx+idx_a) & arrmask].time;
}

/*****************************************************************************/

int E_histoarray::e_leftidx(int argc, char* argv[])
{
// <ARR> leftidx time
if (argc!=3)
  {
  Tcl_SetResult(interp, "wrong # arguments; must be leftidx time", TCL_STATIC);
  return TCL_ERROR;
  }
double time;
if (Tcl_GetDouble(interp, argv[2], &time)!=TCL_OK)  return TCL_ERROR;
OSSTREAM st;
st << leftidx(time, 0, 0);
Tcl_SetResult(interp, st.str(), ch_del);
return TCL_OK;
}

/*****************************************************************************/

int E_histoarray::e_rightidx(int argc, char* argv[])
{
// <ARR> rightidx time
if (argc!=3)
  {
  Tcl_SetResult(interp, "wrong # arguments; must be rightidx time", TCL_STATIC);
  return TCL_ERROR;
  }
double time;
if (Tcl_GetDouble(interp, argv[2], &time)!=TCL_OK)  return TCL_ERROR;
OSSTREAM st;
st << rightidx(time, 0, 0);
Tcl_SetResult(interp, st.str(), ch_del);
return TCL_OK;
}

/*****************************************************************************/

int E_histoarray::e_clear(int argc, char* argv[])
{
// <ARR> clear
if (argc>2)
  {
  Tcl_SetResult(interp, "no arguments after 'clear' expected", TCL_STATIC);
  return TCL_ERROR;
  }
clear();
return TCL_OK;
}

/*****************************************************************************/

void E_histoarray::clear()
{
delete[] arr;
arr=0;
arrsize=0;
for (int i=0; i<numrangecallbacks; i++)
    rangecallbacks[i].proc(this, rangecallbacks[i].data);
}

/*****************************************************************************/

int E_histoarray::e_name(int argc, char* argv[])
{
// <ARR> name [newname]
if (argc>3)
  {
  Tcl_SetResult(interp, "wrong # args; must be name [newname]", TCL_STATIC);
  return TCL_ERROR;
  }
Tcl_SetResult(interp, name(), TCL_VOLATILE);
if (argc==3) arrname=argv[2];
return TCL_OK;
}

/*****************************************************************************/

int E_histoarray::e_print(int argc, char* argv[])
{
// <ARR> print
if (argc>2)
  {
  Tcl_SetResult(interp, "no arguments after 'print' expected", TCL_STATIC);
  return TCL_ERROR;
  }
if (!arr)
  {
  Tcl_SetResult(interp, "empty", TCL_STATIC);
  return TCL_OK;
  }
OSSTREAM st;
print(st);
Tcl_SetResult(interp, st.str(), ch_del);
return TCL_OK;
}

/*****************************************************************************/

void E_histoarray::print(ostream& st)
{
if (arr) st << idx_a << ' ' << idx_e << ' ' << arrsize  << ' ' << arrmask << endl;
if (!arr) return;
for (int x=0; x<arrsize; x++)
  {
  st << '[' << x << "] " << arr[x].time << ' ' << arr[x].val;
  if (x==idx_a) st << " <== idx_a";
  if (x==idx_e) st << " <== idx_e";
  st << endl;
  }
int i=idx_a;
int e=idx_e;
if (e<=i) e+=arrsize;
for (; i<e; i++)
    st << '{'<<arr[i&arrmask].time << ' '<<arr[i&arrmask].val<<"} "<<endl;
}

/*****************************************************************************/

int E_histoarray::e_dump(int argc, char* argv[])
{
// <ARR> dump filename [startidx [endidx]]
if ((argc<3) || (argc>5))
  {
  Tcl_SetResult(interp,
      "wrong # args; must be dump filename [startidx [endidx]]",TCL_STATIC);
  return TCL_OK; 
  }
ofstream os(argv[2]);
if (!os)
  {
  OSSTREAM st;
  st << "open \"" << argv[2] << "\" ging schief: errno=" << errno;
  Tcl_SetResult(interp, st.str(), ch_del);
  return TCL_ERROR;
  }

int aidx, eidx;
if (argc>3)
  {
  if (Tcl_GetInt(interp, argv[3], &aidx)!=TCL_OK)  return TCL_ERROR;
  if (!checkidx(aidx))
    {
    Tcl_AppendResult(interp, "wrong index " , argv[2], 0);
    return TCL_ERROR;
    }
  if (argc>4)
    {
    if (Tcl_GetInt(interp, argv[4], &eidx)!=TCL_OK)  return TCL_ERROR;
    if (!checkidx(eidx))
      {
      Tcl_AppendResult(interp, "wrong index " , argv[3], 0);
      return TCL_ERROR;
      }
    }
  else
    eidx=size()-1;
  }
else
  {
  aidx=0;
  eidx=size()-1;
  }
os << setprecision(20);
for (int i=aidx; i<eidx; i++)
  {
  os << time(i) << '\t' << val(i) << endl;
  }
return TCL_OK;
}

/*****************************************************************************/

int E_histoarray::e_restore(int argc, char* argv[])
{
// <ARR> restore filename
if (argc!=3)
  {
  Tcl_SetResult(interp,
      "wrong # args; must be restore filename",TCL_STATIC);
  return TCL_OK; 
  }

ifstream is(argv[2]);
if (!is)
  {
  OSSTREAM st;
  st << "open \"" << argv[2] << "\" ging schief: errno=" << errno;
  Tcl_SetResult(interp, st.str(), ch_del);
  return TCL_ERROR;
  }
clear();

String a, b;
int n=0;
double last=0;
while (is >> a)
  {
  is >> b;
  int fehler=0;
  double va, vb;
  if (Tcl_GetDouble(interp, a, &va)!=TCL_OK)
    {
    return TCL_ERROR;
    }
  if (Tcl_GetDouble(interp, b, &vb)!=TCL_OK)
    {
    return TCL_ERROR;
    }
//   if (fabs(va-last)>10)
//     cerr << "last=" << setprecision(20) << last << "; now=" << va << endl;
//   else
    add(va, vb); 
  n++;
  last=va;
  }

  {
  OSSTREAM st(interp->result, TCL_RESULT_SIZE);
  st << n;
  }
for (int i=0; i<numrangecallbacks; i++)
    rangecallbacks[i].proc(this, rangecallbacks[i].data);
return TCL_OK;
}

/*****************************************************************************/

void E_histoarray::installdatacallback(void (*proc)(E_histoarray*, void*),
    ClientData data)
{
callback* help=new callback[numdatacallbacks+1];
for (int i=0; i<numdatacallbacks; i++) help[i]=datacallbacks[i];
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
for (int i=0; i<numrangecallbacks; i++) help[i]=rangecallbacks[i];
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
for (int i=0; i<numdeletecallbacks; i++) help[i]=deletecallbacks[i];
delete[] deletecallbacks;
deletecallbacks=help;
help[numdeletecallbacks].proc=proc;
help[numdeletecallbacks].data=data;
numdeletecallbacks++;
}

/*****************************************************************************/

void E_histoarray::deletedatacallback(ClientData data)
{
for (int i=0; i<numdatacallbacks; i++)
  {
  if (datacallbacks[i].data==data)
    {
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
for (int i=0; i<numrangecallbacks; i++)
  {
  if (rangecallbacks[i].data==data)
    {
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
for (int i=0; i<numdeletecallbacks; i++)
  {
  if (deletecallbacks[i].data==data)
    {
    for (int j=i; j<numdeletecallbacks; j++)
        deletecallbacks[i]=deletecallbacks[i+1];
    numdeletecallbacks--;
    return;
    }
  }
}

/*****************************************************************************/
/*****************************************************************************/
