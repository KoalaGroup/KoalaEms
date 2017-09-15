/*
 * procs/camac/fera/FERAsetup.c
 * created before 28.09.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: FERAsetup.c,v 1.16 2011/04/06 20:30:30 wuestner Exp $";

/* FERA-System fuer Readout initialisieren,
  Daten aus einem Setup-Feld verwenden */
/* Parameter: FSCtimeout, -delay, Single-Event-Flag, Pedestrals */

#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/domain/dom_ml.h"
#include "fera.h"

RCS_REGISTER(cvsid, "procs/camac/fera")

/*****************************************************************************/

ems_u32* SetupFERAmodul(ml_entry* n, ems_u32* data, int* vsn)
{
    struct camac_dev* dev=n->address.camac.dev;
    int N=n->address.camac.slot;
    ems_u32 qx;
    int j;

    dev->CAMACcntl(dev, dev->CAMACaddrP(N, 0, 9), &qx);

    switch (n->modultype) {
        case FERA_ADC_4300B:
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 16),
		       FERA_ADC_readout_stat + ((*vsn)++));
            for (j = 0; j < 16; j++)
		    dev->CAMACwrite(dev, dev->CAMACaddrP(N, j, 17), *data++);
            break;
        case FERA_TDC_4300B: /* wie oben, nur mit Overflow-Supression */
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 16),
		       FERA_TDC_readout_stat + ((*vsn)++));
            for (j = 0; j < 16; j++)
		    dev->CAMACwrite(dev, dev->CAMACaddrP(N, j, 17), *data++);
            break;
        case SILENA_ADC_4418V:
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, 14, 20),
		       SILENA_readout_stat + ((*vsn)++));
            dev->CAMACwrite(dev, dev->CAMACaddrP(N ,9 ,20), *data++);
            for (j = 8; j < 16; j++)
		    dev->CAMACwrite(dev, dev->CAMACaddrP(N, j, 17), *data++);
            for (j = 0; j < 8; j++)
		    dev->CAMACwrite(dev, dev->CAMACaddrP(N, j, 17), *data++);
            for (j = 0; j < 8; j++)
		    dev->CAMACwrite(dev, dev->CAMACaddrP(N, j, 20), *data++);
            break;
        case BIT_PATTERN_UNIT:
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 17),
		       0x400 + ((*vsn)++));
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, 1, 17), *data++);
            break;
        case CNRS_QDC_1612F:
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 16),
		       CNRS_QDC_readout_stat + ((*data++) << 9));
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, 1, 16), (*vsn)++);
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, 2, 16), (*vsn)++);
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, 3, 16), *data++);
            for (j = 0; j < 16; j++)
		    dev->CAMACwrite(dev, dev->CAMACaddrP(N, j, 17), *data++);
            break;
        case CNRS_TDC_812F:
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 16),
		       CNRS_TDC_readout_stat + ((*vsn)++));
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, 1, 16), *data++);
            for (j = 0; j < 8; j++)
		    dev->CAMACwrite(dev, dev->CAMACaddrP(N, j, 17), *data++);
            break;
	case RAL11X_RDOUT:
	    dev->CAMACwrite(dev, dev->CAMACaddrP(N, 2, 16), (*vsn)++);
	    dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 17), *data++ | 2); /* FERA enable */
	    dev->CAMACwrite(dev, dev->CAMACaddrP(N, 1, 17), 0);
	    dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 16), 0);
	    dev->CAMACcntl(dev, dev->CAMACaddrP(N, 0, 25), &qx);
	    break;
    }
    return(data);
}

/*---------------------------------------------------------------------------*/

plerrcode test_SetupFERA(ml_entry* n, ems_u32** data)
{
	int datause;

	switch (n->modultype) {
	    case FERA_ADC_4300B:
	    case FERA_TDC_4300B:
		datause = 16;
		break;
	    case SILENA_ADC_4418V:
		datause = 25;
		break;
	    case BIT_PATTERN_UNIT:
		datause = 1;
		break;
	    case CNRS_QDC_1612F:
		datause = 18;
		break;
	    case CNRS_TDC_812F:
		datause = 9;
		break;
	    case RAL11X_RDOUT:
		datause = 1;
		break;
	    default:
		/* return(plErr_BadModTyp); */
                printf("SetupFERA: Module 0x%08x (Slot %d) is not known\n",
                    n->modultype, n->address.camac.slot);
                datause = 0;
	}
	*data += datause;
	return(plOK);
}
