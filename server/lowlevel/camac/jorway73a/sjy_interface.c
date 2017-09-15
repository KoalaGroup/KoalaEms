/* ****************************************************************************
** FILE sjy_interface
**
** DESCRIPTION
**  User level code Interface between the CAMAC IEEE layer and Linux generic
**  scsi driver sg.c.
**
**
** DEVELOPERS
**  Dave Slimmer
**  Fermilab Online Systems Group
**  Batavia, Il 60510, U.S.A.
**
**
** MODIFCATIONS
**  Date    Initials Description
**  22sep97 das      derived from example to demonstrate the generic SCSI
**                   interface
**  19feb98 das      zero output buffer if command fails
**
**  ===========================================================================
*/
/*#define DEBUG*/

  #include <stdio.h>
  #include <unistd.h>
  #include <string.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <signal.h>
  /*#include <scsi/sg.h>*/
  #include "/usr/src/linux/include/scsi/sg.h"
  #include "ieee_fun_types.h"


  #define SCSI_OFF sizeof(struct sg_header)
  #define USER_BUF_SIZE (SG_BIG_BUFF - (SCSI_OFF + 18 + 511)) 
  static unsigned char cmd[SCSI_OFF + 18 + USER_BUF_SIZE]; /* SCSI command buffer */

/*
** handle_scsi_cmd - process a scsi command using the generic scsi interface
**
** returns
**  success  int status - number of bytes read minus the sizeof(sg_header)
**  fail     CAM_S_INVLD_ARG -  bad function argument
**           CAM_S_GETSEM_FAIL - failed to get semaphore
**           CAM_S_DEV_IO_ERROR - scsi write or read returned bad status 
**           CAM_S_PUTSEM_FAIL - failed to release semaphore
*/


