/*======================================================================*/
/*
   Filename     : SFI.H

   Autor        : PS
   Datum        : 95/02/09

   Sprache      : C
   Standard     : K&R

   Inhalt       : Constant definitions for SFI.C

*/
/*======================================================================*/
/*
   Datum, Name    Bemerkungen
  ----------------------------------------------------------------------
   95/02/09 PS    Ersterstellung
*/
/*======================================================================*/
/*
SccsId = %W%    %G%
*/

#ifndef __SFI_H
#define __SFI_H

struct sfiStruct {
   ems_u32 VMESlaveAddress;
   ems_u32* lca1Vout;     /* output register -> LEDs, ECLout, NIMout */
   ems_u32* outregWrite;  
   ems_u32* outregRead;
   ems_u32* dfctrlReg;
   ems_u32* fbProtReg;
   ems_u32* fbArbReg;
   ems_u32* fbCtrlReg;
   ems_u32* lca1Reset;
   ems_u32* lca2Reset;
   ems_u32* fastbusreadback;
   ems_u32* sequencerRamAddressReg;
   ems_u32* sequencerFlowControlReg;

   ems_u32* resetVme2SeqFifo;
   ems_u32* readClockVme2SeqFifo;
   ems_u32* resetSeq2VmeFifo;
   ems_u32* writeClockSeq2VmeFifo;
   ems_u32* writeVme2SeqFifoBase;
   ems_u32* readSeq2VmeFifoBase;

   ems_u32* readSeqFifoFlags;
   ems_u32* readVme2SeqAddressReg;
   ems_u32* readVme2SeqDataReg;
   ems_u32  sequencerOutputFifoSize;

   ems_u32* sequencerReset;
   ems_u32* sequencerEnable;
   ems_u32* sequencerDisable;
   ems_u32* sequencerRamLoadEnable;
   ems_u32* sequencerRamLoadDisable;

   ems_u32* sequencerStatusReg;
   ems_u32* FastbusStatusReg1;
   ems_u32* FastbusStatusReg2;

   ems_u32* FastbusTimeoutReg;
   ems_u32* FastbusArbitrationLevelReg;
   ems_u32* FastbusProtocolInlineReg;
   ems_u32* sequencerFifoFlagAndEclNimInputReg;
   ems_u32* nextSequencerRamAddressReg;
   ems_u32* lastSequencerProtocolReg;
   ems_u32* VmeIrqLevelAndVectorReg;
   ems_u32* VmeIrqMaskReg;
   ems_u32* resetRegisterGroupLca2;
   ems_u32* sequencerTryAgain;

   ems_u32* readVmeTestReg;
   ems_u32* readLocalFbAdBus;
   ems_u32* writeLocalFbAdBus;
   ems_u32* AuxPulsB40;
   ems_u32* readVme2SeqDataFifo;
   ems_u32* writeVmeOutSignalReg;
   ems_u32* clearBothLca1TestReg;
   ems_u32* writeVmeTestReg;
   ems_u32* last_PrimAdr_Reg; 
};


/* address offsets for test design */

#define SFI_TEST_LCA1_RESET          (0x1004)
#define SFI_TEST_LCA1_VOUT           (0x1000)
#define SFI_TEST_OUTREG_WRITE        (0x1008)
#define SFI_TEST_OUTREG_READ 	     (0x1004)
#define SFI_TEST_FASTBUS_READBACK    (0x1008)


#define SFI_TEST_SEQ_FIFO_FLAG_REGISTER     (0x200C)


#define SFI_TEST_DATA_FLOW_CONTROL_REGISTER (0x2000)
#define SFI_TEST_FB_PROTOCOL_REGISTER       (0x2004)
#define SFI_TEST_FB_ARBITRATION_REGISTER    (0x2008)
#define SFI_TEST_FB_CONTROL_REGISTER        (0x200C)
#define SFI_TEST_RESET                      (0x2020)

#define SFI_TEST_SEQUENCER_RAM_ADDRESS_REGISTER  (0x2010)
#define SFI_TEST_SEQUENCER_FLOW_CONTROL_REGISTER (0x2014)
#define SFI_TEST_RESET_VME2SEQ_FIFO              (0x2030)
#define SFI_TEST_READ_CLOCK_VME2SEQ_FIFO         (0x2034)
#define SFI_TEST_RESET_SEQ2VME_FIFO              (0x2038)
#define SFI_TEST_WRITE_CLOCK_SEQ2VME_FIFO        (0x203C)

#define SFI_TEST_READ_SEQ2VME_FIFO_BASE          (0x4000)
#define SFI_TEST_WRITE_VME2SEQ_FIFO_BASE         (0x10000)

#define SFI_TEST_READ_VME2SEQ_ADDRESS_REGISTER   (0x2008)
#define SFI_TEST_READ_VME2SEQ_DATA_REGISTER      (0x100C)

