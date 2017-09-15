/*
 * pairarr_file.hxx
 *
 * $ZEL: pairarr_file.hxx,v 1.5 2004/11/18 12:35:55 wuestner Exp $
 *
 * created 14.09.96 PW
 */

#ifndef _pairarr_file_hxx_
#define _pairarr_file_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <sys/param.h>
#include <sys/types.h>
#ifdef __osf__
#include <sys/mount.h>
#endif
#include <sys/time.h>
#include "pairarr.hxx"

/*****************************************************************************/

class C_pairarr_file: public C_pairarr {
  public:
    C_pairarr_file(const char* file);
    C_pairarr_file(int initialsize, int increment, int maxsize,
        const char* file);
    virtual ~C_pairarr_file();
    typedef struct {
        vt_pair pair;
        int idx;
    } cached_pair;
  protected:
    STRING filename;
    int path;
    static const int cache_size, cache_head;
    int cached_num;
    cached_pair* pair_cache;
    cached_pair *curr_pair, *first_pair, *last_pair;
    void print_cache();
    void addcache(vt_pair&, int);
#ifdef __osf__
    off_t getfsspace();
    static off_t fsspace;
#endif
    void testfilesize();
    void truncate(); // kuerzt den File auf minhistory Paare
#ifdef __osf__
    static struct statfs fsstat;  // file system statistics
    static struct timeval fstime; // Zeitpunkt der letzten Statistik
    static int fsstat_valid;
#endif
    int minhistory; // minimale Laenge der History (Datenpunkte)
    int arrsize; // aktuelle Groesse des Arrays in sizeof(vt_pair)
    vt_pair& readpair(int);
    double xtime(int);
    int gelesen;
  public:
    virtual int checkidx(int);
    virtual int size();
    virtual int notempty() const;
    virtual int empty() const;
    virtual void add(double, double);
    virtual void clear();
    virtual int limit() const;
    virtual void limit(int size);
    virtual const vt_pair* operator[](int);
    virtual int leftidx(double, int);
    virtual int rightidx(double, int);
    virtual double time(int);
    virtual double maxtime();
    virtual double val(int);
    virtual int getmaxxvals(double&, double&);
    virtual void flush(void);
    virtual void forcelimit(void);
    virtual void print(ostream&) const;
};

#endif

/*****************************************************************************/
/*****************************************************************************/
