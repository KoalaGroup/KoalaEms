/*****************************************************************************
*                                                                            *
*		          Include File pbL2con.h		                             *
*                                                                            *
*																		     *
* The include file pbL2con.h contains all constants that the user needs      *
* for programming LAYER2.										     		 *
* 									     									 *
*****************************************************************************/



/****************************************************************************
*                                                                           *
* Edition History                                                           *
* ===============                                                           *
*                                                                           *
* #    Date   Comments                                                 by   *
* -- -------- ------------------------------------------------------- ----  *
* 01 11/03/92 First written                                           HAH   *
*                                                                           *
* 02 01/27/93 Added new service definition:                           HAH   *
*                #define FMA2_CHANGE_BUPARAMETER   0x1F						*
*                                                                           *
****************************************************************************/




/* ---------------------------------------------------------------------------
+									     +
+	Definition of Boolean Constants 				     +   
+									     +
+ ------------------------------------------------------------------------- */
 
#define TRUE 			 -1
#define FALSE 			 0



/* ---------------------------------------------------------------------------
+									     +
+	Definition of the Service Primitives				     +   
+									     +
+ ------------------------------------------------------------------------- */
 
#define REQ 			 0
#define CON 			 1
#define IND 			 2



/* ---------------------------------------------------------------------------
+									     +
+	Definition of Constants for the Length of FDL Telegram Header and    +
+	FDL Telegram Trailer						     +   
+									     +
+ ------------------------------------------------------------------------- */
 
#define FDL_OFFSET		11
#define FDL_TRAILER		 2


/* ---------------------------------------------------------------------------
+									     +
+	Definition of Constants for the Length of IDENT Telegram and         +
+	LSAP Status Telegram    					     +   
+									     +
+ ------------------------------------------------------------------------- */
 
#define IDENT_TELE_LEN		255
#define LSAP_STATUS_TELE_LEN	FDL_OFFSET + FDL_TRAILER + 6


/* ---------------------------------------------------------------------------
+									     +
+	Definition of the Service Codes for FDL and FMA2 Services	     +   
+									     +
+ ------------------------------------------------------------------------- */
 
#define SDA            		0x01
#define SDN	       		0x02
#define SRD	       		0x03
#define CSRD	       		0x04
#define LOAD_POLL_LIST		0x05
#define DEACT_POLL_LIST		0x06
#define POLL_ENTRY		0x07
#define SEND_UPDATE		0x08
#define REPLY_UPDATE		0x09

#define FMA2_RESET      	0x10
#define FMA2_SET_BUSPARAMETER  	0x11
#define FMA2_SET_STATISTIC_CTR  0x12
#define FMA2_READ_BUSPARAMETER	0x13
#define FMA2_READ_STATISTIC_CTR 0x14
#define FMA2_READ_TRR		0x15
#define FMA2_READ_LAS		0x16
#define FMA2_READ_GAPLIST	0x17
#define FMA2_EVENT		0x18
#define FMA2_IDENT		0x19
#define FMA2_LSAP_STATUS	0x1A
#define FMA2_LIVELIST		0x1B
#define FMA2_ACTIVATE_SAP 	0x1C
#define FMA2_ACTIVATE_RSAP 	0x1D
#define FMA2_DEACTIVATE_SAP 	0x1E
#define FMA2_CHANGE_BUSPARAMETER  	0x1F

#define WAIT_FOR_FMA2_EVENT	0x20
#define PUT_RESRC_TO_FDL	0x21
#define WITHDRAW_RESRC_FROM_FDL	0x22
#define WITHDRAW_EVENT		0x23



/* ---------------------------------------------------------------------------
+									     +
+	Definition of Confirmation Status and Update Status of 		     +
+	SRD-Indications and CSRD-Confirmations.				     +
+									     +
+ ------------------------------------------------------------------------- */