#define SFI_TEST_SEQUENCER_RESET  	         (0x3000)
#define SFI_TEST_SEQUENCER_ENABLE	         (0x3000)
#define SFI_TEST_SEQUENCER_DISABLE	         (0x3000)
#define SFI_TEST_SEQUENCER_RAM_LOAD_ENABLE	 (0x3000)
#define SFI_TEST_SEQUENCER_RAM_LOAD_DISABLE	 (0x3000)




/* address offsets for normal operation design */

#define SFI_READ_INT_FB_AD                    	(0x1000) 
#define SFI_LAST_PRIMADR_REG                    (0x1004) 

#define SFI_WRITE_VME_OUT_SIGNAL_REGISTER	(0x1000)
#define SFI_CLEAR_BOTH_LCA1_TEST_REGISTER	(0x1004)
#define SFI_WRITE_INT_FB_AD_REG			(0x1010)
#define SFI_KEY_AUX_PULS_B40			(0x1014)


/* #define SFI_READ_VME_TEST_REGISTER		(0x1004) */
/* #define SFI_READ_LOCAL_FB_AD_BUS		(0x1008) */
/* #define SFI_READ_VME2SEQ_DATA_FIFO		(0x100C) */
/* #define SFI_WRITE_VME_TEST_REGISTER 		(0x1008) */



#define SFI_FASTBUS_TIMEOUT_REGISTER		(0x2000)
#define SFI_FASTBUS_ARBITRATION_LEVEL_REGISTER	(0x2004)
#define SFI_FASTBUS_PROTOCOL_INLINE_REGISTER	(0x2008)
#define SFI_SEQUENCER_FIFO_FLAG_AND_ECL_NIM_INPUT_REGISTER 	(0x200C)

#define SFI_NEXT_SEQUENCER_RAM_ADDRESS_REGISTER	(0x2018)
#define SFI_LAST_SEQUENCER_PROTOCOL_REGISTER	(0x201C)
#define SFI_SEQUENCER_STATUS_REGISTER		(0x2220)
#define SFI_FASTBUS_STATUS_REGISTER1		(0x2224)
#define SFI_FASTBUS_STATUS_REGISTER2		(0x2228)

#define SFI_VME_IRQ_LEVEL_AND_VECTOR_REGISTER	(0x2010)
#define SFI_VME_IRQ_MASK_REGISTER		(0x2014)
#define SFI_SEQUENCER_RAM_ADDRESS_REGISTER	(0x2018)
#define SFI_RESET_REGISTER_GROUP_LCA2		(0x201C)

#define SFI_SEQUENCER_ENABLE			(0x2020)
#define SFI_SEQUENCER_DISABLE			(0x2024)
#define SFI_SEQUENCER_RAM_LOAD_ENABLE		(0x2028)
#define SFI_SEQUENCER_RAM_LOAD_DISABLE		(0x202C)
#define SFI_SEQUENCER_RESET  			(0x2030)
#define SFI_SEQUENCER_TRY_AGAIN			(0x2034) 

#define SFI_READ_SEQ2VME_FIFO_BASE          (0x4000)
#define SFI_WRITE_VME2SEQ_FIFO_BASE         (0x10000)
#define SFI_CLEAR_SEQUENCER_CMD_FLAG (0x2038)



/* constants for sequencer key addresses */
#define PRIM_DSR		(0x0004)
#define PRIM_CSR		(0x0104)
#define PRIM_DSRM		(0x0204)
#define PRIM_CSRM		(0x0304)
#define PRIM_AMS4		(0x0404)
#define PRIM_AMS5		(0x0504)
#define PRIM_AMS6		(0x0604)
#define PRIM_AMS7		(0x0704)

#define PRIM_HM_DSR		(0x0014)
#define PRIM_HM_CSR		(0x0114)
#define PRIM_HM_DSRM		(0x0214)
#define PRIM_HM_CSRM		(0x0314)
#define PRIM_HM_AMS4		(0x0414)
#define PRIM_HM_AMS5		(0x0514)
#define PRIM_HM_AMS6		(0x0614)
#define PRIM_HM_AMS7		(0x0714)

#define PRIM_EG			(0x1000)

#define RNDM_R			(0x0844)
#define RNDM_W			(0x0044)
#define SECAD_R			(0x0A44)
#define SECAD_W			(0x0244)

#define RNDM_R_DIS		(0x0854)
#define RNDM_W_DIS		(0x0054)
#define SECAD_R_DIS		(0x0A54)
#define SECAD_W_DIS		(0x0254)

#define DISCON			(0x0024)
#define DISCON_RM		(0x0034)

#define START_FRDB_WITH_CLEAR_WORD_COUNTER  (0x08A4)

#define STORE_FRDB_WC 		(0x00E4)
#define STORE_FRDB_AP		(0x00D4)

#define LOAD_DMA_ADDRESS_POINTER (0x0094)

#define DISABLE_RAM_MODE	(0x0038)

#define ENABLE_RAM_SEQUENCER    (0x0028)

#define SEQ_WRITE_OUT_REG       (0x0008)
#define SEQ_SET_CMD_FLAG (0x0048)
#endif

