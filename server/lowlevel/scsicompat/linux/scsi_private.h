#ifndef _scsi_private_h_
#define _scsi_private_h_
/* ---------------------------------------------------------------------- */

/* 
 * Rewind command cdb 6 byte
 */
typedef struct seq_rewind_cdb6 {
	u_char	opcode;		/* 0x01					*/
	u_char	immed	:1,	/* Immediate bit */
	u_char		:4,	/* 4 Bits reserved			*/
		lun	:3;	/* Logical unit number			*/
	u_char		:8;	/* Reserved				*/
	u_char		:8;	/* Reserved				*/
	u_char		:8;	/* Reserved				*/
	u_char	control;	/* Control byte				*/
}SEQ_REWIND_CDB6;

/*
 * The mode select CDB 6 bytes.
 */
typedef struct all_mode_sel_cdb6 {
	u_char	opcode;		/* 15 hex 				*/
	u_char	sp	: 1,	/* Save parameters			*/
			: 3, 	/* 3 bits reserved			*/
		pf	: 1,	/* Page format-true conforms to scsi 2	*/
		lun	: 3;	/* logical unit number			*/
	u_char		: 8;	/* Reserved byte			*/
	u_char		: 8;	/* Reserved byte			*/
	u_char	param_len;	/* parameter list length		*/
	u_char	control;	/* The control byte			*/
}ALL_MODE_SEL_CDB6;

/* 
 * The mode parameter header, 6 byte cdb
 */
typedef struct seq_mode_head6 {
	u_char	mode_len;		/* The length of the mode head	*/
	u_char	medium_type;		/* The medium type		*/
	u_char	speed		:4,	/* Tape speed			*/
		buf_mode	:3,	/* Buffered mode setting	*/
		wp		:1;	/* Write protected		*/
	u_char	blk_desc_len;		/* Len of the descriptor block	*/
}SEQ_MODE_HEAD6;

/* 
 * The Block Descriptor for both 6 and 10 byte MODE PARAMETERS.
 */
typedef struct seq_mode_desc {
	u_char	density_code;	/* The density code Tapes		*/
	u_char	num_blocks2;	/* MSB of number of blocks 		*/
	u_char	num_blocks1;	/* Middle of number of blocks		*/
	u_char	num_blocks0;	/* LSB of number of blocks		*/
	u_char	reserved;	/* reserved				*/
	u_char	block_len2;	/* MSB of block length			*/
	u_char	block_len1;	/* Middle of block length		*/
	u_char	block_len0;	/* LSB of block length			*/
}SEQ_MODE_DESC;

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

/* 
 * Write command cdb 6 byte (tapes only support 6 byte Write cdb's)
 */
typedef struct seq_write_cdb6 {
	u_char	opcode;		/* 0x0a					*/
	u_char	fixed	:1,	/* Fixed block or variable length	*/
			:4,	/* 4 Bits reserved			*/
		lun	:3;	/* Logical unit number			*/
	u_char	trans_len2;	/* MSB of transfer length		*/
	u_char	trans_len1;	/* MID of transfer length		*/
	u_char	trans_len0;	/* LSB of transfer length		*/
	u_char	control;	/* Control byte				*/
}SEQ_WRITE_CDB6;

/* 
 * Write filemarks command cdb 6 byte
 */
typedef struct seq_writemarks_cdb6 {
	u_char	opcode;		/* 0x10					*/
	u_char	immed	:1,	/* Immediate bit			*/
		wsmk	:1,	/* Write setmarks 			*/
			:3,	/* 3 bits reserved			*/
		lun	:3;	/* Logical unit number			*/
	u_char	trans_len2;	/* MSB of number of marks		*/
	u_char	trans_len1;	/* MID of number of marks		*/
	u_char	trans_len0;	/* LSB of number of marks		*/
	u_char	control;	/* Control byte				*/
}SEQ_WRITEMARKS_CDB6;

/*
 * INQUIRY CDB 6 byte
 */
