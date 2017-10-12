/*
 * dataout/cluster/do_cluster_tsm.c
 * created      26.05.98 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: do_cluster_tsm.c,v 1.25 2010/04/19 14:31:30 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#include <sys/param.h>
#include <sys/mount.h>
#include <unsoltypes.h>
#include "do_cluster.h"
#include "../../main/scheduler.h"
#include "../../objects/domain/dom_dataout.h"
#include "../../objects/pi/readout.h"
#include "../../objects/ved/ved.h"
#include "../../commu/commu.h"

#ifdef __linux__
#define LINUX_LARGEFILE O_LARGEFILE
#else
#define LINUX_LARGEFILE 0
#endif

#if 1
#    define LOG(fmt, arg...)                     \
        do {                                     \
            printf("%s:%d " fmt "\n",            \
            "do_cluster_tsm.c", \
            __LINE__, ## arg);                   \
        } while (0)
#else
    #define LOG(fmt, arg...)
#endif

struct do_special_tsm {
    int path;
    pid_t filter;
    char filename[MAXPATHLEN+1];
    char filtername[MAXPATHLEN+1];
};

/*
 * Designidee:
 * 
 * Ein EM schreibt Daten in ein 'normales' Dataout, z.B. in einen File und
 * gleichzeitig ins TSM. Die TSM-Prozeduren laufen zur Entkopplung in einem
 * eigenen Thread.
 * Alternativen:
 * 1:
 * Ein EM schreibt Daten in ein 'normales' Dataout und gleichzeitig ueber Netz
 * zu einem zweiten EM auf einem anderen Rechner. Der zweite EM schreibt in
 * TSM. Falls TSM zu langsam ist (vordefinierter Buffer von z.B. 2 GB voll)
 * schreibt er in lokale Files, die er spaeter ins TSM schafft und loescht.
 * 2:
 * EM und normales tsm_archive_ems kommunizieren sinnvoll:
 * Pro Filesystem ein Semaphore. Jeder kann nur schreiben/lesen, wenn er den
 * Semaphore besitzt. EM versucht, bei jedem Run ein anderes Filesystem zu
 * benutzen. Wenn mehr Filesysteme existieren als tsm-Prozesse ist das immer
 * moeglich.
 * TSM sichert immer den aeltesten File, dessen zugeordneten
 * Filesystemsemaphor er acquirieren kann.
 */

/*****************************************************************************/
