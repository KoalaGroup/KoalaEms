/*
 * procs/hlral/hlralops.c
 * 01.Aug.2001 PW: HLRALdummyreadout added
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: hlralops.c,v 1.12 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <sys/types.h>
#include <cdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../lowlevel/hlral/hlralreg.h"
#include "../../lowlevel/hlral/hlral_int.h"
#include "../../lowlevel/oscompat/oscompat.h"
#include "../../lowlevel/devices.h"
#include "../procs.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/hlral")

/*****************************************************************************/
/*
 * p[0]: length (1..3)
 * p[1]: board
 * [p[2]: test registers
 * [p[3]: column]]
 */
plerrcode
proc_HLRALconfig(ems_u32* p)
{
        struct hlral* dev;
	int i, res;
        dev=get_pcihl_device(p[1]);
/*
	if ((res = hlral_reset(board))) {
		*outptr++ = res;
		return (plErr_HW);
	}
*/
        if (p[0]>2) {
                res = hlral_countchips(dev, p[3], p[2]);
                if (res == -1)
                        return plErr_HW;
		*outptr++ = res;
        } else {
	        for(i = 0; i < HLRAL_MAX_COLUMNS; i++) {
                        res = hlral_countchips(dev, i, p[0]>1?p[2]:0);
                        if (res == -1)
                                return plErr_HW;
		        *outptr++ = res;
	        }
        }
/*
	hlral_reset(board);
*/
	return plOK;
}

plerrcode
test_proc_HLRALconfig(ems_u32* p)
{
        plerrcode pres;
	if ((p[0]<1) || (p[0]>3))
		return (plErr_ArgNum);
        if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
            return pres;
        if (p[0]>1 && (p[2]>1)) return plErr_ArgRange;
	return (plOK);
}

char name_proc_HLRALconfig[] = "HLRALconfig";

int ver_proc_HLRALconfig = 1;
/*****************************************************************************/
/*
 * p[0]: length (==3)
 * p[1]: board
 * p[2]: column (crate)
 * p[3]: 1: testregs or 0: daqregs
 */
plerrcode proc_HLRALdatapathtest(ems_u32* p)
{
        struct hlral* dev;
        dev=get_pcihl_device(p[1]);
        *outptr++=hlral_datapathtest(dev, p[2], p[3])?0:1;
        return plOK;
}

plerrcode
test_proc_HLRALdatapathtest(ems_u32* p)
{
        plerrcode pres;
	if (p[0] != 3)
		return (plErr_ArgNum);
        if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
            return pres;
	return plOK;
}

char name_proc_HLRALdatapathtest[] = "HLRALdatapathtest";

int ver_proc_HLRALdatapathtest = 1;
/*****************************************************************************/
/*
 * p[0]: length
 * p[1]: board
 * p[2]: column (crate)
 * p[3]:
 *       0: alles null, abgesehen von den angegeben Kanaelen
 *       1: alles eins, abgesehen von den angegeben Kanaelen
 *       2: data in xdrstring
 *       3: data in word/chip
 * p[4]: data / xdrstring
 */