typedef struct all_inq_cdb {
	u_char	opcode;		/* OPcode = 0X12			*/
	u_char	evpd	:1,	/* Enable vital product data bit	*/
		cmddt	:1,	/* Command support data			*/
			:3,	/* 3 bits resevered			*/
		lun	:3;	/* Logical unit number			*/
	u_char	page;		/* Page code				*/
	u_char		:8;	/* Byte reserved			*/
	u_char  alloc_len;	/* Allocation length			*/
	u_char	control;	/* Control byte				*/
}ALL_INQ_CDB;

/* 
 * Load/unload command cdb 6 byte
 */
typedef struct seq_load_cdb6 {
	u_char	opcode;		/* 0x1b					*/
	u_char	immed	:1,	/* Return immediate			*/
			:4,	/* 3 bits reserved			*/
		lun	:3;	/* Logical unit number			*/
	u_char		:8;	/* Reserved				*/
	u_char		:8;	/* Reserved				*/
	u_char	load	:1,	/* Load tape/unload 1 = load		*/
		reten	:1,	/* Retension tape			*/
		eot	:1,	/* Unload at eot			*/
			:5;	/* 5 Bits reserved			*/
	u_char	control;	/* Control byte				*/
}SEQ_LOAD_CDB6;

/* 
 * Prevent allow medium removal 6 byte cdb
 */
typedef struct dir_prevent_cdb6 {
	u_char	opcode;		/* 0x1e					*/
	u_char		:5,	/* 5 bits reserved			*/
		lun	:3;	/* Logical unit number			*/
	u_char		:8;	/* reserved				*/
	u_char		:8;	/* reserved				*/
	u_char	prevent	:1,	/* Prevent/allow			*/
			:7;	/* 7 bits reserved			*/
	u_char	control;	/* the control byte			*/
}DIR_PREVENT_CDB6;

/* 
 * Erase command cdb 6 bytes
 */
typedef struct seq_erase_cdb6 {
	u_char	opcode;		/* 0x19					*/
	u_char	extend	:1,	/* Erase from position to end (long)	*/
		immed	:1,	/* Return immediate			*/
			:3,	/* 3 bits reserved			*/
		lun	:3;	/* Logical unit number			*/
	u_char		:8;	/* Reserved				*/
	u_char		:8;	/* Reserved				*/
	u_char		:8;	/* Reserved				*/
	u_char	control;	/* Control byte				*/
}SEQ_ERASE_CDB6;

/*
 * Space command cdb 6 byte
 */
typedef struct seq_space_cdb6 {
	u_char	opcode;		/* 0x11					*/
	u_char	code	:3,	/* Code record filemark ,etc		*/
	u_char		:2,	/* 2 Bits reserved			*/
		lun	:3;	/* Logical unit number			*/
	u_char	count2;		/* MSB of count (howmany) and direction */
	u_char	count1;		/* MID of count (howmany) and direction */
	u_char	count0;		/* LSB of count (howmany) and direction */
	u_char	control;	/* Control byte				*/
}SEQ_SPACE_CDB6;

/* 
 * Request sense 6 byte cdb
 */
typedef struct all_req_sense_cdb6 {
	u_char	opcode;		/* 0x03 hex 			[0]	*/
	u_char		: 5,	/* 5 bits reserved		[1]	*/
		lun	: 3;	/* logical unit number			*/
	u_char		: 8;	/* Reserved byte		[2]	*/
	u_char		: 8;	/* Reserved byte		[3]	*/
	u_char	alloc_len;	/* Allocation length		[4]	*/
	u_char	control;	/* The control byte		[5]	*/
}ALL_REQ_SENSE_CDB6;

/* 
 * LOG sense command cdb 10 bytes
 */
typedef struct all_log_sns_cdb10 {
	u_char	opcode;			/* 0x4d				*/
	u_char	sp		:1,	/* Save parameters		*/
		ppc		:1,	/* Parameter pointer control	*/
				:3,	/* 3 bits reserved		*/
		lun		:3;	/* Logical unit number		*/
	u_char	pg_code		:6,	/* Page code			*/
		pc		:2;	/* Page control field		*/
	u_char			:8;	/* Reserved			*/
	u_char			:8;	/* Reserved			*/
	u_char	param_ptr1;		/* MSB Parameter pointer	*/
	u_char	param_ptr0;		/* LSB Parameter pointer	*/
	u_char	alloc_len1;		/* MSB Allocation length	*/
	u_char	alloc_len0;		/* LSB Allocation length	*/
	u_char	control;		/* Control byte			*/
}ALL_LOG_SNS_CDB10;

