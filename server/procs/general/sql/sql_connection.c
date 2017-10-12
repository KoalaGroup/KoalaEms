/*
 * server/procs/general/sql/sql_connection.c
 * 
 * created: 2006-Nov-19 PW
 */
/*
 * static char *rcsid="$ZEL$";
 */

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include "../../procs.h"
#include "../../../objects/ved/ved.h"

extern ems_u32* outptr;
extern int wirbrauchen;

/*****************************************************************************/
/*
 * p[0]: argnum (==0)
 */
plerrcode proc_clear_sql_connection(ems_u32* p)
{
    struct sqldb *sqldb=&ved_globals.sqldb;

    sqldb->valid=0;
    sqldb->do_id=-1;
    free(sqldb->host);    sqldb->host=0;
    free(sqldb->user);    sqldb->user=0;
    free(sqldb->passwd);  sqldb->passwd=0;
    free(sqldb->db);      sqldb->db=0;
    sqldb->port=0;

    return plOK;
}

plerrcode test_proc_clear_sql_connection(ems_u32* p)
{
    if (p[0]!=0)
        return plErr_ArgNum;

    wirbrauchen=0;

    return plOK;
}

char name_proc_clear_sql_connection[]="clear_sql_connection";
int ver_proc_clear_sql_connection=1;
/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: do_ID
 * p[2...]: host  // string
 * p[...]: user   // string
 * p[...]: passwd // string
 * p[...]: db     // string
 * p[...]: port   // int
 */
plerrcode proc_set_sql_connection(ems_u32* p)
{
    struct sqldb *sqldb=&ved_globals.sqldb;
    ems_u32* pp;

    free(sqldb->host);    sqldb->host=0;
    free(sqldb->user);    sqldb->user=0;
    free(sqldb->passwd);  sqldb->passwd=0;
    free(sqldb->db);      sqldb->db=0;

    sqldb->do_id=p[1];
    pp=xdrstrcdup(&sqldb->host, p+2);
    pp=xdrstrcdup(&sqldb->user, pp);
    pp=xdrstrcdup(&sqldb->passwd, pp);
    pp=xdrstrcdup(&sqldb->db, pp);
    sqldb->port=*pp;
    sqldb->valid=1;
    printf("sql_connection: %s, %s, %s, %s, %d\n",
        sqldb->host, sqldb->user, sqldb->passwd, sqldb->db, sqldb->port);

    return plOK;
}

plerrcode test_proc_set_sql_connection(ems_u32* p)
{
    int idx;

    if (p[0]<2)
        return plErr_ArgNum;

    idx=2;
    idx+=xdrstrlen(p+idx); /* host */
    if (idx>p[0]) {
        printf("dataout_sql_connection(A): idx=%d, p[0]=%d\n", idx, p[0]);
        return plErr_ArgNum;
    }
    idx+=xdrstrlen(p+idx); /* user */
    if (idx>p[0]) {
        printf("dataout_sql_connection(B): idx=%d, p[0]=%d\n", idx, p[0]);
        return plErr_ArgNum;
    }
    idx+=xdrstrlen(p+idx); /* passwd */
    if (idx>p[0]) {
        printf("dataout_sql_connection(C): idx=%d, p[0]=%d\n", idx, p[0]);
        return plErr_ArgNum;
    }
    idx+=xdrstrlen(p+idx); /* db */
    if (idx>p[0]) {
        printf("dataout_sql_connection(D): idx=%d, p[0]=%d\n", idx, p[0]);
        return plErr_ArgNum;
    }

    if (idx==p[0]) { /* port */
        printf("dataout_sql_connection(E): idx=%d, p[0]=%d\n", idx, p[0]);
        return plErr_ArgNum;
    }

    if (idx<p[0]) {
        printf("dataout_sql_connection(F): idx=%d, p[0]=%d\n", idx, p[0]);
        return plErr_ArgNum;
    }

    wirbrauchen=0;

    return plOK;
}

char name_proc_set_sql_connection[]="set_sql_connection";
int ver_proc_set_sql_connection=1;

/*****************************************************************************/
/*
 * p[0]: argnum
 */

plerrcode proc_get_sql_connection(ems_u32* p)
{
    struct sqldb *sqldb=&ved_globals.sqldb;

    *outptr++=sqldb->valid;

    if (sqldb->valid) {
        *outptr+=sqldb->do_id;
        if (sqldb->host)
            outptr=outstring(outptr, sqldb->host);
        else
            outptr=outstring(outptr, "");
        if (sqldb->user)
            outptr=outstring(outptr, sqldb->user);
        else
            outptr=outstring(outptr, "");
        if (sqldb->passwd)
            outptr=outstring(outptr, sqldb->passwd);
        else
            outptr=outstring(outptr, "");
        if (sqldb->db)
            outptr=outstring(outptr, sqldb->db);
        else
            outptr=outstring(outptr, "");
        *outptr++=sqldb->port;
    }
    return plOK;
}

plerrcode test_proc_get_sql_connection(ems_u32* p)
{
    if (p[0]!=0)
        return plErr_ArgNum;

    wirbrauchen=-1;
    return plOK;
}

char name_proc_get_sql_connection[]="get_sql_connection";
int ver_proc_get_sql_connection=1;

/*****************************************************************************/
/*****************************************************************************/
