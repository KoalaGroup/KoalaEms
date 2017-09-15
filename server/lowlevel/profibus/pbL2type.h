/*****************************************************************************
*                                                                            *
*		          Include File pbL2type.h		                             *
*                                                                            *
*																		     *
* The include file pbL2type.h contains all structures that the user needs    *
* for programming LAYER2.										     		 *
* 									     									 *
*****************************************************************************/


/*****************************************************************************
*                                                                            *
* Edition History                                                            *
* ===============                                                            *
*                                                                            *
* #    Date   Comments                                                  by   *
* -- -------- -------------------------------------------------------- ----  *
* 01 11/03/92 First written                                            HAH   *
* 02 01/27/93 T_FDL_SERVICE_DESCR extended for:                        HAH   *
*             	"profiman" edition #7                                        *	
*				"phyPROFI" edition #10                                       *
*                                                                            *
*****************************************************************************/



/* ------------------------------------------------------------------------- +
+  Definition of Base Types						     +
+  ------------------------------------------------------------------------ */

#define far
#define VOID void	
#define BOOL char
#define INT8 char
#define INT16 short
#define USIGN8 unsigned char
#define USIGN16 unsigned short
#define USIGN32 unsigned long

/* ------------------------------------------------------------------------- +
+  Definition of LAYER2 Types						     +
+  ------------------------------------------------------------------------ */

typedef struct T_FDL_ADDR
	{
	USIGN8			station;	
	USIGN8			segment;	
	} T_FDL_ADDR;


typedef struct T_FDL_PDU
	{
	USIGN8 far *		buffer_ptr;	
	USIGN8			length;		
	} T_FDL_PDU;


typedef struct T_FDL_SERVICE_DESCR
	{
	USIGN8				sap;		
	USIGN8				service;	
	USIGN8				primitive;
	USIGN8				path_id;	/* reserved for OS-9 PROFIBUS Manager */
	USIGN16				user_id;
	USIGN8				status;		
	USIGN8 far *			descr_ptr;	
	struct T_FDL_SERVICE_DESCR far * next_descr;	
	struct T_FDL_SERVICE_DESCR far * link_descr;
									/* reserved for OS-9 PROFIBUS Manager */
	USIGN32				resrv;		/* reserved for OS-9 PROFIBUS Manager */
									/* currently not used                 */
	} T_FDL_SERVICE_DESCR;



/* ------------------------------------------------------------------------ */

typedef struct T_BUSPAR_BLOCK
	{
	T_FDL_ADDR		loc_add;	
	USIGN8			baud_rate;	
	USIGN8			medium_red;	
	USIGN16			tsl;		
	USIGN16			min_tsdr;	
	USIGN16			max_tsdr;	
	USIGN8			tqui;		
	USIGN8			tset;		
	USIGN32			ttr;		
	USIGN8			g;		
	BOOL			in_ring_desired;
	USIGN8			hsa;		
	USIGN8			max_retry_limit;
	USIGN8 far *		ident;		
	USIGN8			ind_buf_len;	
	} T_BUSPAR_BLOCK;



/* ------------------------------------------------------------------------ */

typedef struct T_FDL_SAP_DESCR
	{
	USIGN8			sap_nr;			
	T_FDL_ADDR		rem_add;		
	USIGN8			sda;			
	USIGN8			sdn;			
	USIGN8			srd;			
	USIGN8			csrd;			
	USIGN8			services;		
	USIGN8 far *		sap_block_ptr;		
	T_FDL_SERVICE_DESCR far * resrc_tail;
	T_FDL_SERVICE_DESCR far * resrc_hdr;
	USIGN8			resrc_ctr;
	USIGN8			sema;			
	} T_FDL_SAP_DESCR;


typedef struct T_FDL_SAP_BLOCK
	{
	USIGN8			max_len_sda_req_low;	
	USIGN8			max_len_sda_req_high;	
	USIGN8			max_len_sda_ind_low;	
	USIGN8			max_len_sda_ind_high;	
	USIGN8			max_len_sdn_req_low;	
	USIGN8			max_len_sdn_req_high;	
	USIGN8			max_len_sdn_ind_low;	
	USIGN8			max_len_sdn_ind_high;	
	USIGN8			max_len_srd_req_low;	
	USIGN8			max_len_srd_req_high;	
	USIGN8			max_len_srd_con_low;	
	USIGN8			max_len_srd_con_high;	
	} T_FDL_SAP_BLOCK;