/*
 * SCSI Operation Codes for All Devices.
 */
#define SOPC_CHANGE_DEFINITION			0x40
#define SOPC_COMPARE				0x39
#define SOPC_COPY				0x18
#define SOPC_COPY_VERIFY			0x3A
#define SOPC_INQUIRY				0x12
#define SOPC_LOG_SELECT				0x4C
#define SOPC_LOG_SENSE				0x4D
#define SOPC_MODE_SELECT_6			0x15
#define SOPC_MODE_SELECT_10			0x55
#define SOPC_MODE_SENSE_6			0x1A
#define SOPC_MODE_SENSE_10			0x5A
#define SOPC_READ_BUFFER			0x3C
#define SOPC_RECEIVE_DIAGNOSTIC			0x1C
#define SOPC_REQUEST_SENSE			0x03
#define SOPC_SEND_DIAGNOSTIC			0x1D
#define SOPC_TEST_UNIT_READY			0x00
#define SOPC_WRITE_BUFFER			0x3B

/*
 * SCSI Operation Codes for Direct-Access Devices.
 */
#define SOPC_CHANGE_DEFINITION			0x40
#define SOPC_COMPARE				0x39
#define SOPC_COPY				0x18
#define SOPC_COPY_VERIFY			0x3A
#define SOPC_FORMAT_UNIT			0x04
#define SOPC_INQUIRY				0x12
#define SOPC_LOCK_UNLOCK_CACHE			0x36
#define SOPC_LOG_SELECT				0x4C
#define SOPC_LOG_SENSE				0x4D
#define SOPC_MODE_SELECT_6			0x15
#define SOPC_MODE_SELECT_10			0x55
#define SOPC_MODE_SENSE_6			0x1A
#define SOPC_MODE_SENSE_10			0x5A
#define SOPC_PREFETCH				0x34
#define SOPC_PREVENT_ALLOW_REMOVAL		0x1E
#define SOPC_READ_6				0x08
#define SOPC_READ_10				0x28
#define SOPC_READ_BUFFER			0x3C
#define SOPC_READ_CAPACITY			0x25
#define SOPC_READ_DEFECT_DATA			0x37
#define SOPC_READ_LONG				0x3E
#define SOPC_REASSIGN_BLOCKS			0x07
#define SOPC_RECEIVE_DIAGNOSTIC			0x1C
#define SOPC_RELEASE				0x17
#define SOPC_REQUEST_SENSE			0x03
#define SOPC_RESERVE				0x16
#define SOPC_REZERO_UNIT			0x01
#define SOPC_SEARCH_DATA_EQUAL			0x31
#define SOPC_SEARCH_DATA_HIGH			0x30
#define SOPC_SEARCH_DATA_LOW			0x32
#define SOPC_SEEK_6				0x0B
#define SOPC_SEEK_10				0x2B
#define SOPC_SEND_DIAGNOSTIC			0x1D
#define SOPC_SET_LIMITS				0x33
#define SOPC_START_STOP_UNIT			0x1B
#define SOPC_SYNCHRONIZE_CACHE			0x35
#define SOPC_TEST_UNIT_READY			0x00
#define SOPC_VERIFY				0x2F
#define SOPC_WRITE_6				0x0A
#define SOPC_WRITE_10				0x2A
#define SOPC_WRITE_VERIFY			0x2E
#define SOPC_WRITE_BUFFER			0x3B
#define SOPC_WRITE_LONG				0x3F
#define SOPC_WRITE_SAME				0x41

/*
 * SCSI Operation Codes for Sequential-Access Devices.
 */
