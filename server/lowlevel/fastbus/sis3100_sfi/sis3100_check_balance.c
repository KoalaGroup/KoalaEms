/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_check_balance.c
 * created: ?
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_check_balance.c,v 1.3 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"
#include "../../oscompat/oscompat.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

#ifdef SIS3100_SFI_MMAPPED
void sis3100_check_balance(struct fastbus_dev* dev, const char* caller)
{
    struct fastbus_sis3100_sfi_info* sfi_info=&dev->info.sis3100_sfi;
    ems_u32 stat, balance, error;

    balance=*sfi_info->p_balance;
    if (balance) {
        stat=*sfi_info->sr;
        error=*sfi_info->prot_error;
        printf("FEHLER(%s): balance=%d stat=%08x error=%x\n",
                caller, balance, stat, error);
        tsleep(5);
        balance=*sfi_info->p_balance;
        printf("AFTER 50 ms: %d\n", balance);
        *sfi_info->p_balance=0;
    }
}
#endif
