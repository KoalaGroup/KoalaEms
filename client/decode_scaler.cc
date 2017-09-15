/*
 * read_scaler.cc
 * 
 * created: 2006-Aug-20 PW
 */

#include "config.h"
#include <iostream>
#include <cmath>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "compressed_io.hxx"
#include <versions.hxx>

VERSION("2006-Aug-23", __FILE__, __DATE__, __TIME__,
"$ZEL: decode_scaler.cc,v 1.2 2007/04/10 00:01:05 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )


struct scaldata {
    ems_u64 timestamp;       /* usec */
    ems_u32 nr_modules;
    ems_u32 nr_channels[32]; /* no more than 32 modules...*/
    ems_u64 val[32][32];     /* val[module][channel] */
};

struct scaldata scal_a, scal_b;

//---------------------------------------------------------------------------//
int
xread(int p, void* vb, int num)
{
    int da=0, rest=num, res;
    char *b=static_cast<char*>(vb);
    while (rest) {
        res=read(p, b+da, rest);
        if (res==0)
            return 0;
        if (res<0) {
            if (errno!=EINTR) {
                printf("xread: %s\n", strerror(errno));
                return -1;
            } else {
                res=0;
            }
        }
        rest-=res;
        da+=res;
    }
    return num;
}
//---------------------------------------------------------------------------//
// format of scaler data:
// int*4 magic == 0x12345678
// int*8 timestamp
// int*4 nr_modules
//     int*4 nr_channels
//         int*8 val
static int
read_record(int p)
{
    ems_u32 magic;
    bool wenden;

    switch (xread(p, &magic, 4)) {
    case -1: // error
        cerr<<"read magic: "<<strerror(errno)<<endl;
        return -1;
    case 0:
        cerr<<"read: end of file"<<endl;
        return 0;
    default: // OK
        break;
    }
    switch (magic) {
    case 0x12345678: wenden=false; break;
    case 0x78563412: wenden=true; break;
    default:
        cerr<<"unknown endian 0x"<<hex<<setfill('0')<<setw(8)<<magic
            <<setfill(' ')<<dec<<endl;
        return -1;
    }
    if (wenden) {
        cerr<<"swapping not yet implemented"<<endl;
        return -1;
    }
    if (xread(p, &scal_a.timestamp, 8)<=0)
        return -1;
    if (xread(p, &scal_a.nr_modules, 4)<=0)
        return -1;
    for (unsigned int m=0; m<scal_a.nr_modules; m++) {
        if (xread(p, &scal_a.nr_channels[m], 4)<=0)
            return -1;
        for (unsigned int c=0; c<scal_a.nr_channels[m]; c++) {
            if (xread(p, &scal_a.val[m][c], 8)<=0)
                return -1;
            if (scal_a.val[m][c]&0x80000000)
                scal_a.val[m][c]+=0x100000000;
        }
    }
    return 1;
}
//---------------------------------------------------------------------------//
static int
cmp_scaldata(void)
{
#if 0
printf("t=%lld\n", scal_a.timestamp);
printf("nr_modules=%d\n", scal_a.nr_modules);
for (int m=0; m<scal_a.nr_modules; m++) {
    printf("module %d, %d channels\n", m, scal_a.nr_channels[m]);
    for (int c=0; c<scal_a.nr_channels[m]; c++) {
        printf("0x%16llx\n", scal_a.val[m][c]);
    }
}
#endif

    double tdiff=(scal_a.timestamp-scal_b.timestamp)/1000000.;
    if ((tdiff<0)||(tdiff>10)) {
        cout<<"tdiff="<<tdiff<<endl;
    }
    if (scal_a.nr_modules!=scal_b.nr_modules) {
        cout<<"a.nr_modules="<<scal_a.nr_modules<<" b.nr_modules="
            <<scal_b.nr_modules<<endl;
    }
    for (unsigned int m=0; m<scal_a.nr_modules; m++) {
        if (scal_a.nr_channels[m]!=scal_b.nr_channels[m]) {
            cout<<uppercase
                <<"a.nr_channels["<<m<<"]="<<scal_a.nr_channels[m]
                <<" b.nr_channels["<<m<<"]="<<scal_b.nr_channels[m]<<endl;
            return -1;
        }
        for (unsigned int c=0; c<scal_a.nr_channels[m]; c++) {
            ems_u64 al=scal_a.val[m][c]&0xffffffff;
            ems_u64 ah=(scal_a.val[m][c]>>32)&0xffffffff;
            ems_u64 bl=scal_b.val[m][c]&0xffffffff;
            ems_u64 bh=(scal_b.val[m][c]>>32)&0xffffffff;
            if (al>=bl) {
                if (ah!=bh) {
                    cout<<"A "<<m<<':'<<c<<" al="<<hex<<setfill('0')
                        <<uppercase
                        <<setw(8)<<al
                        <<" bl="<<setw(8)<<bl
                        <<" ah="<<setw(8)<<ah
                        <<" bh="<<setw(8)<<bh
                        <<setfill(' ')<<dec<<endl;
                }
            } else {
                if (ah!=bh+1) {
                    cout<<"B "<<m<<':'<<c<<" al="<<hex<<setfill('0')
                        <<setw(8)<<al
                        <<" bl="<<setw(8)<<bl
                        <<" ah="<<setw(8)<<ah
                        <<" bh="<<setw(8)<<bh
                        <<setfill(' ')<<dec<<endl;
                }
            }
        }
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
main(int argc, char* argv[])
{
    string fname=argc>1?argv[1]:"-";
    compressed_input input(fname);

    if (!input.good()) {
        cerr<<"cannot open input file "<<fname<<endl;
        return 1;
    }

    if (read_record(input.path())<=0)
        return 2;
    scal_b=scal_a;

    while (read_record(input.path())>0) {
        if (cmp_scaldata()<0)
            return 3;
        scal_b=scal_a;
    }

    return 0;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