#define SOPC_CHANGE_DEFINITION			0x40
#define SOPC_COMPARE				0x39
#define SOPC_COPY				0x18
#define SOPC_COPY_VERIFY			0x3A
#define SOPC_ERASE				0x19
#define SOPC_INQUIRY				0x12
#define SOPC_LOAD_UNLOAD			0x1B
#define SOPC_LOCATE				0x2B
#define SOPC_LOG_SELECT				0x4C
#define SOPC_LOG_SENSE				0x4D
#define SOPC_MODE_SELECT_6			0x15
#define SOPC_MODE_SELECT_10			0x55
#define SOPC_MODE_SENSE_6			0x1A
#define SOPC_MODE_SENSE_10			0x5A
#define SOPC_PREVENT_ALLOW_REMOVAL		0x1E
#define SOPC_READ				0x08
#define SOPC_READ_BLOCK_LIMITS			0x05
#define SOPC_READ_BUFFER			0x3C
#define SOPC_READ_POSITION			0x34
#define SOPC_READ_REVERSE			0x0F
#define SOPC_RECEIVE_DIAGNOSTIC			0x1C
#define SOPC_RECOVER_BUFFERED_DATA		0x14
#define SOPC_RELEASE_UNIT			0x17
#define SOPC_REQUEST_SENSE			0x03
#define SOPC_RESERVE_UNIT			0x16
#define SOPC_REWIND				0x01
#define SOPC_SEND_DIAGNOSTIC			0x1D
#define SOPC_SPACE				0x11
#define SOPC_TEST_UNIT_READY			0x00
#define SOPC_VERIFY_TAPE			0x13
#define SOPC_WRITE				0x0A
#define SOPC_WRITE_BUFFER			0x3B
#define SOPC_WRITE_FILEMARKS			0x10

/*
 * SCSI Operation Codes for Printer Devices.
 */
#define SOPC_CHANGE_DEFINITION			0x40
#define SOPC_COMPARE				0x39
#define SOPC_COPY				0x18
#define SOPC_COPY_VERIFY			0x3A
#define SOPC_FORMAT				0x04
#define SOPC_INQUIRY				0x12
#define SOPC_LOG_SELECT				0x4C
#define SOPC_LOG_SENSE				0x4D
#define SOPC_MODE_SELECT_6			0x15
#define SOPC_MODE_SELECT_10			0x55
#define SOPC_MODE_SENSE_6			0x1A
#define SOPC_MODE_SENSE_10			0x5A
#define SOPC_PRINT				0x0A
#define SOPC_READ_BUFFER			0x3C
#define SOPC_RECEIVE_DIAGNOSTIC			0x1C
#define SOPC_RECOVER_BUFFERED_DATA		0x14
#define SOPC_RELEASE_UNIT			0x17
#define SOPC_REQUEST_SENSE			0x03
#define SOPC_RESERVE_UNIT			0x16
#define SOPC_SEND_DIAGNOSTIC			0x1D
#define SOPC_SLEW_PRINT				0x0B
#define SOPC_STOP_PRINT				0x1B
#define SOPC_SYNCHRONIZE_BUFFER			0x10
#define SOPC_TEST_UNIT_READY			0x00
#define SOPC_WRITE_BUFFER			0x3B

/*
 * SCSI Operation Codes for Processor Devices.
 */
#define SOPC_CHANGE_DEFINITION			0x40
#define SOPC_COMPARE				0x39
#define SOPC_COPY				0x18
#define SOPC_COPY_VERIFY			0x3A
#define SOPC_INQUIRY				0x12
#define SOPC_LOG_SELECT				0x4C
#define SOPC_LOG_SENSE				0x4D
#define SOPC_READ_BUFFER			0x3C
#define SOPC_RECEIVE				0x08
#define SOPC_RECEIVE_DIAGNOSTIC			0x1C
#define SOPC_REQUEST_SENSE			0x03
#define SOPC_SEND				0x0A
#define SOPC_SEND_DIAGNOSTIC			0x1D
#define SOPC_TEST_UNIT_READY			0x00
#define SOPC_WRITE_BUFFER			0x3B

/*
 * SCSI Operation Codes for Write-Once Devices.
 */