plerrcode
proc_HLRALloadtestregs(ems_u32* p)
{
        struct hlral* dev;
	u_int8_t data[HLRAL_MAX_ROWS * 4];
	int i, len, oldlen;
	char *tmp;

        dev=get_pcihl_device(p[1]);
        oldlen=hlral_read_old_data(dev, p[2], 0/*testregs*/, (ems_u8*)outptr);
        if (oldlen<0) {
            printf("HLRALloadtestregs: read_old_data failed\n");
            return plErr_HW;
        }

        len = hlral_countchips(dev, p[2], 1) * 4;
        if (len <= 0 || len > HLRAL_MAX_ROWS * 4)
		return plErr_HW;

	switch (p[3]) {
	case 0:
	case 1:
		memset(data, p[3] == 0 ? 0 : 0x0f, len);
		for (i = 4; i <= p[0]; i++) {
			int bytepos;
			u_int8_t bit;
			bytepos = (p[i] / 16) * 4 + (3 - (p[i] / 4) % 4);
			if (bytepos >= len)
				return (plErr_ArgRange);
			bit = (1 << (p[i] % 4));
			if (p[3] == 0)
				data[bytepos] |= bit;
			else
				data[bytepos] &= ~bit;
		}
		break;
	case 2:
		if (xdrstrclen(p+4) * 2 != len)
			return (plErr_ArgNum);
		xdrstrcdup(&tmp, p+4);
		for (i = 0; i < len / 4; i++) {
			data[4 * i + 3] = tmp[2 * i] & 0x0f;
			data[4 * i + 2] = tmp[2 * i] >> 4;
			data[4 * i + 1] = tmp[2 * i + 1] & 0x0f;
			data[4 * i] = tmp[2 * i + 1] >> 4;
		}
		free(tmp);
		break;
        case 3:
                if (p[0]==4) {
                  for (i=0; i<len/4; i++) {
                    data[4 * i + 3] = p[4]& 0x0f;
                    data[4 * i + 2] = p[4]>>4 & 0x0f;
                    data[4 * i + 1] = p[4]>>8 & 0x0f;
                    data[4 * i] = p[4]>>12 & 0x0f;
                  }
                } else if (p[0]-3==len/4) {
                  for (i=0; i<len/4; i++) {
                    data[4 * i + 3] = p[4+i]& 0x0f;
                    data[4 * i + 2] = p[4+i]>>4 & 0x0f;
                    data[4 * i + 1] = p[4+i]>>8 & 0x0f;
                    data[4 * i] = p[4+i]>>12 & 0x0f;
                  }
                } else if (p[0]-3==len) {
                  for (i=0; i<len; i++) {
                    data[i] = p[4+i] & 0x0f;
                  }
                } else {
                  return (plErr_ArgNum);
                }
                break;
	}

	if (hlral_loadregs(dev, p[2], 0, data, len))
                return plErr_HW;

        if (oldlen<len) {
            printf("HLRALloadtestregs: read_old_data returned only %d bytes\n",
                    oldlen);
            return plErr_HW;
        }

        for (i=len-1; i>=0; i--) {
            outptr[i]=(((ems_u8*)outptr)[i])&0xf;
        }
        outptr+=len;

	return plOK;
}

plerrcode
test_proc_HLRALloadtestregs(ems_u32* p)
{
        plerrcode pres;
	if (p[0] < 3)
		return (plErr_ArgNum);
        if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
            return pres;
	switch (p[3]) {
	    case 0:
	    case 1:
		if (p[0] > HLRAL_MAX_ROWS * 16 + 3)
			return (plErr_ArgNum);
		break;
	    case 2:
		if ((p[0] < 4) ||
		    (p[0] != xdrstrlen(&p[4]) + 3))
			return (plErr_ArgNum);
		break;
	    case 3:
                if (p[0]<4) return (plErr_ArgNum);
                break;
	    default:
		return (plErr_ArgRange);
	}
	return (plOK);
}

char name_proc_HLRALloadtestregs[] = "HLRALloadtestregs";

int ver_proc_HLRALloadtestregs = 1;
/*****************************************************************************/
/*
 * p[0]: length
 * p[1]: board
 * p[2]: column
 * p[3]: 0: data 1: xdrstring
 * p+4 : data or string
 */
