/*
 * common/clusterformat.h
 * created: 15.03.1998
 * 09.May.2001 PW: clusterty_file added
 */
/*
 * static char *rcsid="$ZEL: clusterformat.h,v 1.8 2011/04/12 14:28:29 wuestner Exp $";
 */

#ifndef _clusterformat_h_
#define _clusterformat_h_
/*
  Beschreibung siehe ems/server/dataout/cluster/CLUSTER
*/

enum clustertypes {
    clusterty_events=0,
    clusterty_ved_info=1,
    clusterty_text=2,
    /*clusterty_x_wendy_setup=3,*/
    clusterty_file=4,
    clusterty_async_data=5,
    clusterty_no_more_data=0x10000000
};

enum clusterflags {
    clusterfl_fragmented=1,
    clusterfl_fragfollows=2
};

#define ClOPTFL_TIME 1
#define ClOPTFL_CHECK 2

#define CHECKSUMSIZE_MD5 4
#define CHECKSUMTYPE_MD5 ('M'<<24|'D'<<16|'5'<<8)
#define CHECKSUMSIZE_MD4 4
#define CHECKSUMTYPE_MD4 ('M'<<24|'D'<<16|'4'<<8)

/* Makros, um auf Cluster.data zuzugreifen */
#define ClLEN(d)     ((d)[0])
#define ClENDIEN(d)  ((d)[1])
#define ClTYPE(d)    ((d)[2])
#define ClOPTSIZE(d) ((d)[3])
#define ClOPTFLAGS(d) ((d)[4])
/* Achtung! Cluster darf nicht geswapped sein */
#define ClFLAGS(d)   ((d)[4+ClOPTSIZE(d)])
#define ClVEDID(d)   ((d)[5+ClOPTSIZE(d)])
#define ClFRAGID(d)  ((d)[6+ClOPTSIZE(d)])
#define ClEVNUM(d)   ((d)[7+ClOPTSIZE(d)])
#define ClEVSIZ1(d)  ((d)[8+ClOPTSIZE(d)])
#define ClEVID1(d)   ((d)[9+ClOPTSIZE(d)])
/* fuer geswappte Cluster */
#define ClsFLAGS(d, o)   ((d)[4+(o)])
#define ClsVEDID(d, o)   ((d)[5+(o)])
#define ClsFRAGID(d, o)  ((d)[6+(o)])
#define ClsEVNUM(d, o)   ((d)[7+(o)])
#define ClsEVSIZ1(d, o)  ((d)[8+(o)])
#define ClsEVID1(d, o)   ((d)[9+(o)])

#endif