#define SOPC_CHANGE_DEFINITION			0x40
#define SOPC_COMPARE				0x39
#define SOPC_COPY				0x18
#define SOPC_COPY_VERIFY			0x3A
#define SOPC_INQUIRY				0x12
#define SOPC_LOCK_UNLOCK_CACHE			0x36
#define SOPC_LOG_SELECT				0x4C
#define SOPC_LOG_SENSE				0x4D
#define SOPC_MEDIUM_SCAN			0x38
#define SOPC_MODE_SELECT_6			0x15
#define SOPC_MODE_SELECT_10			0x55
#define SOPC_MODE_SENSE_6			0x1A
#define SOPC_MODE_SENSE_10			0x5A
#define SOPC_PREFETCH				0x34
#define SOPC_PREVENT_ALLOW_REMOVAL		0x1E
#define SOPC_READ_6				0x08
#define SOPC_READ_10				0x28
#define SOPC_READ_12				0xA8
#define SOPC_READ_BUFFER			0x3C
#define SOPC_READ_CAPACITY			0x25
#define SOPC_READ_LONG				0x3E
#define SOPC_REASSIGN_BLOCKS			0x07
#define SOPC_RECEIVE_DIAGNOSTIC			0x1C
#define SOPC_RELEASE				0x17
#define SOPC_REQUEST_SENSE			0x03
#define SOPC_RESERVE				0x16
#define SOPC_REZERO_UNIT			0x01

/*
 * SCSI Operation Codes for CD-ROM Devices.
 */
#define SOPC_CHANGE_DEFINITION			0x40
#define SOPC_COMPARE				0x39
#define SOPC_COPY				0x18
#define SOPC_COPY_VERIFY			0x3A
#define SOPC_INQUIRY				0x12
#define SOPC_LOCK_UNLOCK_CACHE			0x36
#define SOPC_LOG_SELECT				0x4C
#define SOPC_LOG_SENSE				0x4D
#define SOPC_MODE_SELECT_6			0x15
#define SOPC_MODE_SELECT_10			0x55
#define SOPC_MODE_SENSE_6			0x1A
#define SOPC_MODE_SENSE_10			0x5A
#define SOPC_PAUSE_RESUME			0x4B
#define SOPC_PLAY_AUDIO_10			0x45
#define SOPC_PLAY_AUDIO_12			0xA5
#define SOPC_PLAY_AUDIO_MSF			0x47
#define SOPC_PLAY_AUDIO_TRACK_INDEX		0x48
#define SOPC_PLAY_TRACK_RELATIVE_10		0x49
#define SOPC_PLAY_TRACK_RELATIVE_12		0xA9
#define SOPC_PREFETCH				0x34
#define SOPC_PREVENT_ALLOW_REMOVAL		0x1E
#define SOPC_READ_6				0x08
#define SOPC_READ_10				0x28
#define SOPC_READ_12				0xA8
#define SOPC_READ_BUFFER			0x3C
#define SOPC_READ_CAPACITY			0x25
#define SOPC_READ_HEADER			0x44
#define SOPC_READ_LONG				0x3E
#define SOPC_READ_SUBCHANNEL			0x42
#define SOPC_READ_TOC				0x43
#define SOPC_RECEIVE_DIAGNOSTIC			0x1C
#define SOPC_RELEASE				0x17
#define SOPC_REQUEST_SENSE			0x03
#define SOPC_RESERVE				0x16
#define SOPC_REZERO_UNIT			0x01
#define SOPC_SEARCH_DATA_EQUAL_10		0x31
#define SOPC_SEARCH_DATA_EQUAL_12		0xB1
#define SOPC_SEARCH_DATA_HIGH_10		0x30
#define SOPC_SEARCH_DATA_HIGH_12		0xB0
#define SOPC_SEARCH_DATA_LOW_10			0x32
#define SOPC_SEARCH_DATA_LOW_12			0xB2
#define SOPC_SEEK_6				0x0B
#define SOPC_SEEK_10				0x2B
#define SOPC_SEND_DIAGNOSTIC			0x1D
#define SOPC_SET_LIMITS_10			0x33
#define SOPC_SET_LIMITS_12			0xB3
#define SOPC_START_STOP_UNIT			0x1B
#define SOPC_SYNCHRONIZE_CACHE			0x35
#define SOPC_TEST_UNIT_READY			0x00
#define SOPC_VERIFY_10				0x2F
#define SOPC_VERIFY_12				0xAF
#define SOPC_WRITE_BUFFER			0x3B