static int handle_scsi_cmd(       int branch,          /* CAMAC branch */
			     unsigned cmd_len,         /* command length */
                             unsigned in_size,         /* input data size */
                             unsigned char *i_buff,    /* input buffer */
                                  int out_size,        /* output data size */
                             unsigned char *o_buff     /* output buffer */
                             )
  {
      int status = 0;
      struct sg_header *sg_hd;
      sigset_t newmask,oldmask;
      int i;
      int fd = sjy_branch[branch].fd;


      /* safety checks */
      if (!cmd_len) return (CAM_S_INVLD_ARG);           /* need a cmd_len != 0 */
      if (!i_buff) return (CAM_S_INVLD_ARG);            /* need an input buffer != NULL */
  #ifdef SG_BIG_BUFF
      if (SCSI_OFF + cmd_len + in_size > SG_BIG_BUFF) return -1;
      if (SCSI_OFF + out_size > SG_BIG_BUFF) return -1;
  #else
      if (SCSI_OFF + cmd_len + in_size > 4096) return -1;
      if (SCSI_OFF + out_size > 4096) return -1;
  #endif

      if (!o_buff) out_size = 0;

      /* generic scsi device header construction */
      sg_hd = (struct sg_header *) i_buff;
      sg_hd->reply_len   = SCSI_OFF + out_size;
      sg_hd->twelve_byte = cmd_len == 12;
      sg_hd->result = 0;
  #if     0
      sg_hd->pack_len    = SCSI_OFF + cmd_len + in_size; /* not necessary */
      sg_hd->pack_id;     /* not used */
      sg_hd->other_flags; /* not used */
  #endif

      /* protect the write/read from being interrupted by ^C */
      sigemptyset(&newmask);
      sigaddset(&newmask, SIGINT);
      /*  block SIGINT and save current signal mask */
      if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
	printf("sjy_interface: SIG_BLOCK error");


      /* protect the write/read section for multi-user */
      if(sjy_getsem() != 0){
	printf("sjy_interface: getsem failed");
	sjy_putsem();
	return(CAM_S_GETSEM_FAIL);
      }

      /* send command */
      status = write( fd, i_buff, SCSI_OFF + cmd_len + in_size );

      if ( status < 0 ) {
        sjy_putsem();
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
        return (CAM_S_DEV_IO_ERROR);
      }

#ifdef DEBUG
      if ( status < 0 || status != SCSI_OFF + cmd_len + in_size ||
                         sg_hd->result ) {
          /* some error happened */
          printf("write(generic) result = 0x%x cmd = 0x%x\n",
                      sg_hd->result, i_buff[SCSI_OFF] );
          if (status < 0) perror("sjy_interface");
          sjy_putsem();
          return (CAM_S_DEV_IO_ERROR);
      }
#endif
      

      /* Since the sg.c driver _read_ always returns the sg_header, we must
      ** provide a buffer +SCSI_OFF in size. Since i_buff was already
      ** allocated as the largest size data buffer we could use (+SCSI_OFF),
      ** we use it. We have to copy the data read back to the user's 
      ** buffer, but this ultimatly saves an extra malloc call.
      */

      /* retrieve result */
      status = read( fd, i_buff, SCSI_OFF + out_size);

      /* return the semaphore */
      if(sjy_putsem() != 0){
	printf("sjy_interface: getsem failed");
	return(CAM_S_PUTSEM_FAIL);
      }

      /* reset signal mask which unblocks SIGINT */
      if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
	printf("sjy_interface: SIG_SETMASK error");


      /* copy data read back to the user buffer */
      /* The generic scsi driver keeps an internal buffer with the
      ** last data read, and does not clear it when a subsequent SCSI
      ** command completes with a non-zero SCSI response. sg_hd->result
      ** does not contain the SCSI response as observed from a SCSI
      ** analyzer, so it is not used. The only alternative is to test
      ** for a non-zero sense buffer sense key field returned from the
      ** Jorway. If the sense key is non-zero, the user buffer is
      ** zeroed instead of returning stale data from the internal SCSI
      ** buffer.
      */
      if (!(sg_hd->sense_buffer[SENSEKEY])) {
	if (o_buff) 
	  memcpy(o_buff, i_buff+SCSI_OFF, out_size);
      } else {
	bzero(o_buff, out_size);
      }

      /* save scsi status and sense buffer */
      sjy_branch[branch].result = sg_hd->result;
      memcpy(sjy_branch[branch].sense_buffer, sg_hd->sense_buffer, SJY_SENSE_LENGTH);

      /* Unlike the Jorway 73A, the 411S returns SCSI status CONDITION_MET
      ** (0x04) for Q=1 on a data transfer. This is converted by the 
      ** generic SCSI driver sg.c to EIO (0x05). The CAMAC completion routines
      ** ignore this status, but the following code segment does not.
      */
#ifdef DEBUG 
      if ( status < 0 || status != SCSI_OFF + out_size || sg_hd->result ) {
          /* some error happened */
          printf("read(generic) result = 0x%x cmd = 0x%x\n",
                  sg_hd->result, i_buff[SCSI_OFF] );
          printf("read(generic) sense "
                  "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
                  sg_hd->sense_buffer[0],         sg_hd->sense_buffer[1],
                  sg_hd->sense_buffer[2],         sg_hd->sense_buffer[3],
                  sg_hd->sense_buffer[4],         sg_hd->sense_buffer[5],
                  sg_hd->sense_buffer[6],         sg_hd->sense_buffer[7],
                  sg_hd->sense_buffer[8],         sg_hd->sense_buffer[9],
                  sg_hd->sense_buffer[10],        sg_hd->sense_buffer[11],
                  sg_hd->sense_buffer[12],        sg_hd->sense_buffer[13],
                  sg_hd->sense_buffer[14],        sg_hd->sense_buffer[15]);
          if (status < 0)
	    perror("sjy_interface");
          return (CAM_S_DEV_IO_ERROR);
      }
#endif
      return(status-SCSI_OFF);
  }

