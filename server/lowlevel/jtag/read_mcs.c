/*
 * lowlevel/jtag/read_mcs.c
 * created 2006-Aug-26 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: read_mcs.c,v 1.2 2011/04/06 20:30:24 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "lowlevel/jtag")

/****************************************************************************/
static size_t
guess_size(int p)
{
    struct stat buf;

    if (fstat(p, &buf)<0) {
        printf("read_mcs: stat: %s\n", strerror(errno));
        return 0;
    }
    if (buf.st_size==0)
        printf("read_mcs: empty file\n");
    return buf.st_size/2;
}

int
read_mcs(const char* name, ems_u8** data, int* len)
{
    FILE* f;
    char s[1024], *p;
    ems_u8 *d, *dd;
    size_t size;
    int res=0, line=0, eof=0, i, l, r;
    int rec_len, rec_addr, rec_type, rec_sum, sum;
    int rec_data[512];

    *data=d=0;

    f=fopen(name, "r");
    if (!f) {
        printf("read_mcs: open \"%s\": %s\n", name, strerror(errno));
        return -1;
    }

    size=guess_size(fileno(f));
    if (!size)
        return -1;
    d=dd=malloc(size);
    if (!d) {
        printf("read_mcs: cannot allocate %d bytes: %s\n", size,
            strerror(errno));
        goto error;
    }

    while (fgets(s, 1024, f)) {
        line++;

        if (eof) {
            printf("read_mcs(line %d): after end-of-file record\n", line);
            continue;
        }

        l=strl(s);
        if (s[l]=='\n')
            s[l--]=0;
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
        printf("l=%02x a=%04x t=%02x\n", rec_len, rec_addr, rec_type);
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
            for (i=0; i<rec_len; i++) {
                *dd++=rec_data[i];
            }
            break;
        case 1: /* end-of-file */
            printf("read_mcs(line %d): end of file\n", line);
            break;
        case 2: /* extended segment address */
            printf("read_mcs(line %d): segment address\n", line);
            break;
        case 4: /* extended linear address */
            printf("read_mcs(line %d): extended linear address\n", line);
            break;
        default:
            printf("unknown record type 0x%02x\n", rec_type);
        }
    }

    *len=dd-d;
    *data=d;
    printf("read_mcs: 0x%x bytes read\n", *len);

error:
    fclose(f);
    if (d && res)
        free(d);
    return res;
}
/****************************************************************************/
int
write_mcs(const char name, ems_u8* data, int len)
{
}
/****************************************************************************/
/****************************************************************************/
