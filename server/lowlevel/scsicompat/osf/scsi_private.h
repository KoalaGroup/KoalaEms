#ifndef _scsi_private_h_
#define _scsi_private_h_
typedef struct priv_read_position_cdb10 {
	u_char	opcode;		/* 0x34					*/
	u_char	bt	:1,	/* block type                           */
		res	:4,	/* 4 Bits reserved                      */
		lun	:3;	/* Logical unit number			*/
	u_char  res_2     ;	/* reserved                             */
	u_char  res_3     ;	/* reserved                             */
	u_char  res_4     ;	/* reserved                             */
	u_char  res_5     ;	/* reserved                             */
	u_char  res_6     ;	/* reserved                             */
	u_char  res_7     ;	/* reserved                             */
	u_char  res_8     ;	/* reserved                             */
	u_char	control;	/* Control byte				*/
}PRIV_READ_POSITION_CDB10;
#endif
