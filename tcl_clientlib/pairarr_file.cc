/*
 * pairarr_file.cc
 * 
 * created 14.09.96 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <assert.h>
#include <unistd.h>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <tcl.h>
#include "pairarr_file.hxx"
#include "compat.h"
#include <errors.hxx>
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: pairarr_file.cc,v 1.19 2010/02/03 00:15:51 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
/*
#ifdef __osf__
extern "C" int fstatfs( int, struct statfs*, int);
#endif
*/
/*****************************************************************************/

const int C_pairarr_file::cache_size=13;
const int C_pairarr_file::cache_head=3;

#ifdef __osf__
struct statfs C_pairarr_file::fsstat;
struct timeval C_pairarr_file::fstime={0, 0};
off_t C_pairarr_file::fsspace=0;
int C_pairarr_file::fsstat_valid=0;
#endif

/*****************************************************************************/
C_pairarr_file::C_pairarr_file(const char* name)
:cached_num(0), minhistory(0), gelesen(-1)
{
    pair_cache=new cached_pair[cache_size];
    curr_pair=pair_cache+0;
    first_pair=pair_cache+1;
    last_pair=pair_cache+2;

#if 1
    assert(name!=0);
#else
    if (name==0) {
        OSTRINGSTREAM st;
        st << "Filename must be specified.";
        errormessage=st.str();
        return;
    }
#endif

    filename=name;
    path=open(filename.c_str(), O_RDWR|O_CREAT, 0640);
    if (path<0) {
        OSTRINGSTREAM st;
        st << "open " << filename << " " << strerror(errno);
        errormessage=st.str();
        return;
    }
#ifdef __osf__
    if (lockf(path, F_TLOCK, 0)<0) {
        OSTRINGSTREAM st;
        st << "lock " << filename << " " << strerror(errno);
        errormessage=st.str();
        close(path);
        return;
    }
#endif

    struct stat status;
    if (fstat(path, &status)<0) {
        OSTRINGSTREAM st;
        st << "fstat " << filename << " " << strerror(errno);
        errormessage=st.str();
        close(path);
        return;
    }

    size_t size=status.st_size;
    if (size>0) {
        //cerr << "filesize: " << size << endl;
        if (size%sizeof(vt_pair)!=0) {
            cerr << "Size of file " << filename << ": " << size << endl;
            size=(size/sizeof(vt_pair))*sizeof(vt_pair);
            cerr << "truncating to " << size << endl;
            if (ftruncate(path, size/(sizeof(vt_pair)))<0) {
                OSTRINGSTREAM st;
                st << "ftruncate " << filename << " " << strerror(errno);
                errormessage=st.str();
                close(path);
                return;
            }
        }

        if (lseek(path, 0, SEEK_SET)<0) {
            OSTRINGSTREAM st;
            st << "lseek to start of " << filename << " " << strerror(errno);
            errormessage=st.str();
            close(path);
            return;
        }

        if (read(path, (char*)&first_pair->pair, sizeof(vt_pair))
                !=sizeof(vt_pair)) {
            OSTRINGSTREAM st;
            st << "read first pair of " << filename << " " << strerror(errno);
            errormessage=st.str();
            close(path);
            return;
        }
        first_pair->idx=0;

        if (lseek(path, -sizeof(vt_pair), SEEK_END)<0) {
            OSTRINGSTREAM st;
            st << "lseek to end of " << filename << " " << strerror(errno);
            errormessage=st.str();
            close(path);
            return;
        }
        if (read(path, (char*)&last_pair->pair, sizeof(vt_pair))
                !=sizeof(vt_pair)) {
            OSTRINGSTREAM st;
            st << "read last pair of " << filename << " " << strerror(errno);
            errormessage=st.str();
            close(path);
            return;
        }
        last_pair->idx=size/sizeof(vt_pair)-1;
        *curr_pair=*last_pair;
        cached_num=3;
        //cerr << "firstidx=" << first_pair->idx << "; lastidx=" << last_pair->idx << endl;
        arrsize=size/sizeof(vt_pair);
    } else {
        last_pair->idx=-1;
        cached_num=0;
        arrsize=0;
    }
    //numarrays++;
    valid_=1;
}
/*****************************************************************************/
C_pairarr_file::~C_pairarr_file()
{
    //numarrays--;
    //cerr << "--numarrays=" << numarrays << endl;
    if (path>=0) close(path);
    delete[] pair_cache;
}
/*****************************************************************************/
void C_pairarr_file::flush(void)
{
#if defined (__osf__)
    fdatasync(path);
#else
    fsync(path);
#endif
}
/*****************************************************************************/
#ifdef __osf__
off_t C_pairarr_file::getfsspace(void)
{
struct timeval jetzt;
gettimeofday(&jetzt, 0);
if (fsstat_valid && (fsspace>1048576) && ((jetzt.tv_sec-fstime.tv_sec)<60))
    return fsspace;

fstime=jetzt;

if (fstatfs(path, &fsstat, sizeof(struct statfs))<0)
  {
  cerr << "fstatfs: " << strerror(errno) << endl;
  return -1;
  }
// if (first)
//   {
//   cerr << "f_type  : " << fsstat.f_type << endl;
//   cerr << "f_flags : " << fsstat.f_flags << endl;
//   cerr << "f_fsize : " << fsstat.f_fsize << endl;
//   cerr << "f_bsize : " << fsstat.f_bsize << endl;
//   cerr << "f_blocks: " << fsstat.f_blocks << endl;
//   cerr << "f_bfree : " << fsstat.f_bfree << endl;
//   cerr << "f_bavail: " << fsstat.f_bavail << endl;
//   cerr << "f_files : " << fsstat.f_files << endl;
//   cerr << "f_ffree : " << fsstat.f_ffree << endl;
//   //cerr << "f_fsid  : " << fsstat.f_fsid << endl;
//   cerr << "f_mntonname  : " << fsstat.f_mntonname << endl;
//   cerr << "f_mntfromname: " << fsstat.f_mntfromname << endl;
//   }

fsspace=fsstat.f_bavail*fsstat.f_fsize;
fsstat_valid=1;
//cerr << "free: " << fsspace << endl;
return fsspace;
}
#endif