/*
** sjy_nondata - called by CAMAC IEEE routines. Sets up scsi command block non-data commands.
**
** returns
**  int status - return status from sjy_interface
**
*/

  int sjy_nondata( int branch, int camCmd)
  {
     static unsigned char scmdblk [6];
     static unsigned char lcmdblk [12];   /* required for commands 0xc0-0xff */
     static unsigned char *cmdblk;
     int cmdsize;
     int status;

#ifdef SJY411S

     cmdblk = lcmdblk;
     cmdsize = 12;
     cmdblk [0] = SJY_NONDATA_CMD;        /* Op code */
     cmdblk [1] = (camCmd>>24) & 0x1F;    /* LUN/function */
     cmdblk [2] = (camCmd>>16) & 0x1F;    /* station */
     cmdblk [3] = (camCmd>> 8) & 0x0F;    /* address */
     cmdblk [4] = (camCmd)     & 0xFF;    /* S/P / crate */
     cmdblk [5] = 0;                      /* control byte, always 0 */
  

#endif

#ifdef SJY73A

     cmdblk = scmdblk;
     cmdsize = 6;
     cmdblk [0] = SJY_NONDATA_CMD;         /* Op code */
     cmdblk [1] = (camCmd>>24) & 0x1F;     /* lun/function code */
     cmdblk [2] = (camCmd>>16) & 0xFF;     /* mode/station */
     cmdblk [3] = (camCmd>>8)  & 0x0F;     /* address */
     cmdblk [4] = 0;                       /* allocation length */
     cmdblk [5] = 0;                       /* control byte, always 0 */

#endif

    /* copy cmdblk behind sg_header */
    /*
     * +------------------+
     * | struct sg_header | <- cmd
     * +------------------+
     * | copy of cmdblk   | <- cmd + SCSI_OFF
     * +------------------+
     */

    memcpy( cmd + SCSI_OFF, cmdblk, cmdsize );

    status = handle_scsi_cmd(branch, cmdsize, 0, cmd, 0, NULL); 

    sjy_notify_camac_nondata(branch, 0, status);
    return (status);
  }



/*
** sjy_write - called by CAMAC IEEE routines. Sets up scsi command block for writes
**             and non-data commands.
**
** returns
**  int status - return status from sjy_interface
**
*/

  int sjy_write( int branch, int camCmd, void *dataBuffer, int xferlen )
  {
     static unsigned char scmdblk [6];
     static unsigned char lcmdblk [10];
     static unsigned char *cmdblk;
     int cmdsize;
     int status;


#ifdef SJY411S
     if (xferlen) {

       cmdblk = lcmdblk;
       cmdblk [0] = SJY_DATA_CMD;           /* Op code */
       cmdblk [1] = 0;                      /* LUN/reserved */
       cmdblk [2] = (camCmd>>24) & 0x3F;    /* list flag/BS/function code */
       cmdblk [3] = (camCmd>>16) & 0xFF;    /* mode/station */
       cmdblk [4] = (camCmd>>8)  & 0x3F;    /* disable disc./SCS/SNXC/address*/
       cmdblk [5] =  camCmd      & 0xFF;    /* S/P /crate */
       cmdblk [6] = (xferlen>>16) & 0xFF;   /* transfer length (MSB) */
       cmdblk [7] = (xferlen>>8)  & 0xFF;
       cmdblk [8] =  xferlen      & 0xFF;   /* transfer length (LSB) */
       cmdblk [9] = 0;                      /* control byte, always 0 */

     } else {

       cmdblk = scmdblk;
       cmdblk [0] = SJY_NONDATA_CMD;        /* Op code */
       cmdblk [1] = (camCmd>>24) & 0x1F;    /* LUN/function */
       cmdblk [2] = (camCmd>>24) & 0x1F;    /* station */
       cmdblk [3] = (camCmd>>16) & 0x0F;    /* address */
       cmdblk [4] = (camCmd>>8)  & 0xFF;    /* S/P / crate */
       cmdblk [5] = 0;                      /* control byte, always 0 */
  
     }
#endif

#ifdef SJY73A
     if (xferlen < 256) { /* This is also the nondata command block */

     cmdblk = scmdblk;
     cmdblk [0] = SJY_SHORTXFER_DATA_CMD;  /* Op code */
     cmdblk [1] = (camCmd>>24) & 0x1F;     /* lun/function code */
     cmdblk [2] = (camCmd>>16) & 0xFF;     /* mode/station */
     cmdblk [3] = (camCmd>>8)  & 0x0F;     /* address */
     cmdblk [4] = xferlen;                 /* allocation length */
     cmdblk [5] = 0;                       /* control byte, always 0 */


    } else {

     cmdblk = lcmdblk;
     cmdblk [0] = SJY_LONGXFER_DATA_CMD;   /* Op code */
     cmdblk [1] = 0;                       /* lun/reserved */
     cmdblk [2] = (camCmd>>24) & 0x1F;     /* reserved/function code */
     cmdblk [3] = (camCmd>>16) & 0xFF;     /* mode/S/station */
     cmdblk [4] = (camCmd>>8)  & 0x0F;     /* address */
     cmdblk [5] = 0;                       /* reserved */
     cmdblk [6] = (xferlen>>16) & 0xFF;    /* transfer length (MSB) */
     cmdblk [7] = (xferlen>>8)  & 0xFF;
     cmdblk [8] = xferlen       & 0xFF;    /* transfer length (LSB) */
     cmdblk [9] = 0;                       /* control byte, always 0 */ 
 
    }
#endif

    /* copy cmdblk behind sg_header */
    /*
     * +------------------+
     * | struct sg_header | <- cmd
     * +------------------+
     * | copy of cmdblk   | <- cmd + SCSI_OFF
     * +------------------+
     */

    if (cmdblk[0] == SJY_SHORTXFER_DATA_CMD)
      cmdsize = 6;
    else
      cmdsize = 10;

    memcpy( cmd + SCSI_OFF, cmdblk, cmdsize );

    /* copy user buffer behind cmdblk */
    if (dataBuffer != NULL)
      memcpy( cmd + SCSI_OFF + cmdsize, dataBuffer, xferlen );

    status = handle_scsi_cmd(branch, cmdsize, xferlen, cmd, 0, NULL);

    sjy_notify_camac_data(branch, xferlen, status);
    return(status);
  }