#define OK 	0x00
#define UE  	0x01
#define RR  	0x02
#define RS  	0x03
#define HI  	0x05
#define LO  	0x06
#define DL  	0x08
#define NR  	0x09
#define DH  	0x0a
#define RDL 	0x0c
#define RDH 	0x0d
#define LS  	0x10
#define NA  	0x11
#define NLT 	0x12	/* corresponds to status DS in DIN 19245 Teil 1     */
#define NO  	0x13
#define LR  	0x14
#define IV  	0x15



/* ---------------------------------------------------------------------------
+									     +
+	Definition of Broadcast SAP, Default SAP and FMA2 SAPs		     +
+									     +
+ ------------------------------------------------------------------------- */

#define BRCT_SAP		0x3F
#define DEFAULT_SAP		 128

#define MSAP_0			0xF0
#define MSAP_1			0xF1
#define MSAP_2			0xF2


/* ---------------------------------------------------------------------------
+									     +
+	Definition of Constant NO_SEGMENT (for component 'segment' in	     +
+	type T_FDL_ADDR)						     +
+									     +
+ ------------------------------------------------------------------------- */

#define NO_SEGMENT		0xFF





/* ---------------------------------------------------------------------------
+									     +
+	Definition of Constants for services FMA2_SET_BUSPARAMETER and	     +
+	FMA2_READ_BUSPARAMETER						     +
+									     +
+ ------------------------------------------------------------------------- */

/* Baud rate -------------------------------------------------------------- */

#define K_BAUD_9_6		0
#define K_BAUD_19_2		1
#define K_BAUD_93_75		2
#define K_BAUD_187_5		3
#define K_BAUD_500		4

/* Redundancy ------------------------------------------------------------- */

#define NO_REDUNDANCY 		0
#define REDUNDANCY 		1




/* ---------------------------------------------------------------------------
+									     +
+	Definition of Constants for SAP Activation			     +
+									     +
+ ------------------------------------------------------------------------- */

/* Service type ----------------------------------------------------------- */

#define SDA_RESERVED		0x00
#define SDN_RESERVED		0x01
#define SRD_RESERVED		0x03
#define CSRD_RESERVED		0x05

/* Role in Service -------------------------------------------------------- */

#define INITIATOR		0x00
#define RESPONDER		0x10
#define BOTH_ROLES		0x20
#define SERVICE_NOT_ACTIVATED	0x30



/* ---------------------------------------------------------------------------
+									     +
+	Definition of Service Class in Send Requests and Indications	     +
+									     +
+ ------------------------------------------------------------------------- */

#define LOW		0
#define HIGH		1




/* ---------------------------------------------------------------------------
+									     +
+	Definition of Transmit Mode in Send Update Requests and Reply 	     +
+	Update Requests     						     +
+									     +
+ ------------------------------------------------------------------------- */

#define SINGLE			0xF0
#define MULTIPLE		0xF1



/* ---------------------------------------------------------------------------
+									     +
+	ALL is Global Address and Confirm Mode in Poll List 		     +
+	respectively Indication Mode in Responder SAP			     +
+									     +
+	DATA is Confirm Mode in Poll List and Indication Mode 		     +
+	in Responder SAP						     +
+									     +
+ ------------------------------------------------------------------------- */

#define ALL				0xFF
#define GLOBAL_ADDR		0x7F

#define DATA			0xF0




/* ---------------------------------------------------------------------------
+									     +
+	Definition of Poll List Entry Marker State (Service POLL_ENTRY)	     +
+									     +
+ ------------------------------------------------------------------------- */

#define UNLOCKED		0x00
#define LOCKED			0x01




/* ---------------------------------------------------------------------------
+									     +
+	Definition FMA2 events						     +
+									     +
+ ------------------------------------------------------------------------- */

#define FMA2_FAULT_ADDRESS	0x01	/* Duplicate address recognized	*/
#define FMA2_FAULT_TRANSCEIVER	0x02	/* Transceiver error occured	*/
#define FMA2_FAULT_TTO		0x03	/* Time out on BUS detected	*/
#define FMA2_FAULT_SYN		0x04	/* No receiver synchronization	*/
#define FMA2_FAULT_OUT_OF_RING	0x05	/* Station out of ring		*/
#define FMA2_GAP_EVENT	 	0x06	/* New station in ring		*/