/*****************************************************************************/

void C_pairarr_file::testfilesize()
{
off_t asize;
#ifdef __osf__
off_t space;
space=getfsspace();
#endif
asize=arrsize*sizeof(vt_pair);

#ifdef __osf__
if ((arrsize>10*minhistory) || (space<100*asize)) truncate();
#else
if (arrsize>10*minhistory) truncate();
#endif
return;
}

/*****************************************************************************/
void C_pairarr_file::truncate()
{
    if ((minhistory<=0) || (minhistory*2>arrsize))
        return;

    STRING newname=filename+"_new";

    int lostpairs=last_pair->idx+1-minhistory;

    int newpath=open(newname.c_str(), O_RDWR|O_CREAT|O_EXCL, 0640);
    if (newpath<0) {
        cerr << "create new file "<<newname << ": "<< strerror(errno) << endl;
        return;
    }

    if (lseek(path, lostpairs*sizeof(vt_pair), SEEK_SET)<0) {
        cerr << "lseek: " << strerror(errno) << endl;
    }

    vt_pair* buf;
    int bufsize=minhistory*2;
    do {
        bufsize/=2;
        buf=new vt_pair[bufsize];
    }
    while ((buf==0) && (bufsize>10));

    if (buf) {
        int rest=minhistory;
        while (rest) {
            ssize_t res, num;
            num=rest<bufsize?rest:bufsize;
            res=read(path, (char*)buf, sizeof(vt_pair)*num);
            if (res!=(ssize_t)sizeof(vt_pair)*num) {
                cerr << "read original file; num=" << num << "; res=" << res
                    << "; " << strerror(errno) << endl;
                delete[] buf;
                unlink(newname.c_str());
                return;
            }
            res=write(newpath, (char*)buf, sizeof(vt_pair)*num);
            if (res!=(ssize_t)sizeof(vt_pair)*num) {
                cerr << "write copy of file: " << strerror(errno) << endl;
                delete[] buf;
                unlink(newname.c_str());
                return;
            }
            rest-=num;
        }
        delete[] buf;
    } else {
        vt_pair pair;
        for (int n=0; n<minhistory; n++) {
            if (read(path, (char*)&pair, sizeof(vt_pair))!=sizeof(vt_pair)) {
                cerr<<"truncate: read failed"<<endl;
                return;
            }
            if (write(newpath, (char*)&pair, sizeof(vt_pair))!=sizeof(vt_pair)) {
                cerr<<"truncate: write failed"<<endl;
                return;
            }
        }
    }

    close(path);
    path=newpath;
    //unlink(filename); // macht rename
    if (rename(newname.c_str(), filename.c_str())<0) {
        cerr << "rename: " << strerror(errno) << endl;
    }
    arrsize=minhistory;

    lseek(path, 0, SEEK_SET);
    if (read(path, (char*)&first_pair->pair, sizeof(vt_pair))!=
            sizeof(vt_pair)) {
        cerr<<"truncate: read failed"<<endl;
        return;
    }
    first_pair->idx=0;

    lseek(path, -sizeof(vt_pair), SEEK_END);
    if (read(path, (char*)&last_pair->pair, sizeof(vt_pair))!=
            sizeof(vt_pair)) {
        cerr<<"truncate: read failed"<<endl;
        return;
    }
    last_pair->idx=arrsize-1;

    *curr_pair=*last_pair;
    cached_num=3;
    #ifdef __osf__
    fsstat_valid=0;
    #endif
}
/*****************************************************************************/