/*
 * Sony CDU-541 Vendor Unique Commands (Group 6).
 */
#define SOPC_SET_ADDRESS_FORMAT			0xC0
#define SOPC_PLAYBACK_STATUS			0xC4
#define SOPC_PLAY_TRACK				0xC6
#define SOPC_PLAY_MSF				0xC7
#define SOPC_PLAY_VAUDIO			0xC8
#define SOPC_PLAYBACK_CONTROL			0xC9

/*
 * SCSI Operation Codes for Scanner Devices.
 */
#define SOPC_CHANGE_DEFINITION			0x40
#define SOPC_COMPARE				0x39
#define SOPC_COPY				0x18
#define SOPC_COPY_VERIFY			0x3A
#define SOPC_GET_DATA_BUFFER_STATUS		0x34
#define SOPC_GET_WINDOW				0x25
#define SOPC_INQUIRY				0x12
#define SOPC_LOG_SELECT				0x4C
#define SOPC_LOG_SENSE				0x4D
#define SOPC_MODE_SELECT_6			0x15
#define SOPC_MODE_SELECT_10			0x55
#define SOPC_MODE_SENSE_6			0x1A
#define SOPC_MODE_SENSE_10			0x5A
#define SOPC_OBJECT_POSITION			0x31
#define SOPC_READ_SCANNER			0x28
#define SOPC_READ_BUFFER			0x3C
#define SOPC_RECEIVE_DIAGNOSTIC			0x1C
#define SOPC_RELEASE_UNIT			0x17
#define SOPC_REQUEST_SENSE			0x03
#define SOPC_RESERVE_UNIT			0x16
#define SOPC_SCAN				0x1B
#define SOPC_SET_WINDOW				0x24
#define SOPC_SEND_SCANNER			0x2A
#define SOPC_SEND_DIAGNOSTIC			0x1D
#define SOPC_TEST_UNIT_READY			0x00
#define SOPC_WRITE_BUFFER			0x3B

/*
 * SCSI Operation Codes for Optical Memory Devices.
 */
#define SOPC_CHANGE_DEFINITION			0x40
#define SOPC_COMPARE				0x39
#define SOPC_COPY				0x18
#define SOPC_COPY_VERIFY			0x3A
#define SOPC_ERASE_10				0x2C
#define SOPC_ERASE_12				0xAC
#define SOPC_FORMAT_UNIT			0x04
#define SOPC_INQUIRY				0x12
#define SOPC_LOCK_UNLOCK_CACHE			0x36
#define SOPC_LOG_SELECT				0x4C
#define SOPC_LOG_SENSE				0x4D
#define SOPC_MEDIUM_SCAN			0x38
#define SOPC_MODE_SELECT_6			0x15
#define SOPC_MODE_SELECT_10			0x55
#define SOPC_MODE_SENSE_6			0x1A
#define SOPC_MODE_SENSE_10			0x5A
#define SOPC_PREFETCH				0x34
#define SOPC_PREVENT_ALLOW_REMOVAL		0x1E
#define SOPC_READ_6				0x08
#define SOPC_READ_10				0x28
#define SOPC_READ_12				0xA8
#define SOPC_READ_BUFFER			0x3C
#define SOPC_READ_CAPACITY			0x25
#define SOPC_READ_DEFECT_DATA_10		0x37
#define SOPC_READ_DEFECT_DATA_12		0xB7
#define SOPC_READ_GENERATION			0x29
#define SOPC_READ_LONG				0x3E
#define SOPC_READ_UPDATED_BLOCK			0x2D
#define SOPC_REASSIGN_BLOCKS			0x07
#define SOPC_RECEIVE_DIAGNOSTIC			0x1C
#define SOPC_RELEASE				0x17
#define SOPC_REQUEST_SENSE			0x03
#define SOPC_RESERVE				0x16
#define SOPC_REZERO_UNIT			0x01
#define SOPC_SEARCH_DATA_EQUAL_10		0x31
#define SOPC_SEARCH_DATA_EQUAL_12		0xB1
#define SOPC_SEARCH_DATA_HIGH_10		0x30
#define SOPC_SEARCH_DATA_HIGH_12		0xB0
#define SOPC_SEARCH_DATA_LOW_10			0x32
#define SOPC_SEARCH_DATA_LOW_12			0xB2
#define SOPC_SEEK_6				0x0B
#define SOPC_SEEK_10				0x2B
#define SOPC_SEND_DIAGNOSTIC			0x1D
#define SOPC_SET_LIMITS_10			0x33
#define SOPC_SET_LIMITS_12			0xB3
#define SOPC_START_STOP_UNIT			0x1B
#define SOPC_SYNCHRONIZE_CACHE			0x35
#define SOPC_TEST_UNIT_READY			0x00
#define SOPC_UPDATE_BLOCK			0x3D
#define SOPC_VERIFY_10				0x2F
#define SOPC_VERIFY_12				0xAF
#define SOPC_WRITE_6				0x0A
#define SOPC_WRITE_10				0x2A
#define SOPC_WRITE_12				0xAA
#define SOPC_WRITE_VERIFY_10			0x2E
#define SOPC_WRITE_VERIFY_12			0xAE
#define SOPC_WRITE_BUFFER			0x3B
#define SOPC_WRITE_LONG				0x3F