/*
** sjy_read - called by CAMAC IEEE routines. Sets up scsi command block for reads.
**
** returns
**  int status - return status from sjy_interface
**
*/

  int sjy_read( int branch, int camCmd, void *dataBuffer, int xferlen )
  {
     static unsigned char scmdblk [6];
     static unsigned char lcmdblk [10];
     static unsigned char *cmdblk;
     int cmdsize;
     int status;


#ifdef SJY411S
     if (xferlen) {

       cmdblk = lcmdblk;
       cmdblk [0] = SJY_DATA_CMD;           /* Op code */
       cmdblk [1] = 0;                      /* LUN/reserved */
       cmdblk [2] = (camCmd>>24) & 0x3F;    /* list flag/BS/function code */
       cmdblk [3] = (camCmd>>16) & 0xFF;    /* mode/station */
       cmdblk [4] = (camCmd>>8)  & 0x3F;    /* disable disc./SCS/SNXC/address*/
       cmdblk [5] =  camCmd      & 0xFF;    /* S/P /crate */
       cmdblk [6] = (xferlen>>16) & 0xFF;   /* transfer length (MSB) */
       cmdblk [7] = (xferlen>>8)  & 0xFF;
       cmdblk [8] =  xferlen      & 0xFF;   /* transfer length (LSB) */
       cmdblk [9] = 0;                      /* control byte, always 0 */

     } else {

       cmdblk = scmdblk;
       cmdblk [0] = SJY_NONDATA_CMD;        /* Op code */
       cmdblk [1] = (camCmd>>24) & 0x1F;    /* LUN/function */
       cmdblk [2] = (camCmd>>24) & 0x1F;    /* station */
       cmdblk [3] = (camCmd>>16) & 0x0F;    /* address */
       cmdblk [4] = (camCmd>>8)  & 0xFF;    /* S/P / crate */
       cmdblk [5] = 0;                      /* control byte, always 0 */
  
     }
#endif

#ifdef SJY73A
    if (xferlen < 256) {

     cmdblk = scmdblk;
     cmdblk [0] = SJY_SHORTXFER_DATA_CMD;   /* Op code */
     cmdblk [1] = (camCmd>>24) & 0x1F;      /* lun/function code */
     cmdblk [2] = (camCmd>>16) & 0xFF;      /* mode/station */
     cmdblk [3] = (camCmd>>8)  & 0x0F;      /* address */
     cmdblk [4] = xferlen;                  /* allocation length */
     cmdblk [5] = 0;                        /* control byte, always 0 */


    } else {

     cmdblk = lcmdblk;
     cmdblk [0] = SJY_LONGXFER_DATA_CMD;    /* Op code */
     cmdblk [1] = 0;                        /* lun/reserved */
     cmdblk [2] = (camCmd>>24) & 0x1F;      /* reserved/function code */
     cmdblk [3] = (camCmd>>16) & 0xFF;      /* mode/S/station */
     cmdblk [4] = (camCmd>>8)  & 0x0F;      /* address */
     cmdblk [5] = 0;                        /* reserved */
     cmdblk [6] = (xferlen>>16) & 0xFF;     /* transfer length (MSB) */
     cmdblk [7] = (xferlen>>8)  & 0xFF;
     cmdblk [8] = xferlen       & 0xFF;     /* transfer length (LSB) */
     cmdblk [9] = 0;                        /* control byte, always 0 */ 
 

    }
#endif


    /* copy cmdblk behind sg_header */
    /*
     * +------------------+
     * | struct sg_header | <- cmd
     * +------------------+
     * | copy of cmdblk   | <- cmd + SCSI_OFF
     * +------------------+
     */


    if (cmdblk[0] == SJY_SHORTXFER_DATA_CMD)
      cmdsize = 6;
    else
      cmdsize = 10;

    memcpy( cmd + SCSI_OFF, cmdblk, cmdsize );

    status = handle_scsi_cmd(branch, cmdsize, 0, cmd, xferlen, dataBuffer);

    sjy_notify_camac_data(branch, xferlen, status);
    return(status);
  }


