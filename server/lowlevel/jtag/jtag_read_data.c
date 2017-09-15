/*
 * lowlevel/jtag/jtag_read_data.c
 * created 2006-Aug-26 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag_read_data.c,v 1.5 2011/04/06 20:30:24 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <emsctypes.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "jtag_int.h"

RCS_REGISTER(cvsid, "lowlevel/jtag")

/****************************************************************************/
static plerrcode
jtag_read_mcs(int path, ems_u8* data, int size)
{
    FILE* f;
    char s[1024], *p;
    int pres=plErr_Other, line=0, eof=0, i, l, r;
    int rec_len, rec_addr, rec_type, rec_sum, sum;
    int rec_data[512];
    ems_u8 *dd, *end;

    f=fdopen(path, "r");
    if (!f) {
        printf("read_mcs: fdopen: %s\n", strerror(errno));
        return plErr_System;
    }

    dd=data;
    end=data+size;

    while (fgets(s, 1024, f)) {
        line++;

        if (eof) {
            printf("read_mcs(line %d): after end-of-file record\n", line);
            continue;
        }

        l=strlen(s);
        while (isspace(s[l-1]))
            s[--l]=0;
        p=s;

        if (p[0]!=':') {
            printf("read_mcs(line %d): first character is not a colon\n",
                line);
            goto error;
        }
        p++; l--; /* skip colon */
        if ((l&1)||(l<10)) {
            printf("read_mcs(line %d): illegal length %d\n", line, l);
            goto error;
        }
        r=sscanf(p, "%2x%4x%2x", &rec_len, &rec_addr, &rec_type);
        if (r!=3) {
            printf("read_mcs(line %d): parse error\n", line);
            goto error;
        }
        sum=rec_len+(rec_addr>>8)+(rec_addr&0xff)+rec_type;
        p+=8; l-=8;
        if (l!=rec_len*2+2) {
            printf("read_mcs(line %d): inconsistent length\n", line);
            goto error;
        }
        for (i=0; i<rec_len; i++) {
            r=sscanf(p, "%2x", &rec_data[i]);
            sum+=rec_data[i];
            p+=2; l-=2;
        }
        r=sscanf(p, "%2x", &rec_sum);
        sum+=rec_sum;
        if (sum&0xff) {
            printf("read_mcs(line %d): checksum error\n", line);
            goto error;
        }

        switch (rec_type) {
        case 0: /* data */
            if (dd+rec_len>end) {
                printf("read_mcs: too many data\n");
                goto error;
            }
            for (i=0; i<rec_len; i++) {
                *dd++=rec_data[i];
            }
            break;
        case 1: /* end-of-file */
            printf("read_mcs(line %d): end of file\n", line);
            break;
        case 2: /* extended segment address (ignored) */
            printf("read_mcs(line %d): segment address\n", line);
            break;
        case 4: /* extended linear address (ignored) */
            printf("read_mcs(line %d): extended linear address\n", line);
            break;
        default:
            printf("unknown record type 0x%02x\n", rec_type);
        }
    }

    if (dd!=end) {
        printf("read_mcs: incorrect data size: 0x%llx instead of 0x%x\n",
            (unsigned long long)(dd-data), size);
#if 0
        goto error;
#else
        memset(dd, 0xff, end-dd);
#endif
    }
    printf("read_mcs: 0x%x bytes read\n", size);
    pres=plOK;

error:
    fclose(f);
    return pres;
}
/****************************************************************************/
static plerrcode
jtag_read_binary(int p, void* data, int size)
{
    if (read(p, data, size)!=size) {
        printf("jtag_read_binary: read: %s\n", strerror(errno));
        return plErr_System;
    }
    return plOK;   
}
/****************************************************************************/
/*
 * !!! Wenn zu kurz: eventuell mit 0xFF auffuellen
 */
plerrcode
jtag_read_data(const char* name, void* data, int size)
{
    plerrcode pres=plOK;
    struct stat statbuf;
    int p=-1;

    /* printf("jtag_read_data: data=%p, size=%d\n", data, size); */
    p=open(name, O_RDONLY, 0);
    if (p<0) {
        printf("jtag_read_data: open \"%s\": %s\n", name, strerror(errno));
        pres=plErr_System;
        goto error;
    }

    if (fstat(p, &statbuf)<0) {
        printf("jtag_read_data: fstat \"%s\": %s\n", name, strerror(errno));
        pres=plErr_System;
        goto error;
    }

    if (statbuf.st_size==size) {
        pres=jtag_read_binary(p, data, size);
    } else {
        char c;
        if (read(p, &c, 1)!=1) {
            printf("jtag_read_data: error reading data: %s\n", strerror(errno));
            pres=plErr_System;
            goto error;
        }
        lseek(p, 0, SEEK_SET);
        if (c!=':') {
            printf("jtag_read_data: \"%s\" is neither binary nor a Intel HEX\n",
                name);
            pres=plErr_Other;
            goto error;
        }
        pres=jtag_read_mcs(p, data, size);
    }

error:
    close(p);
    return pres;
}
/****************************************************************************/
#if 0
int
write_mcs(const char* name, void* data, int size)
{
}
#endif
/****************************************************************************/
/****************************************************************************/