/*
 * SCSI Operation Codes for Medium-Changer Devices.
 */
#define SOPC_CHANGE_DEFINITION			0x40
#define SOPC_EXCHANGE_MEDIUM			0xA6
#define SOPC_INITIALIZE_ELEMENT_STATUS		0x07
#define SOPC_INQUIRY				0x12
#define SOPC_LOG_SELECT				0x4C
#define SOPC_LOG_SENSE				0x4D
#define SOPC_MODE_SELECT_6			0x15
#define SOPC_MODE_SELECT_10			0x55
#define SOPC_MODE_SENSE_6			0x1A
#define SOPC_MODE_SENSE_10			0x5A
#define SOPC_MOVE_MEDIUM			0xA5
#define SOPC_POSITION_TO_ELEMENT		0x2B
#define SOPC_PREVENT_ALLOW_REMOVAL		0x1E
#define SOPC_READ_BUFFER			0x3C
#define SOPC_READ_ELEMENT_STATUS		0xB8
#define SOPC_RECEIVE_DIAGNOSTIC			0x1C
#define SOPC_RELEASE				0x17
#define SOPC_REQUEST_VOLUME_ELEMENT_ADDRESS	0xB5
#define SOPC_REQUEST_SENSE			0x03
#define SOPC_RESERVE				0x16
#define SOPC_REZERO_UNIT			0x01
#define SOPC_SEND_DIAGNOSTIC			0x1D
#define SOPC_SEND_VOLUME_TAG			0xB6
#define SOPC_TEST_UNIT_READY			0x00
#define SOPC_WRITE_BUFFER			0x3B

/*
 * SCSI Operation Codes for Communication Devices.
 */
#define SOPC_CHANGE_DEFINITION			0x40
#define SOPC_GET_MESSAGE_6			0x08
#define SOPC_GET_MESSAGE_10			0x28
#define SOPC_GET_MESSAGE_12			0xA8
#define SOPC_INQUIRY				0x12
#define SOPC_LOG_SELECT				0x4C
#define SOPC_LOG_SENSE				0x4D
#define SOPC_MODE_SELECT_6			0x15
#define SOPC_MODE_SELECT_10			0x55
#define SOPC_MODE_SENSE_6			0x1A
#define SOPC_MODE_SENSE_10			0x5A
#define SOPC_READ_BUFFER			0x3C
#define SOPC_RECEIVE_DIAGNOSTIC			0x1C
#define SOPC_REQUEST_SENSE			0x03
#define SOPC_SEND_DIAGNOSTIC			0x1D
#define SOPC_SEND_MESSAGE_6			0x0A
#define SOPC_SEND_MESSAGE_10			0x2A
#define SOPC_SEND_MESSAGE_12			0xAA
#define SOPC_TEST_UNIT_READY			0x00
#define SOPC_WRITE_BUFFER			0x3B

#endif