int C_pairarr_file::checkidx(int idx)
{
return (idx>0) && (idx<=last_pair->idx);
}

/*****************************************************************************/

int C_pairarr_file::size(void)
{
return last_pair->idx+1;
}

/*****************************************************************************/
void C_pairarr_file::add(double time, double val)
{
    last_pair->pair.time=time;
    last_pair->pair.val=val;
    lseek(path, 0, SEEK_END);
    if (write(path, (char*)&last_pair->pair, sizeof(vt_pair))!=sizeof(vt_pair)) {
        cerr<<"C_pairarr_file::add: write failed"<<endl;
        return;
    }
    last_pair->idx++;
    if (last_pair->idx==0) {
        *first_pair=*last_pair;
        *curr_pair=*last_pair;
        cached_num=3;
    }
    arrsize++;
    if (minhistory>0)
        testfilesize();
}
/*****************************************************************************/

int C_pairarr_file::limit(void) const
{
return minhistory;
}

/*****************************************************************************/

void C_pairarr_file::forcelimit(void)
{
if (minhistory==0) return;
truncate();
}

/*****************************************************************************/

void C_pairarr_file::limit(int size)
{
minhistory=size;
if (minhistory>0) testfilesize();
}

/*****************************************************************************/

void C_pairarr_file::print_cache()
{
cerr << "cache: num=" << cached_num << endl;
for (int i=0; i<cached_num; i++)
  cerr << '[' << i << "]: " << pair_cache[i].idx << ": "
      << pair_cache[i].pair.time-first_pair->pair.time << endl;
}

/*****************************************************************************/

void C_pairarr_file::addcache(vt_pair& pair, int idx)
{
for (int j=0; j<cached_num; j++)
  if (pair_cache[j].idx==idx) return;
//print_cache();
for (int i=cache_size-1; i>cache_head; i--) pair_cache[i]=pair_cache[i-1];
pair_cache[cache_head].pair=pair;
pair_cache[cache_head].idx=idx;
if (cached_num<cache_size) cached_num++;
//print_cache();
}

/*****************************************************************************/

int C_pairarr_file::rightidx(double zeit, int exact)
{
if (last_pair->idx==-1) return 0;

if (zeit>=last_pair->pair.time) return last_pair->idx;
if (exact)
  {
  if (zeit<=first_pair->pair.time) return first_pair->idx;
  }
else
  {
  if (zeit<first_pair->pair.time) return first_pair->idx;
  }

int cacheidx=0;
double diffzeit=fabs(first_pair->pair.time-zeit);
for (int i=1; i<cached_num; i++)
  {
  if (fabs(pair_cache[i].pair.time-zeit)<diffzeit)
    {
    diffzeit=fabs(pair_cache[i].pair.time-zeit);
    cacheidx=i;
    }
  }
int idx=pair_cache[cacheidx].idx;

if (diffzeit==0)
  {
  if (!exact && (idx<size()-1)) idx++;
  goto raus;
  //return idx;
  }
gelesen=0;
vt_pair pair, saved_pair;
if (pair_cache[cacheidx].pair.time<zeit)
  {
  if (exact)
    {
    do
      {
      idx++;
      pair=readpair(idx);
      }
    while ((idx<size()-1) && (pair.time<zeit));
    }
  else
    {
    do
      {
      idx++;
      pair=readpair(idx);
      }
   while ((idx<size()-1) && (pair.time<=zeit));
    }
  addcache(pair, idx);
  }
else
  {
  pair=pair_cache[cacheidx].pair;
  if (exact)
    {
    do
      {
      saved_pair=pair;
      idx--;
      pair=readpair(idx);
      }
    while ((idx>0) && (pair.time>=zeit));
    }
  else
    {
    do
      {
      saved_pair=pair;
      idx--;
      pair=readpair(idx);
      }
    while ((idx>0) && (pair.time>zeit));
    }
  idx++;
  addcache(saved_pair, idx);
  }

raus:
gelesen=-1;
return idx;
}

/*****************************************************************************/

