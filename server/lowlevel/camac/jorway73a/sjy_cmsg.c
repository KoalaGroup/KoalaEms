/***************************************************************************
**	SUBROUTINE CMSG
**
** DESCRIPTION
**	Subroutine to decode Sense Data information
**
** DEVELOPERS
**      J.Streets D.Slimmer
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
** MODIFICATIONS
**         Date       Initials  Description
**      21-Aug-1991	MVW     Creation
**      24-Sep-1992     EGS     Modified for MVME147/167 and VxWorks
**	25-Oct-1993	DAS	Modified for VSD2992
**	21-Sep-1994	jms	JY411S, convert from vsd_d_csr
**      05-Aug-1997     das     modified for Linux
**      19-Feb-1998     das     correct dma byte count for Jor73A
**
** --------------------------------------------------------------- 
*/

#include "ieee_fun_types.h"


unsigned int cmsg (char* subroutine, int data, int branch)
{
  int	i,status;
  unsigned char *buf;

/* ------------------------------------------------------------------------- */

  printf("\nCMSG called from %s, data=%d\n",subroutine,data);

  buf = sjy_branch[branch].sense_buffer;
  if(strlen(buf) == 0)
	return(CAM_S_INVLD_BRANCH);

  printf("SCSI Sense Buffer:");
  for(i=0;i<SJY_SENSE_LENGTH;i++)
    printf(" %02X",buf[i]);

  printf("\nSense Key=0x%02x: ",buf[2]);
  switch(buf[2]) {
    case 0x0: printf("NO SENSE - available, last CAMAC operation OK\n");
    break;
    case 0x2: printf("NOT READY - Controller was offline\n");  /* 73A only */
    break;
    case 0x4: printf("HARDWARE ERROR - Parity error or No X\n");
    break;
    case 0x5: printf("ILLEGAL REQUEST - A non-zero logical unit, command not implemented or contained an improper bit\n");
    break;
    case 0x6: printf("UNIT ATTENTION - The Controller has been reset\n");
    break;
    case 0x9: printf("SHORT TRANSFER - A stop-mode transfer did not transfer the expected number of byutes because a CAMAC cycle failed to return Q\n");
    break;
    case 0xB: printf("ABORTED COMMAND - JY411S aborted the command, perhaps host detected an error\n");
    break;
    default: printf("CMSG Error, received undocumented Sense Key\n");
    break;
    }

  if(buf[2] != 0){

#ifdef SJY411S
    printf("\nWord Count Deficit = %d\n",buf[6]+256*buf[5]+65536*buf[4]);

    printf("\nMain Status Register=0x%02x: \n",buf[8]);
    if(buf[8]&128) printf("Q failure\n");
    if(buf[8]&64)  printf("X failure\n");
    if(buf[8]&32)  printf("Timeout\n");
    if(buf[8]&16)  printf("Crate Overflow\n");
    if(buf[8]&8)   printf("Serial No Execution\n");
    if(buf[8]&4)   printf("Serial Demand Sequencer Error\n");
    if(buf[8]&2)   printf("Serial Demand Stack not empty\n");
    if(buf[8]&1)   printf("Branch Demand Set\n");

    if(sjy_branch[branch].serial){
      printf("\nSerial Status Register=0x%02x%02x: \n",buf[9],buf[10]);
      if(buf[10]&128) printf("SNEX   Serial No Execution\n");
      if(buf[10]&64)  printf("RNRG1  Reply not recognised with length > 1\n");
      if(buf[10]&32)  printf("RNRE1  Reply not recognised with length = 1\n");
      if(buf[10]&16)  printf("CMDFOR Corrupt Command Format\n");
      if(buf[10]&8)   printf("HDRREC No Truncated Command received\n");
      if(buf[10]&4)   printf("RPE    Parity Error Detected in Reply\n");
      if(buf[10]&2)   printf("TIMOS  Serial Time Out\n");
      if(buf[10]&1)   printf("CRET   Command Type Message Detected at receiver\n");
      if(buf[9]&128)  printf("DERR   Delayed Error Bit in Reply\n");
      if(buf[9]&64)   printf("RERR   Error Bit in Reply\n");
      if(buf[9]&32)   printf("LOSYN  Sync lost while driver is in a transmit awaiting reply cycle\n");
      if(buf[9]&16)   printf("SYNC   Receiver has lost byte sync (dynamic)\n");
      if(buf[9]&8)    printf("SPARE  Not used Normally 0\n");
      if(buf[9]&4)    printf("HNGD   Hung Demand Received\n");
      if(buf[9]&2)    printf("DALISQ Demand Alarm Interrupt sequencer\n");
      if(buf[9]&1)    printf("DALIE  Demand Alarm Interrupt Enable\n");
      }
#endif  /* 411S */
#ifdef SJY73A
      printf("FIFO status = %d\n", buf[3]);
      printf("DMA Byte Count remaining to be transferred = %d\n", 
	      buf[6]+256*buf[5]+65536*buf[4]);
#endif

    printf("\nAdditional Sense Code=0x%02x: ",buf[12]);
    switch(buf[12]) {
      case 0x00: printf("No additional sense information \n");
      break;
      case 0x04: printf("Controller off-line \n");   /* 73A only */
      break;
      case 0x20: printf("Invalid Command Operation Code\n");
      break;
      case 0x24: printf("Invalid field in CDB\n");
      break;
      case 0x25: printf("Logical Unit not supported\n");
      break;
      case 0x29: printf("Power on reset or Bus Device Reset occurred\n");
      break;
      case 0x3D: printf("Invalid bit in Identify Message\n");
      break;
      case 0x44: printf("CAMAC cycle did not return X=1\n");
      break;
      case 0x47: printf("SCSI Parity Error\n");
      break;
      case 0x48: printf("INITIATOR DETECTED ERROR message received\n");
      break;
      case 0x80: printf("CAMAC cycle did not return Q=1\n");
      break;
      default:   printf("CMSG Error, received undocumented Additional Sense Key\n");
      break;
      }

#ifdef SJY411S
    printf("\nCrate %d; Slot %d; Subaddress %d\n",buf[14]>>1,
          16*(buf[14]&1)+(buf[15]>>4), buf[15]&15);
#endif
    }

  buf = sjy_inquiry (branch);
  printf("Jorway Hardware Version is \"%s\"\n",&buf[8]);

  printf("Software Version is \"%s\"\n",SJY_VERSION);
  
  return (CAM_S_SUCCESS);
}
/* ========================================================================== */
/************************************************************************** */
/*	SUBROUTINE CMSG			


 * ========================================================================== */


void cmsg_ (char *subroutine, int* data, int* branch)

{
	ieee_status = cmsg (subroutine, *data, *branch);
}
/* --------------------------------------------------------------- */
