/*
 * lowlevel/jtag/read_mcs.c
 * created 2006-Aug-26 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: mcs2bin.c,v 1.2 2011/04/06 20:30:24 wuestner Exp $";

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <rcs_ids.h>

unsigned char* buf=0;
int bufsize, n;

RCS_REGISTER(cvsid, "lowlevel/jtag")

/****************************************************************************/
static int
read_mcs(void)
{
    char s[1024], *p;
    int line=0, eof=0, i, l, r;
    int rec_len, rec_addr, rec_type, rec_sum, sum;
    int rec_data[512];
    u_int8_t *dd, *end;

    while (fgets(s, 1024, stdin)) {
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
            return -1;
        }
        p++; l--; /* skip colon */
        if ((l&1)||(l<10)) {
            printf("read_mcs(line %d): illegal length %d\n", line, l);
            return -1;
        }
        r=sscanf(p, "%2x%4x%2x", &rec_len, &rec_addr, &rec_type);
        if (r!=3) {
            printf("read_mcs(line %d): parse error\n", line);
            return -1;
        }
        sum=rec_len+(rec_addr>>8)+(rec_addr&0xff)+rec_type;
        p+=8; l-=8;
        if (l!=rec_len*2+2) {
            printf("read_mcs(line %d): inconsistent length\n", line);
            return -1;
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
            return -1;
        }

        switch (rec_type) {
        case 0: /* data */
            while (n+rec_len>bufsize) {
                bufsize=bufsize?bufsize*2:16;
            };
            if (!(buf=realloc(buf, bufsize))) {
                perror("realloc buffer");
                return -1;
            }
            for (i=0; i<rec_len; i++) {
                buf[n++]=rec_data[i];
            }
            break;
        case 1: /* end-of-file */
            fprintf(stderr, "read_mcs(line %d): end of file\n", line);
            break;
        case 2: /* extended segment address (ignored) */
            fprintf(stderr, "read_mcs(line %d): segment address\n", line);
            break;
        case 4: {/* extended linear address (ignored) */
                int addr=0;
                for (i=0; i<rec_len; i++) {
                    addr<<=16;
                    addr+=rec_data[i];
                }
                fprintf(stderr, "read_mcs(line %d): extended linear address: %s\n",
                        line, s);
                fprintf(stderr, "  addr=0x%x; current=0x%x\n", addr, n);
            }
            break;
        default:
            fprintf(stderr, "unknown record type 0x%02x\n", rec_type);
        }
    }

    fprintf(stderr, "read_mcs: 0x%x bytes read\n", n);

    return 0;
}
/****************************************************************************/
static int
write_bin()
{
    int i;

    for (i=0; i<n; i++) {
        write(1, buf+i, 1);
    }
    return 0;
}
/****************************************************************************/
int
main(int argc, char* argv[])
{
    buf=0;
    bufsize=0;
    n=0;
    read_mcs();
    write_bin();
}
/****************************************************************************/
/****************************************************************************/
