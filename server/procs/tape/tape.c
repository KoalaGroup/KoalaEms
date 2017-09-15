/*
 * tape.c
 *       
 * created 17.07.1998 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: tape.c,v 1.6 2011/10/17 23:14:47 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <xdrstring.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"
#include "../../dataout/cluster/do_cluster_tape.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/tape")

/*****************************************************************************/
plerrcode proc_TapeErase(ems_u32* p)
{
return tape_erase(p[1], p[2]);
}
plerrcode test_proc_TapeErase(ems_u32* p)
{
if (p[0]!=2) return plErr_ArgNum;
wirbrauchen=4;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeErase_prop={1, 5, "do_idx immediate", "Erase Tape"};
procprop* prop_proc_TapeErase(void)
{
return &TapeErase_prop;
}
#endif
char name_proc_TapeErase[]="TapeErase";
int ver_proc_TapeErase=1;
/*****************************************************************************/
plerrcode proc_TapeInquiry(ems_u32* p)
{
plerrcode res;
printf("proc_TapeInquiry called\n");
res=tape_inquiry(p[1]);
printf("leave proc_TapeInquiry; res=%d\n", res);
return res;
}
plerrcode test_proc_TapeInquiry(ems_u32* p)
{
if (p[0]!=1) return plErr_ArgNum;
wirbrauchen=10;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeInquiry_prop={1, 10, "do_id", "Inquire Tape and returns names"};
procprop* prop_proc_TapeInquiry(void)
{
return &TapeInquiry_prop;
}
#endif
char name_proc_TapeInquiry[]="TapeInquiry";
int ver_proc_TapeInquiry=1;
/*****************************************************************************/
plerrcode proc_TapeLoad(ems_u32* p)
{
return tape_load(p[1], 1, p[2]);
}
plerrcode test_proc_TapeLoad(ems_u32* p)
{
if (p[0]!=2) return plErr_ArgNum;
wirbrauchen=5;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeLoad_prop={1, 5, "do_id immediate", "Load Tape"};
procprop* prop_proc_TapeLoad(void)
{
return &TapeLoad_prop;
}
#endif
char name_proc_TapeLoad[]="TapeLoad";
int ver_proc_TapeLoad=1;
/*****************************************************************************/
plerrcode proc_TapeUnload(ems_u32* p)
{
plerrcode res;
res=tape_load(p[1], 0, p[2]);
printf("proc_TapeUnload(%d, %d) returns %d\n", p[1], p[2], res);
return res;
}
plerrcode test_proc_TapeUnload(ems_u32* p)
{
if (p[0]!=2) return plErr_ArgNum;
wirbrauchen=5;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeUnload_prop={1, 5, "do_id immediate", "Unload Tape"};
procprop* prop_proc_TapeUnload(void)
{
return &TapeUnload_prop;
}
#endif
char name_proc_TapeUnload[]="TapeUnload";
int ver_proc_TapeUnload=1;
/*****************************************************************************/
plerrcode proc_TapeLocate(ems_u32* p)
{
return tape_locate(p[1], p[2]);
}
plerrcode test_proc_TapeLocate(ems_u32* p)
{
if (p[0]!=2) return plErr_ArgNum;
wirbrauchen=5;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeLocate_prop={0, 0, 0, 0};
procprop* prop_proc_TapeLocate(void)
{
return &TapeLocate_prop;
}
#endif
char name_proc_TapeLocate[]="TapeLocate";
int ver_proc_TapeLocate=1;
/*****************************************************************************/
plerrcode proc_TapeClearLog(ems_u32* p)
{
return tape_clearlog(p[1]);
}
plerrcode test_proc_TapeClearLog(ems_u32* p)
{
if (p[0]!=1) return plErr_ArgNum;
wirbrauchen=4;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeClearLog_prop={1, 5, "do_id", "Clear Log"};
procprop* prop_proc_TapeClearLog(void)
{
return &TapeClearLog_prop;
}
#endif
char name_proc_TapeClearLog[]="TapeClearLog";
int ver_proc_TapeClearLog=1;
/*****************************************************************************/
plerrcode proc_TapeModeSelect(ems_u32* p)
{
return tape_modeselect(p[1], p[2]);
}
plerrcode test_proc_TapeModeSelect(ems_u32* p)
{
if (p[0]!=2) return plErr_ArgNum;
wirbrauchen=4;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeModeSelect_prop={1, 5, "do_id density", "Select Density"};
procprop* prop_proc_TapeModeSelect(void)
{
return &TapeModeSelect_prop;
}
#endif
char name_proc_TapeModeSelect[]="TapeModeSelect";
int ver_proc_TapeModeSelect=1;
/*****************************************************************************/
plerrcode proc_TapeModeSense(ems_u32* p)
{
return tape_modesense(p[1]);
}
plerrcode test_proc_TapeModeSense(ems_u32* p)
{
if (p[0]!=1) return plErr_ArgNum;
wirbrauchen=4;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeModeSense_prop={1, 5, "do_id", "Report Density"};
procprop* prop_proc_TapeModeSense(void)
{
return &TapeModeSense_prop;
}
#endif
char name_proc_TapeModeSense[]="TapeModeSense";
int ver_proc_TapeModeSense=1;
/*****************************************************************************/
plerrcode proc_TapePreventRemoval(ems_u32* p)
{
return tape_prevent_allow_removal(p[1], 1);
}
plerrcode test_proc_TapePreventRemoval(ems_u32* p)
{
if (p[0]!=1) return plErr_ArgNum;
wirbrauchen=5;
return plOK;
}
#ifdef PROCPROPS
static procprop TapePreventRemoval_prop={1, 5, "do_id", "Prevent Removal"};
procprop* prop_proc_TapePreventRemoval(void)
{
return &TapePreventRemoval_prop;
}
#endif
char name_proc_TapePreventRemoval[]="TapePreventRemoval";
int ver_proc_TapePreventRemoval=1;
/*****************************************************************************/
plerrcode proc_TapeAllowRemoval(ems_u32* p)
{
plerrcode res;
res=tape_prevent_allow_removal(p[1], 0);
return res;
}
plerrcode test_proc_TapeAllowRemoval(ems_u32* p)
{
if (p[0]!=1) return plErr_ArgNum;
wirbrauchen=5;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeAllowRemoval_prop={1, 5, "do_id", "Allow Removal"};
procprop* prop_proc_TapeAllowRemoval(void)
{
return &TapeAllowRemoval_prop;
}
#endif
char name_proc_TapeAllowRemoval[]="TapeAllowRemoval";
int ver_proc_TapeAllowRemoval=1;
/*****************************************************************************/
plerrcode proc_TapeReadPos(ems_u32* p)
{
return tape_readpos(p[1]);
}
plerrcode test_proc_TapeReadPos(ems_u32* p)
{
if (p[0]!=1) return plErr_ArgNum;
wirbrauchen=5;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeReadPos_prop={1, 5, "do_id", "Read Position"};
procprop* prop_proc_TapeReadPos(void)
{
return &TapeReadPos_prop;
}
#endif
char name_proc_TapeReadPos[]="TapeReadPos";
int ver_proc_TapeReadPos=1;
/*****************************************************************************/
plerrcode proc_TapeRewind(ems_u32* p)
{
return tape_rewind(p[1], p[2]);
}
plerrcode test_proc_TapeRewind(ems_u32* p)
{
if (p[0]!=2) return plErr_ArgNum;
wirbrauchen=5;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeRewind_prop={1, 5, "do_id immediate", "Rewind Tape"};
procprop* prop_proc_TapeRewind(void)
{
return &TapeRewind_prop;
}
#endif
char name_proc_TapeRewind[]="TapeRewind";
int ver_proc_TapeRewind=1;
/*****************************************************************************/
/*
 * [0]: size==3
 * [1]: do_id
 * [2]: code: 0: blocks, 1: filemarks, 3: eod, 4: setmarks
 * [3]: count
 */
plerrcode proc_TapeSpace(ems_u32* p)
{
return tape_space(p[1], p[2], p[3]);
}
plerrcode test_proc_TapeSpace(ems_u32* p)
{
if (p[0]!=3) return plErr_ArgNum;
wirbrauchen=5;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeSpace_prop={1, 5, "do_id code count", "Space Tape"};
procprop* prop_proc_TapeSpace(void)
{
return &TapeSpace_prop;
}
#endif
char name_proc_TapeSpace[]="TapeSpace";
int ver_proc_TapeSpace=1;
/*****************************************************************************/
plerrcode proc_TapeWrite(ems_u32* p)
{
return tape_write(p[1], p[2], p+3);
}
plerrcode test_proc_TapeWrite(ems_u32* p)
{
return plOK;
}
#ifdef PROCPROPS
static procprop TapeWrite_prop={1, 5, "do_idx num data", "Write Tape"};
procprop* prop_proc_TapeWrite(void)
{
return &TapeWrite_prop;
}
#endif
char name_proc_TapeWrite[]="TapeWrite";
int ver_proc_TapeWrite=1;
/*****************************************************************************/
plerrcode proc_TapeFilemark(ems_u32* p)
{
return tape_writefilemarks(p[1], p[2], p[3], p[4]);
}
plerrcode test_proc_TapeFilemark(ems_u32* p)
{
if (p[0]!=4) return plErr_ArgNum;
wirbrauchen=5;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeFilemark_prop={1, 5, "do_idx num immediate force",
    "Write Filemarks"};
procprop* prop_proc_TapeFilemark(void)
{
return &TapeFilemark_prop;
}
#endif
char name_proc_TapeFilemark[]="TapeFilemark";
int ver_proc_TapeFilemark=1;
/*****************************************************************************/
plerrcode proc_TapeDebug(ems_u32* p)
{
return tape_debug(p[1], p[2]);
}
plerrcode test_proc_TapeDebug(ems_u32* p)
{
if (p[0]!=2) return plErr_ArgNum;
wirbrauchen=0;
return plOK;
}
#ifdef PROCPROPS
static procprop TapeDebug_prop={1, 5, "do_idx code",
    "set debug code"};
procprop* prop_proc_TapeDebug(void)
{
return &TapeDebug_prop;
}
#endif
char name_proc_TapeDebug[]="TapeDebug";
int ver_proc_TapeDebug=1;
/*****************************************************************************/
/*****************************************************************************/
