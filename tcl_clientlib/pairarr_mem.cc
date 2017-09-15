/*
 * pairarr_mem.cc
 * 
 * created 13.09.96 PW
 */

#include <pairarr_mem.hxx>
#include "versions.hxx"

VERSION("Nov 16 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: pairarr_mem.cc,v 1.5 2004/11/26 23:03:54 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_pairarr_mem::C_pairarr_mem(const char* name)
:arr(0), arrlimit(16)
{
cerr << "C_pairarr_mem::C_pairarr_mem(" << name << ")" << endl;
if (name==0)
  filename="";
else
  filename=name;
valid_=1;  
}

/*****************************************************************************/

C_pairarr_mem::~C_pairarr_mem()
{
delete[] arr;
}

/*****************************************************************************/

int C_pairarr_mem::checkidx(int idx)
{
if ((idx<0) || (arr==0)) return 0;
int d=idx_e-idx_a-1;
if (d<0) d+=arrsize;
return idx<=d;
}

/*****************************************************************************/

void C_pairarr_mem::add(double time, double value)
{
if (arr==0) // Array existiert nicht
  {
  arrsize=1<<4;
  arrmask=arrsize-1;
  if ((arr=new vt_pair[arrsize])==0)
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
  if (arrsize<(1<<arrlimit)) // Array kann vergroessert werden
    {
    int newsize=2*arrsize;
    int newmask=newsize-1;
    vt_pair* help;
    if ((help=new vt_pair[newsize])==0)
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
      for (l=newsize-1; i>=idx_a; l--, i--)
        {
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
      }
    }
  else // Array kann nicht vergroessert werden
    {
    arr[idx_e].time=time;
    arr[idx_e].val=value;
    idx_e=(idx_e+1)&arrmask;
    idx_a=idx_e;
    }
  }
else // Es ist noch Platz
  {
  arr[idx_e].time=time;
  arr[idx_e].val=value;
  idx_e=(idx_e+1)&arrmask;
  }
}

/*****************************************************************************/

int C_pairarr_mem::limit(void) const
{
return 1<<arrlimit;
}

/*****************************************************************************/

void C_pairarr_mem::forcelimit(void)
{
}

/*****************************************************************************/

void C_pairarr_mem::limit(int limit)
{
arrlimit=2;
while ((1<<arrlimit)<limit) arrlimit++;
if (arrsize>(1<<arrlimit))
  {
  int newsize=1<<arrlimit;
  int newmask=newsize-1;
  vt_pair* help;
  if ((help=new vt_pair[newsize])==0)
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
  }
}

/*****************************************************************************/

// int E_histoarray::checkidx(int idx)
// {
// if ((idx<0) || (arr==0)) return 0;
// int d=idx_e-idx_a-1;
// if (d<0) d+=arrsize;
// return idx<=d;
// }

/*****************************************************************************/

int C_pairarr_mem::size()
{
if (arr==0) return 0;
int d=idx_e-idx_a;
if (d<=0) d+=arrsize;
return d;
}

/*****************************************************************************/

int C_pairarr_mem::leftidx(double time, int exact)
{
if (arr==0) return 0;

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

int C_pairarr_mem::rightidx(double time, int exact)
{
if (arr==0) return 0;

int ii=0;
if (exact)
  while ((ii<size()-1) && (arr[(ii+idx_a) & arrmask].time<time)) ii++;
else
  while ((ii<size()-1) && (arr[(ii+idx_a) & arrmask].time<=time)) ii++;
return ii;
}

/*****************************************************************************/

int C_pairarr_mem::getmaxxvals(double& x0, double& x1)
{
if (arr==0) return -1;

x0=arr[idx_a].time;
x1=idx_e?arr[idx_e-1].time:arr[arrsize-1].time;
return 0;
}

/*****************************************************************************/

double C_pairarr_mem::maxtime()
{
if (arr==0) return 0;
return idx_e?arr[idx_e-1].time:arr[arrsize-1].time;
}

/*****************************************************************************/

double C_pairarr_mem::val(int idx)
{
return arr[(idx+idx_a) & arrmask].val;
}

/*****************************************************************************/

double C_pairarr_mem::time(int idx)
{
return arr[(idx+idx_a) & arrmask].time;
}

/*****************************************************************************/

void C_pairarr_mem::clear()
{
delete[] arr;
arr=0;
}

/*****************************************************************************/

const vt_pair* C_pairarr_mem::operator [](int idx)
{
return &arr[(idx+idx_a) & arrmask];
}

/*****************************************************************************/

void C_pairarr_mem::print(ostream& st) const
{
if (arr)
  st << idx_a << ' ' << idx_e << ' ' << arrsize  << ' ' << arrmask << endl;
else
  {
  st << "arr ist leer" << endl;
  return;
  }

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
/*****************************************************************************/
