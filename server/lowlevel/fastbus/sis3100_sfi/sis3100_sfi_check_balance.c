static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_check_balance.c,v 1.2 2011/04/06 20:30:23 wuestner Exp $";

#include <rcs_ids.h>

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

#ifdef SIS3100_SFI_MAPPED
void SIS3100_CHECK_BALANCE(struct fastbus_dev* dev, const char* caller)
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
/*****************************************************************************/