typedef struct T_FDL_RSAP_BLOCK
	{
	USIGN8			indication_mode;	
	USIGN8			max_len_upd_req_low;	
	USIGN8			max_len_upd_req_high;	
	USIGN8			max_len_srd_ind_low;	
	USIGN8			max_len_srd_ind_high;	
	USIGN8			max_len_sdn_ind_low;	
	USIGN8			max_len_sdn_ind_high;	
	T_FDL_PDU		upd_buf_low;		
	T_FDL_PDU		upd_buf_high;		
	T_FDL_PDU		telegram_low;		
	T_FDL_PDU		telegram_high;		
	USIGN8			transmit_low;		
	USIGN8			transmit_high;		
	USIGN8			marker_low;		
	USIGN8			marker_high;		
	USIGN8			fcs_low;		
	USIGN8			fcs_high;		
	} T_FDL_RSAP_BLOCK;


typedef struct T_FDL_DEACT_SAP
	{
	USIGN8			ssap;		
	T_FDL_SAP_DESCR far *	sap_descr_ptr;	
	} T_FDL_DEACT_SAP;


/* ------------------------------------------------------------------------ */

typedef struct T_FDL_SR_BLOCK
	{
	T_FDL_ADDR		loc_add;	
	USIGN8			remote_sap;	
	T_FDL_ADDR		rem_add;	
	USIGN8			serv_class;	
	USIGN8			update_status;	
	T_FDL_PDU		send_data;	
	T_FDL_PDU		receive_data;	
	T_FDL_PDU		resource;	
	} T_FDL_SR_BLOCK;


typedef struct T_FDL_UPDATE_BLOCK
	{
	USIGN8				dsap;		
	T_FDL_ADDR			rem_add;	
	USIGN8				serv_class;	
	USIGN8				transmit;	
	T_FDL_PDU			upd_data;	
	} T_FDL_UPDATE_BLOCK;



/* ------------------------------------------------------------------------ */

typedef struct T_POLL_LIST_ELEMENT
	{
	USIGN8			dsap;		
	T_FDL_ADDR		rem_add;	
	USIGN8			max_len_csrd_req_low;	
	USIGN8			max_len_csrd_con_low;	
	USIGN8			max_len_csrd_con_high;	
	T_FDL_PDU		poll_buffer;	
	T_FDL_PDU		send_data;	
	T_FDL_SERVICE_DESCR far *resrc_hdr; 	
	T_FDL_SERVICE_DESCR far *resrc_tail;	
	USIGN8			resrc_ctr;	
	T_FDL_SERVICE_DESCR far *next_for_rcv;
	USIGN8			transmit;	
	USIGN8			to_send;	
	USIGN8			marker;		
	T_FDL_PDU		poll_telegram;	
	T_FDL_PDU		data_telegram;	
	USIGN8			data_fcs;	
	USIGN8			poll_fcs;	
	} T_POLL_LIST_ELEMENT;


typedef T_POLL_LIST_ELEMENT far * T_POLL_LIST_ELEM_PTR; 


typedef struct T_POLL_LIST_DESCR
	{
	USIGN8 				len;		
	USIGN8				confirm_mode;	
	T_POLL_LIST_ELEM_PTR far *	elem_ptr;	
	} T_POLL_LIST_DESCR;


typedef struct T_POLL_ENTRY
	{
	USIGN8				dsap;		
	T_FDL_ADDR			rem_add;	
	USIGN8 				marker;		
	} T_POLL_ENTRY;


	
/* ------------------------------------------------------------------------ */

typedef struct T_FDL_RESRC_DESCR 
	{
	USIGN8 				dsap;		
	T_FDL_ADDR			rem_add;	
	USIGN8 				nr_of_resources;
	T_FDL_SERVICE_DESCR far *	resrc_ptr;	
	} T_FDL_RESRC_DESCR;



/* ------------------------------------------------------------------------ */

typedef struct T_FDL_STATISTIC_CTR
	{
	USIGN32 	frame_send_count;	
	USIGN16		retry_count;		
	USIGN32		sd_count;		
	USIGN16		sd_error_count;		
	} T_FDL_STATISTIC_CTR;