int C_pairarr_file::leftidx(double zeit, int exact)
{
if (last_pair->idx==-1) return 0;

if (zeit<=first_pair->pair.time) return first_pair->idx;
if (exact)
  {
  if (zeit>=last_pair->pair.time) return last_pair->idx;
  }
else
  {
  if (zeit>last_pair->pair.time) return last_pair->idx;
  }

int cacheidx=0;
double diffzeit=fabs(first_pair->pair.time-zeit);
for (int i=1; i<cached_num; i++)
  {
  if (fabs(pair_cache[i].pair.time-zeit)<diffzeit)
    {
    diffzeit=fabs(pair_cache[i].pair.time-zeit);
    cacheidx=i;
    }
  }
int idx=pair_cache[cacheidx].idx;

if (diffzeit==0)
  {
  if (!exact && (idx>0)) idx--;
  return idx;
  }

gelesen=0;
vt_pair pair, saved_pair;
if (pair_cache[cacheidx].pair.time>zeit)
  {
  if (exact)
    {
    do
      {
      idx--;
      pair=readpair(idx);
      }
    while ((idx>0) && (pair.time>zeit));
    }
  else
    {
    do
      {
      idx--;
      pair=readpair(idx);
      }
   while ((idx>0) && (pair.time>=zeit));
    }
  addcache(pair, idx);
  }
else
  {
  pair=pair_cache[cacheidx].pair;
  if (exact)
    {
    do
      {
      saved_pair=pair;
      idx++;
      pair=readpair(idx);
      }
    while ((idx<size()-1) && (pair.time<=zeit));
    }
  else
    {
    do
      {
      saved_pair=pair;
      idx++;
      pair=readpair(idx);
      }
    while ((idx<size()-1) && (pair.time<zeit));
    }
  idx--;
  addcache(saved_pair, idx);
  }
  
gelesen=-1;
return idx;
}
/*****************************************************************************/
vt_pair& C_pairarr_file::readpair(int idx)
{
    static vt_pair pair;
    lseek(path, idx*sizeof(vt_pair), SEEK_SET);
    if (read(path, (char*)&pair, sizeof(vt_pair))!=sizeof(vt_pair)) {
        cerr<<"C_pairarr_file::readpair: read failed"<<endl;
    }
    gelesen++;
    return pair;
}
/*****************************************************************************/
double C_pairarr_file::val(int idx)
{
    for (int i=0; i<cached_num; i++) {
        if (pair_cache[i].idx==idx)
            return pair_cache[i].pair.val;
    }
    curr_pair->pair=readpair(idx);
    curr_pair->idx=idx;
    return curr_pair->pair.val;
}
/*****************************************************************************/
double C_pairarr_file::time(int idx)
{
    for (int i=0; i<cached_num; i++) {
        if (pair_cache[i].idx==idx)
            return pair_cache[i].pair.time;
    }
    curr_pair->pair=readpair(idx);
    curr_pair->idx=idx;
    return curr_pair->pair.time;
}
/*****************************************************************************/
double C_pairarr_file::xtime(int idx)
{
    for (int i=0; i<cached_num; i++) {
        if (pair_cache[i].idx==idx)
            return pair_cache[i].pair.time;
    }
    vt_pair pair;
    pair=readpair(idx);
    return pair.time;
}
/*****************************************************************************/
double C_pairarr_file::maxtime()
{
    return last_pair->pair.time;
}
/*****************************************************************************/
int C_pairarr_file::getmaxxvals(double& x0, double& x1)
{
    if (last_pair->idx==-1)
        return -1;
    x0=first_pair->pair.time;
    x1=last_pair->pair.time;
    return 0;
}
/*****************************************************************************/
int C_pairarr_file::empty(void) const
{
    return last_pair->idx==-1;
}
/*****************************************************************************/
int C_pairarr_file::notempty(void) const
{
    return last_pair->idx>-1;
}
/*****************************************************************************/
void C_pairarr_file::clear(void)
{
    if (ftruncate(path, 0)<0) {
        cerr<<"C_pairarr_file::clear: ftruncate failed"<<endl;
    }
    last_pair->idx=-1;
    cached_num=0;
}
/*****************************************************************************/
const vt_pair* C_pairarr_file::operator [](int idx)
{
    for (int i=0; i<cached_num; i++) {
        if (pair_cache[i].idx==idx)
            return &pair_cache[i].pair;
    }
    curr_pair->pair=readpair(idx);
    curr_pair->idx=idx;
    return &curr_pair->pair;
}
/*****************************************************************************/
void C_pairarr_file::print(ostream& st) const
{
    struct stat status;
    fstat(path, &status);

    st << "lastidx=" << last_pair->idx << "; filesize=" << status.st_size;
    if (last_pair->idx>-1) {
        vt_pair pair;
        lseek(path, 0, SEEK_SET);
        for (int i=0; i<=last_pair->idx; i++) {
            if (read(path, (char*)&pair, sizeof(vt_pair))!=sizeof(vt_pair)) {
                cerr<<"C_pairarr_file::print: read failed"<<endl;
                break;
            }
            st << '[' << i << "] " << pair.time << ' ' << pair.val;
        }
    }
}
/*****************************************************************************/
/*****************************************************************************/