plerrcode
proc_HLRALloaddac(ems_u32* p)
{
        struct hlral* dev;
	ems_u8 data[HLRAL_MAX_ROWS];
        char* sdata=(char*)data; /* extractstring wants char* */
	int len, oldlen, i;

        dev=get_pcihl_device(p[1]);
        oldlen=hlral_read_old_data(dev, p[2], 1/*DACs*/, (ems_u8*)outptr);
        if (oldlen<0) {
            printf("HLRALloaddac: read_old_data failed\n");
            return plErr_HW;
        }

	len = hlral_countchips(dev, p[2],0);
	if (len <= 0 || len > HLRAL_MAX_ROWS) {
                printf("HLRALloaddac: hlral_countchips: len=%d\n", len);
		return (plErr_HW);
        }

	switch (p[3]) {
	case 0:
		memset(data, p[4], len);
		break;
	case 1:
		if (xdrstrclen(&p[4]) != len) {
                        printf("HLRALloaddac: inconsistent strlen\n");
			return (plErr_ArgNum);
                }
		extractstring(sdata, &p[4]);
		break;
	}

	if (hlral_loadregs(dev, p[2], 1, data, len)) {
                printf("HLRALloaddac: hlral_loadregs failed\n");
		return (plErr_HW);
        }

        if (oldlen<len) {
            printf("HLRALloaddac: read_old_data returned only %d bytes\n",
                    oldlen);
            return plErr_HW;
        }
        for (i=len-1; i>=0; i--) {
            outptr[i]=((ems_u8*)outptr)[i];
        }
        outptr+=len;
	return (plOK);
}

plerrcode
test_proc_HLRALloaddac(ems_u32* p)
{
        plerrcode pres;
	if (p[0] < 3)
		return (plErr_ArgNum);
        if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
            return pres;
	switch (p[3]) {
	    case 0:
		if (p[0] != 4)
			return (plErr_ArgNum);
		break;
	    case 1:
		if ((p[0] < 4) ||
		    (p[0] != xdrstrlen(&p[4]) + 3))
			return(plErr_ArgNum);
		break;
	    default:
		return (plErr_ArgRange);
	}
	return (plOK);
}

char name_proc_HLRALloaddac[] = "HLRALloaddac";

int ver_proc_HLRALloaddac = 1;
/*****************************************************************************/
/*
 * p[0]: length
 * p[1]: pci-board
 * p[2]: column
 * p[3]: row    -1: all rows   (row==module)
 * p[4]: channel -1: all channels          row==-1 && channel!=-1 not allowed
 * p[5..]: data
 * #data==1: all selected channels are set to this value
 * #data==#row*#channel: all selected channels are set to there
 *     respective value
 */
plerrcode proc_HLRALloaddac_2(ems_u32* p)
{
    struct hlral* dev;
    ems_i32* sp=(ems_i32*)p; /* for signed access */
    plerrcode res;

    dev=get_pcihl_device(p[1]);
    if (hlral_reset(dev))
        return plErr_HW;
    if ((res=hlral_loaddac_2(dev, p[2], sp[3], sp[4], sp+5, p[0]-4))!=plOK)
        return res;
    hlral_reset(dev);
    return plOK;
}

plerrcode test_proc_HLRALloaddac_2(ems_u32* p)
{
    plerrcode pres;
    int i, num;
    ems_i32* sp=(ems_i32*)p; /* for signed access */

    if (p[0]<5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK) {
        *outptr++=1;
        return pres;
    }
    if ((sp[3]<-1) || (sp[3]>=HLRAL_MAX_ROWS)) {
        *outptr++=3;
        return plErr_ArgRange;
    }
    if ((sp[4]<-1) || (sp[4]>=HLRAL_CHAN_BORD) || ((sp[3]==-1) && (sp[4]>=0))) {
        *outptr++=4;
        return plErr_ArgRange;
    }
    num=1;
    if (sp[3]<0) num*=HLRAL_MAX_ROWS;
    if (sp[4]<0) num*=HLRAL_CHAN_BORD;
    if (p[0]-4>num)
        return plErr_ArgNum;
    for (i=5; i<p[0]; i++) {
        if (p[i]>=255) {
            *outptr++=i;
            return plErr_ArgRange;
        }
    }
    return plOK;
}

char name_proc_HLRALloaddac_2[] = "HLRALloaddac2";

int ver_proc_HLRALloaddac_2 = 2;
/*****************************************************************************/
/*
 * p[0]: length
 * p[1]: board
 * p[2]: mode 0: unbuffered 1: buffered
 */