/*
** sjy_inquiry - request information about the JORWAY
**
** returns
**  success - pointer to a static char buffer containing inquiry data
**  fail - pointer to an empty static char buffer
**
*/

  /* request vendor brand and model */
  unsigned char *sjy_inquiry ( int branch )
  {
    static unsigned char Inqbuffer[ SCSI_OFF + INQUIRY_REPLY_LEN ];
    int i;
    char *cptr;


    unsigned char cmdblk [ INQUIRY_CMDLEN ] =
        { INQUIRY_CMD,  /* command */
                    0,  /* lun/reserved */
                    0,  /* page code */
                    0,  /* reserved */
    INQUIRY_REPLY_LEN,  /* allocation length */
                    0 };/* reserved/flag/link */

    memcpy( cmd + SCSI_OFF, cmdblk, sizeof(cmdblk) );

    /*
     * +------------------+
     * | struct sg_header | <- cmd
     * +------------------+
     * | copy of cmdblk   | <- cmd + SCSI_OFF
     * +------------------+
     */

    /* clear the Inquiry buffer */
    /*memset( Inqbuffer, '\0', sizeof(Inqbuffer) );*/

    handle_scsi_cmd(branch, sizeof(cmdblk), 0, cmd,
                        sizeof(Inqbuffer) - SCSI_OFF, Inqbuffer);

    /* handle_scsi_cmd removes sg_header in the copy from i_buff to o_buff,
    ** so don't add SCSI_OFF offset to Inqbuffer
    */
    /*
    printf("Inquiry buffer\n");
    printf("%s\n",Inqbuffer+INQUIRY_VENDOR);
    */  
    return (Inqbuffer);
  }


/*
** sjy_tur - sends Test Unit Ready command
**
** returns
**  int status - return status from sjy_interface 
*/

  int sjy_tur ( int branch )
  {
    int i;
    int status;


    /* request READY status */
    static unsigned char cmdblk [TESTUNITREADY_CMDLEN] = {
        TESTUNITREADY_CMD, /* command */
                        0, /* lun/reserved */
                        0, /* reserved */
                        0, /* reserved */
                        0, /* reserved */
                        0};/* reserved */

    memcpy( cmd + SCSI_OFF, cmdblk, sizeof(cmdblk) );

    /*
     * +------------------+
     * | struct sg_header | <- cmd
     * +------------------+
     * | copy of cmdblk   | <- cmd + SCSI_OFF
     * +------------------+
     */

    /* This must be done twice to get good status from power cycled Jorway */
    for (i=0; i<2; i++) {
      status = handle_scsi_cmd(branch, sizeof(cmdblk), 0, cmd, 0, NULL);
    }
    if (status) perror("sjy_tur");

    return (status);
  }



/*
** sjy_reqSense - REQUEST_SENSE from the JORWAY. Normally issued after a CHECK
**                CONDITION status is returned by any command.
**
** returns
**  success - pointer to a static char buffer containing sense data
**  fail - pointer to an empty static char buffer
**
*/

  unsigned char *sjy_reqSense ( int branch )
  {
    static unsigned char sensebuffer[ SCSI_OFF + SJY_SENSE_LENGTH ];

    unsigned char cmdblk [ SENSE_CMDLEN ] =
          { SENSE_CMD,  /* command */
                    0,  /* lun/reserved */
                    0,  /* reserved */
                    0,  /* reserved */
     SJY_SENSE_LENGTH,  /* allocation length */
                    0 };/* reserved/flag/link */

    memcpy( cmd + SCSI_OFF, cmdblk, sizeof(cmdblk) );

    /*
     * +------------------+
     * | struct sg_header | <- cmd
     * +------------------+
     * | copy of cmdblk   | <- cmd + SCSI_OFF
     * +------------------+
     */

    /* clear the sense buffer */
    memset( sensebuffer, '\0', sizeof(sensebuffer) );

    handle_scsi_cmd(sjy_branch[branch].fd, sizeof(cmdblk), 0, cmd,
                        sizeof(sensebuffer) - SCSI_OFF, sensebuffer);

    /* handle_scsi_cmd removes sg_header in the copy from i_buff to o_buff,
    ** so don't add SCSI_OFF offset to sensebuffer
    */
    return (sensebuffer);
  }


