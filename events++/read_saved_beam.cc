/*
 * ems/events++/read_saved_beam.cc
 * 
 * created 2015-Nov-25 PeWue
 * $ZEL: read_saved_beam.cc,v 1.4 2016/02/04 12:40:08 wuestner Exp $
 */

/*
 * read_saved_beam.cc can provide the beam data (saved by save_beam) for a
 * given point in time. Most ems events have a timestamp which is used here
 * to automatically select and open the appropriate beam data file and read
 * the requested data.
 */

#define _DEFAULT_SOURCE

#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include "read_saved_beam.hxx"

#define FILESTR "/%Y/%m/%d/%H"
#define MAXPATHLEN 1024

#define U32(a) (static_cast<uint32_t>(a))
#define swap_int(a) ( (U32(a) << 24) | \
                      (U32((a) << 8) & 0x00ff0000) | \
                      (U32((a) >> 8) & 0x0000ff00) | \
                      (U32(a) >>24) )

static const char *dirname=0;

static int32_t *data=0;
static ssize_t numdata;
static int blocksize;
static int numblocks;
static int32_t *blockptr;
static int32_t *lastblock;

/*
 * the stucture of the saved beam data is (all words in network byte ordering):
 * 0 int32_t number of following words
 * 1 int32_t tv.tv_sec
 * 2 int32_t tv.tv_usec
 * 3 int32_t number of following data words
 * 4 int32_t data words ...
 * 5 ...
 */
//---------------------------------------------------------------------------//
/*
 * read_file reads beam data from file descriptor p.
 * It deletes old data, allocates space for the new data, read the
 * new data and initialises some global variables.
 */
static int
read_file(int p)
{
    off_t end;
    ssize_t res;

    if (data)
        free(data);
    data=0;
    numdata=0;

    end=lseek(p, 0, SEEK_END);
    if (end==static_cast<off_t>(-1)) {
        perror("beamdata:lseek:");
        return -1;
    }
    lseek(p, 0, SEEK_SET);

    data=reinterpret_cast<int32_t*>(malloc(4*end));
    res=read(p, data, end);
    if (res<0) {
        perror("beamdata:read:");
        free(data);
        data=0;
        return -1;
    }
    if (res!=end) {
        fprintf(stderr, "beamdata:read: res=%ld, end=%ld\n", res, end);
        free(data);
        data=0;
        return -1;
    }

    numdata=res/4;
    for (int i=0; i<numdata; i++)
        data[i]=swap_int(data[i]);

    blocksize=data[0]+1;
    numblocks=numdata/blocksize;
    if (numdata%blocksize) {
        fprintf(stderr, "beamdata:read: uncomplete block at the end\n");
        numdata=numblocks*blocksize;
    }

    blockptr=data;
    lastblock=data+(numblocks-1)*blocksize;

#if 0
    for (int i=0; i<32; i++) {
        fprintf(stderr, "%2d %08x\n", i, data[i]);
    }
#endif

    return 0;
}
//---------------------------------------------------------------------------//
/*
 * select_file constructs a filename from the given timeval and asks read_file
 * to read and store the data.
 */
static int
select_file(struct timeval *tv)
{
    char name[MAXPATHLEN];
    char *ts;
    size_t len;
    struct tm* tm;
    time_t tt;
    int p, res;

    /*
     * The standardised filename is:
     * dir/year/month/day/hour (dirname/2015/11/25/17)
     */

    strncpy(name, dirname, MAXPATHLEN);
    if (name[MAXPATHLEN-1]) { // no trailing zeros
        fprintf(stderr, "beamdata:select_file: dirname too long\n");
        return -1;
    }
    len=strlen(name);
    ts=name+len;
    if (MAXPATHLEN-len<15) {
        fprintf(stderr, "beamdata:select_file: dirname too long "
                "(or MAXPATHLEN too short :-)\n");
        return -1;
    }

    tt=tv->tv_sec;
    tm=gmtime(&tt);
    strftime(ts, MAXPATHLEN-len, FILESTR, tm);

    fprintf(stderr, "beamdata:select_file: \"%s\"\n", name);

    p=open(name, O_RDONLY);
    if (p<0) {
        perror("beamdata:select_file:open:");
        return -1;
    }
    res=read_file(p);
    close(p);

    return res;
}
//---------------------------------------------------------------------------//
/*
 * find_beam_data searches for the requested data block in the stored data.
 * If there are no data yet or new data are needed it calls select_file to
 * read new data.
 * We can savely assume that the data are requested in chronological order,
 * thus we don't need to search backwards.
 */
static int32_t*
find_beam_data(struct timeval *tv)
{
    if (!data) {
        if (select_file(tv)<0)
            return 0;
    }

    while (blockptr[1]<tv->tv_sec && blockptr<=lastblock)
        blockptr+=blocksize;
    if (blockptr[1]<tv->tv_sec) {
        if (select_file(tv)<0)
            return 0;
    }

    while (blockptr[1]<tv->tv_sec && blockptr<=lastblock)
        blockptr+=blocksize;
    if (blockptr[1]<tv->tv_sec) {
        fprintf(stderr, "beamdata:find: new file, but still no valid data\n");
            return 0;
    }

    if (blockptr[1]>tv->tv_sec+1) {
        fprintf(stderr, "beamdata:find: requested %ld, found %d\n",
                tv->tv_sec, blockptr[1]);
    }

    return blockptr;
}
//---------------------------------------------------------------------------//
/*
 * convert converts the data from the ET200s raw integer format to float.
 */
static float
convert(int32_t val)
{
    float v;

    v=static_cast<float>(val)/27648.*10.;
    return v;
}
//---------------------------------------------------------------------------//
/*
 * get_beamdata asks find_beam_data to find the data block coresponding to
 * the given timeval.
 * If idx is >= 0 then the value with index idx is returned in *val.
 * If idx is <0 then -idx is the number of values tu be returned in the array
 * val, beginning with index 0.
 */
int
get_beamdata(struct timeval *tv, int idx, float *val)
{
    int32_t *bd;

    bd=find_beam_data(tv);
    if (!bd)
        return -1;

    // sanity check
    if (bd[3]+3!=bd[0]) {
        fprintf(stderr, "get_beamdata::inconsistent data size: %d / %d+3\n",
                bd[0], bd[3]);
        return -1;
    }

    if (idx>=0) {
        if (idx>=bd[3]) {
            fprintf(stderr, "get_beamdata:requested index too large\n");
            return -1;
        }
        *val=convert(bd[idx+4]);
    } else {
        idx=-idx; // -idx is used as number of values
        if (idx>bd[3]) {
            fprintf(stderr, "get_beamdata:requested number too large\n");
            return -1;
        }
        for (int i=0; i<idx; i++) {
            *val++=convert(bd[i+4]);
        }
    }

    return 0;
}
//---------------------------------------------------------------------------//
/*
 * init_beamdata must be called before get_beamdata. It just stores the
 * name of the directory containing the beam data files.
 * If directory is the NULL pointer only the data are freed.
 */
int
init_beamdata(const char *directory)
{
    dirname=directory;
    if (data)
        free(data);
    data=0;

    return 0;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