plerrcode proc_HLRALbuffermode(ems_u32* p)
{
    struct hlral* dev;
    plerrcode res;

    dev=get_pcihl_device(p[1]);
    if (hlral_reset(dev))
        return (plErr_HW);
    if ((res=hlral_buffermode(dev, p[2]))!=plOK)
        return res;
    hlral_reset(dev);
    return plOK;
}

plerrcode test_proc_HLRALbuffermode(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
        return pres;
    if (p[2]>HLRAL_MAX_COLUMNS)
        return plErr_ArgRange;
    return plOK;
}
char name_proc_HLRALbuffermode[] = "HLRALbuffermode";

int ver_proc_HLRALbuffermode = 1;
/*****************************************************************************/
plerrcode
proc_HLRALtestreadout(ems_u32* p)
{
        struct hlral* dev;
	ems_u32 *res;

        dev=get_pcihl_device(p[1]);
	res = hlral_testreadout(dev, outptr);

	if (res)
		outptr = res;
	else
		return plErr_HW;
	return plOK;
}

plerrcode
test_proc_HLRALtestreadout(ems_u32* p)
{
        plerrcode pres;
	if (p[0] != 1)
		return plErr_ArgNum;
        if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
            return pres;
	return plOK;
}

char name_proc_HLRALtestreadout[] = "HLRALtestreadout";

int ver_proc_HLRALtestreadout = 1;
/*****************************************************************************/
plerrcode
proc_HLRALreadout(ems_u32* p)
{
        struct hlral* dev;
	ems_u32 *res;

        dev=get_pcihl_device(p[1]);
	res = hlral_readout(dev, outptr);

	if (res)
		outptr = res;
	else
		return (plErr_HW);
	return (plOK);
}

plerrcode
test_proc_HLRALreadout(ems_u32* p)
{
        plerrcode pres;
	if (p[0] != 1)
		return (plErr_ArgNum);
        if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
            return pres;
	return (plOK);
}

char name_proc_HLRALreadout[] = "HLRALreadout";

int ver_proc_HLRALreadout = 1;
/*****************************************************************************/
plerrcode
proc_HLRALdummyreadout(ems_u32* p)
{
        struct hlral* dev;
	ems_u32 *res;

        dev=get_pcihl_device(p[1]);
	res = hlral_readout(dev, outptr);

	if (res) {
		/* outptr = res; */ /* leave outptr untouched */
	} else
		return (plErr_HW);
	return (plOK);
}

plerrcode
test_proc_HLRALdummyreadout(ems_u32* p)
{
        plerrcode pres;
	if (p[0] != 1)
		return (plErr_ArgNum);
        if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
            return pres;
	return (plOK);
}

char name_proc_HLRALdummyreadout[] = "HLRALdummyreadout";

int ver_proc_HLRALdummyreadout = 1;
/*****************************************************************************/
/*
 * p[0]: length (==2)
 * p[1]: board
 * p[2]: mode 0: norm 1: test
 */
plerrcode
proc_HLRALstartreadout(ems_u32* p)
{
        struct hlral* dev;

        dev=get_pcihl_device(p[1]);
	hlral_startreadout(dev, p[2]);
	return (plOK);
}

plerrcode
test_proc_HLRALstartreadout(ems_u32* p)
{
        plerrcode pres;
	if (p[0] != 2)
		return (plErr_ArgNum);
        if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
            return pres;
	return (plOK);
}

char name_proc_HLRALstartreadout[] = "HLRALstartreadout";

int ver_proc_HLRALstartreadout = 1;
/*****************************************************************************/
/*
 * p[0]: length (==1)
 * p[1]: board
 */
plerrcode
proc_HLRALreset(ems_u32* p)
{
        struct hlral* dev;

        dev=get_pcihl_device(p[1]);
	hlral_real_reset(dev);
	return plOK;
}

plerrcode
test_proc_HLRALreset(ems_u32* p)
{
        plerrcode pres;
	if (p[0] != 1)
		return plErr_ArgNum;
        if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
            return pres;
	return plOK;
}

char name_proc_HLRALreset[] = "HLRALreset";

int ver_proc_HLRALreset = 1;
/*****************************************************************************/
/*****************************************************************************/