/*
** sjy_notify_camac_data - SCSI command completion notification routine
**                         for CAMAC data transfers
**
** returns
**  int result - SCSI command completion result
*/

 int sjy_notify_camac_data( int branch,      /* CAMAC branch */
			    int xferlen,     /* expected transfer length */
                            int status)      /* actual transfer length */ 
 {
   int sense_key = sjy_branch[branch].sense_buffer[SENSEKEY];
   int add_sense = sjy_branch[branch].sense_buffer[ADD_SENSECODE];

 
   sjy_branch[branch].errn = 0;

   if (sense_key) {
     if (add_sense == SJY_NO_Q) {
       /* SCSI ok, CAMAC ok, no Q on transfer */
       sjy_branch[branch].errn = ENODEV;
     } else if (add_sense == SJY_NO_X) {
       /* SCSI ok, no X on transfer */
       sjy_branch[branch].errn = EFAULT;
     } else {
       /* SCSI problem */
       sjy_branch[branch].errn = EIO;
     }
   } else {
     if (status == xferlen) {
       /* SCSI ok, CAMAC ok, assuming Q on transfer */
       sjy_branch[branch].errn = 0;
     }
   }

   return sjy_branch[branch].result;
 }


/*
** sjy_notify_camac_nondata - SCSI command completion notification routine
**                            for CAMAC nondata transfers
**
** returns
**  int result - SCSI command completion result
*/

 int sjy_notify_camac_nondata( int branch,      /* CAMAC branch */
			       int xferlen,     /* expected transfer length */
                               int status)      /* actual transfer length */ 
 {
   int sense_key = sjy_branch[branch].sense_buffer[SENSEKEY];
   int add_sense = sjy_branch[branch].sense_buffer[ADD_SENSECODE];

   sjy_branch[branch].errn = 0; 

   if ((sjy_branch[branch].result == SJY_GOOD) && !(sense_key)) {
     /* SCSI ok. CAMAC ok, no Q on transfer */
     sjy_branch[branch].errn = ENODEV;

   } else if ((sjy_branch[branch].result == SJY_CHECK_CONDITION) || (sense_key)) {
     if (add_sense == SJY_NO_Q) {
       /* SCSI ok, CAMAC ok, no Q on transfer */
       sjy_branch[branch].errn = ENODEV;
     } else if (add_sense == SJY_NO_X) {
       /* SCSI ok, no X on transfer */
       sjy_branch[branch].errn = EFAULT;
     } else {
       /* SCSI problem */
       sjy_branch[branch].errn = EIO;
     }
   } else {
     /* SCSI ok, CAMAC ok, assuming Q on transfer */
     sjy_branch[branch].errn = 0;
   }

#if 0
   if (sjy_branch[branch].result == SJY_CHECK_CONDITION) {
     add_sense = sjy_branch[branch].sense_buffer[12];
     if (add_sense == SJY_NO_Q) {
       /* SCSI ok, CAMAC ok, no Q on transfer */
       sjy_branch[branch].errn = ENODEV;
     } else if (add_sense == SJY_NO_X) {
       /* SCSI ok, no X on transfer */
       sjy_branch[branch].errn = EFAULT;
     } else {
       /* SCSI problem */
       sjy_branch[branch].errn = EIO;
     }
   } else if (sjy_branch[branch].result == SJY_GOOD) {
     /* SCSI ok. CAMAC ok, no Q on transfer */
     sjy_branch[branch].errn = ENODEV;

   } else if (sjy_branch[branch].result == SJY_CONDITION_MET) {
     /* SCSI ok. CAMAC ok, Q on transfer */
     sjy_branch[branch].errn = 0;
   }
#endif
   return sjy_branch[branch].result;
 }











