/************************************************************
* $ZEL: can331.c,v 1.1 2006/09/15 17:50:56 wuestner Exp $
*
* Funktionen fuer das High Voltage Power Supply von iseg
*
* Autor: Sebastian Detert
*
* Datum: Feb.06
*        Apr.06 support fuer pci/cpci can331
*
************************************************************/

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <emsctypes.h>
#include <errorcodes.h>
/*#include "../canbus.h"*/
#include "can_can331.h"
#include "can331.h"

/* Berechnet aus dem data frame einer CAN Nachricht die zugehoerige
 ID des Slaves */
#define REAL_ID(msgID) ((msgID>>3)&0x3f)

#define SLEEP(arg) usleep(arg)

/*****************************************************************************/
/* missing procedures */
int canGetBaudrate(
         HANDLE handle,           /* nt-handle                        */
         unsigned long *baud )    /* Out: baudrate                    */
{return 0;}
int canSetBaudrate(
         HANDLE handle,           /* nt-handle                        */
         unsigned long baud )     /* baudrate-constant                */
{return 0;}
int CALLTYPE canIdAdd(            /* ret NTCAN_OK, if success         */
         HANDLE        handle,    /* read handle                      */
         long          id  )      /* can identifier                   */
{return 0;}
int CALLTYPE canSend(             /* ret 0, if success                */
         HANDLE        handle,    /* handle                           */
         CMSG          *cmsg,     /* ptr to data-buffer               */
         long          *len )     /* size of CMSG-Buffer              */
{return 0;}
int CALLTYPE canIoctl(
           HANDLE        handle,  /* handle                           */
           unsigned long ulCmd,   /* Command                          */
           void *        pArg )   /* Ptr to command specific argument */
{return 0;}
int CALLTYPE canTake(             /* ret 0, if success                */
         HANDLE        handle,    /* handle                           */
         CMSG          *cmsg,     /* ptr to data-buffer               */
         long          *len )     /* out: size of CMSG-Buffer         */
                                  /* in:  count of read messages      */
{return 0;}
int CALLTYPE canOpen(
         int           net,          /* net number                    */
         unsigned long flags,        /*                               */
         long          txqueuesize,  /* nr of entries in message queue*/
         long          rxqueuesize,  /* nr of entries in message queue*/
         long          txtimeout,    /* tx-timeout in miliseconds     */
         long          rxtimeout,    /* rx-timeout in miliseconds     */
         HANDLE        *handle )     /* out: nt-handle                */
{return 0;}

/*****************************************************************************/

/*
 * Initialisieren des CAN Handles
 *
 * returns:
 * 0 -> fehlerfrei
 * 1 -> LINUX_CAN_Open fehlgeschlagen
 * 2 -> CAN_Init fehlgeschlagen
 */
static int
SD_CAN_331_Init(struct can_dev* dev)
{
    struct can_can331_info* info=&dev->info.can331;
  int ret;
  long i, j;
  unsigned long  baudrate;
  
  if( (ret = canOpen(CAN_NET, CAN_MODE, CAN_TXQUEUESIZE, 
		     CAN_RXQUEUESIZE, CAN_TXTIMEOUT, CAN_RXTIMEOUT, 
		     &info->hnd)) != NTCAN_SUCCESS )
    {
      return 1;
    }
  
  if( (ret = canGetBaudrate(info->hnd, &baudrate)) != NTCAN_SUCCESS ) {
    return 2;
  }
  else if( baudrate != USE_BAUD ) {
    if( (ret=canSetBaudrate(info->hnd, USE_BAUD))!=NTCAN_SUCCESS ) {
      return 3;
    }
  }
    
  for(i = 0; i <= 2047; i++) {
    for(j = 0; j < 2; j++) {
      ret = canIdAdd(info->hnd, i);	    
      if(ret == NTCAN_INSUFFICIENT_RESOURCES) {
	SLEEP(100);
	continue;
      }
      break;
    }
    if(ret != NTCAN_SUCCESS)
      {
	break;
      }
  }
  
  return 0;
    
}

/*****************************************************************************/

 /*
  * CAN Nachricht senden (s.o.), hier mit fertigem Identifier
  *
  * returns:
  * 0 -> fehlerfrei
  * 1 -> Fehler aufgetreten
  * 2 -> Geraet zu dem gesendet werden soll, wurde beim Start nicht gefunden
  */
static int
SD_CAN_331_Send_ID(struct can_dev* dev, int identifier,  int data_id, int len,
        int *buf)
{
    struct can_can331_info* info=&dev->info.can331;
    CMSG msg;
    int i;
    long leins = 1;
            
    if( SD_CAN_331_Init(dev) != 0 ) {
        /* printf("SD_CAN_331_Send_ID : Init NOT successful\n"); */
        return 1;
    }
    
    if( info->found_devices[REAL_ID(identifier)].found == 0) {
        /* printf("SD_CAN_331_Send_ID : device #%d NOT found\n", REAL_ID(identifier)); */
        return 2;
    }
        
    msg.id = identifier;
    /*msg.MSGTYPE = MSGTYPE_STANDARD;*/
    msg.len = len + 1;    
    msg.data[0] = data_id;

    for (i=1; i<=len; ++i) {
        msg.data[i] = buf[i-1];
    }
    for(i=len+1; i<8; ++i) {
      msg.data[i] = 0;      
    }
    
    if( canSend(info->hnd, &msg, &leins) != NTCAN_SUCCESS) {   
      return 3;
    }

    return 0;
} /* int SD_CAN_331_Send_ID( ... ) */

/*****************************************************************************/

 /*
  * Standart CAN Nachricht senden
  * ext_instr = 0
  * nmt = 0
  * active = 0
  *
  * returns:
  * 0 -> fehlerfrei
  * 1 -> Fehler aufgetreten
  */
static int
SD_CAN_331_Send_Short(struct can_dev* dev, int data_dir, int id, int data_id,
        int len, int *buf)
{
    return SD_CAN_331_Send_ID(dev, (1<<9)|(id<<3)|data_dir, data_id, len, buf);
}

/*****************************************************************************/

 /*
  * loescht alle Nachrichten aus dem Buffer, die bisher gefunden wurden
  */
static void
SD_CAN_331_ClearBuf(struct can_dev* dev)
{	
    struct can_can331_info* info=&dev->info.can331;
   canIoctl(info->hnd, NTCAN_IOCTL_FLUSH_RX_FIFO, NULL);
}
 
/*****************************************************************************/

/*
 * Vorhandene Geraete suchen
 *
 * returns:
 * 0 -> fehlerfrei
 * 1 -> Fehler aufgetreten
 */
static int
SD_CAN_331_Search_Dev(struct can_dev* dev)
{
    struct can_can331_info* info=&dev->info.can331;
    int diff, start, nodeID;
    int buf[1];
    long len;  
    DWORD ret;
   
    CMSG msg;
     
    buf[0]=disconnect;       
    
    /* force all devices to send log on messages */
    for(nodeID=0; nodeID<MAX_CAN_DEVICES; ++nodeID) {
        info->found_devices[nodeID].found=1; // enable sending     
        SD_CAN_331_Send_Short(dev, 0, nodeID, DataID_LogOn, 1, buf);
        info->found_devices[nodeID].found=0;
        
        usleep(100);
    }
      
    SD_CAN_331_ClearBuf(dev);

    start = (int) time(NULL);
    buf[0]=connect;       
  
    do {            
        /* find devices */ 
	ret = canTake(info->hnd, &msg, &len);
        if (ret==NTCAN_SUCCESS && len>0) {            
	  info->found_devices[REAL_ID(msg.id)].found = 1;
	  
	  // send a log on message                 
	  SD_CAN_331_Send_ID(dev, msg.id-1, DataID_LogOn, 1, buf);          
        }     
        diff = (int)time(NULL) - start; 
    } while( diff < MAX_SEARCH_SEC );
    
    return 0;
}

/*****************************************************************************/

/* 
 * CAN Nachricht laut Spezifikation 2.0A absenden
 * 
 * Identifier:
 * data_dir:    0 oder 1 -> (nicht-)gesetzt             ( ID0 )
 * ext_instr:   0 oder 1 -> (nicht-)gesetzt             ( ID1 )
 * nmt:         0 oder 1 -> (nicht-)gesetzt             ( ID2 )
 * id:          0 bis 63, je nach ID des Ziel Moduls    ( ID3 - ID8 )
 * active:      0 oder 1 siehe Manual                   ( ID9 )
 *
 * Nachricht:
 * data_id:     befehl der Nachricht (die DATA_ID)      
 * len:         Anzahl der Datenbytes in buf
 * buf:         enthaelt len datenbytes
 *
 * returns:
 * 0 -> fehlerfrei
 * 1 -> Fehler aufgetreten
 * 
 */
static int
SD_CAN_331_Send(struct can_dev* dev, int data_dir, int ext_instr, int nmt, 
		     int id, int active, int data_id, 
		     int len, int *buf) {   
         
    return SD_CAN_331_Send_ID(dev,
            (active<<9)|(id<<3)|(nmt<<2)|(ext_instr<<1)|data_dir,
            data_id, len, buf);
    
}

/*****************************************************************************/

 /*
  * Gibt eine Liste der gefundenen Geraete zurueck
  *
  * Parameter:
  * *anz -> enthaelt nach fehlerfreier Ausführung die Anzahl der gefundenen Geraete
  * *dev -> enthaelt alle gefundenen Geraete in aufsteigender Reihenfolge
  *
  * anz und dev werden nicht allokiert
  * dev muss Platz fuer MAX_CAN_DEVICES aufweisen
  *
  * returns:
  * 0 -> fehlerfrei
  * 1 -> Fehler aufgetreten
  */
static int
SD_CAN_331_Found_Dev(struct can_dev* cdev, int *anz, int *dev)
{
    struct can_can331_info* info=&cdev->info.can331;
    int nodeID;
    
    *anz = 0;
    
    for(nodeID=0; nodeID<MAX_CAN_DEVICES; ++nodeID) {
        if(info->found_devices[nodeID].found==1) {
        	dev[(*anz)++] = nodeID;
        } 
    }
    
    return 0;
}

/*****************************************************************************/

 /*
  * Liest die erste im Buffer verfuegbare Nachricht
  *
  * Parameter
  * msg enthaelt nach erfolgreichem Aufruf die Nachricht
  *
  * returns:
  * 0 -> fehlerfrei
  * 1 -> Fehler aufgetreten
  */
static int
SD_CAN_331_Read(struct can_dev* dev, TPCANRdMsg *msg)
{
    struct can_can331_info* info=&dev->info.can331;
    DWORD ret;
    long len;

    CMSG msg331;
   
    if( (ret = canTake(info->hnd, &msg331, &len)) == NTCAN_SUCCESS) {
      if(len != 0) {
	msg->Msg.ID = msg331.id;
	msg->Msg.MSGTYPE = MSGTYPE_STANDARD;
	msg->Msg.LEN = msg331.len;
	msg->Msg.DATA[0] = msg331.data[0];
	msg->Msg.DATA[1] = msg331.data[1];
	msg->Msg.DATA[2] = msg331.data[2];
	msg->Msg.DATA[3] = msg331.data[3];
	msg->Msg.DATA[4] = msg331.data[4];
	msg->Msg.DATA[5] = msg331.data[5];
	msg->Msg.DATA[6] = msg331.data[6];
	msg->Msg.DATA[7] = msg331.data[7];
      }
      else {
	return 1;
      }
    }
    else {
      return 1;
    }
        
    return 0;
} /* SD_CAN_331_Read(TPCANRdMsg *msg) */

/*****************************************************************************/
#if 0
 /*
  * sendet eine Nachricht und gibt die erste Medlung des angeschriebenen Moduls zurueck
  * sonstige Nachrichten im Buffer verfallen
  *
  * Parameter
  * msg enthaelt nach erfolgreichem Aufruf die Nachricht
  *
  * returns:
  * 0 -> fehlerfrei
  * 1 -> Fehler aufgetreten
  * 2 -> Geraet zu dem gesendet werden soll wurde zuvor nicht gefunden
  * 3 -> Zeitueberschreitung
  */
static int
SD_CAN_331_ReadWrite_ID(struct can_dev* dev, int identifier, int data_id,
        int len, int *buf, TPCANRdMsg *msg) {
    int ret;
    int sID;
    int start;
    int sendagain = 1;
    
    SD_CAN_331_ClearBuf(dev);
    ret = SD_CAN_331_Send_ID(dev, identifier, data_id, len, buf);
    if(ret != 0) {
    	return ret;
    }
    
    msg->Msg.ID = -1;
    sID = REAL_ID(identifier);
    start = (int) time(NULL);
    do {
    	ret = SD_CAN_331_Read(dev, msg);
        if(ret == 0 && REAL_ID(msg->Msg.ID) == sID && msg->Msg.DATA[0] == data_id) {
    		return 0;
    	}
        if((int) time(NULL) - start > MAX_WAIT_SEC) {
        	return 3;
        }
        if((int) time(NULL) - start > MAX_WAIT_SEC/2 && sendagain) {
	  sendagain = 0;
	  /* erneut senden, falls erste Nachricht nicht angekommen ist */
	  ret = SD_CAN_331_Send_ID(dev, identifier, data_id, len, buf);
	  if(ret != 0) {
	    return ret;
	  }
        }
    } while(1);
    
    return 1;
} /* SD_CAN_331_ReadWrite_ID */ 
#endif
/*****************************************************************************/
#if 0
static int
SD_CAN_331_ReadWrite_Short(struct can_dev* dev, int data_dir, int id,
        int data_id, int len, int *buf, TPCANRdMsg *msg)
{
    return SD_CAN_331_ReadWrite_ID(dev, (1<<9)|(id<<3)|data_dir, data_id, len,
            buf, msg);
} 
#endif
/*****************************************************************************/
#if 0
static int SD_CAN_331_ReadWrite(struct can_dev* dev, int data_dir,
        int ext_instr, int nmt, int id, int active, int data_id, int len,
        int *buf, TPCANRdMsg *msg)
{   
    return SD_CAN_331_ReadWrite_ID(dev,
            (active<<9)|(id<<3)|(nmt<<2)|(ext_instr<<1)|data_dir,
            data_id, len, buf, msg);
}
#endif
/*****************************************************************************/
static errcode
can_done_can331(struct generic_dev* gdev)
{
    struct can_dev* dev=(struct can_dev*)gdev;
    struct can_can331_info* info=(struct can_can331_info*)dev->info;

    close (info->hnd); /* ??? */

    free(info);

    return OK;
}
/*****************************************************************************/
errcode
can_init_can331(char* pathname, struct can_dev* dev)
{
    struct can_can331_info* info;
    errcode res;

printf("== lowlevel/can/can331/can331.c:can_init_can331 ==\n");
    printf("can_init_can331: path=%s\n", pathname);

    dev->info=calloc(1, sizeof(struct can_can331_info));
    if (!dev->info) {
        printf("can_init_can331: calloc(info): %s\n", strerror(errno));
        return Err_System;
    }
    info=(struct can_can331_info*)dev->info;

    res=SD_CAN_331_Init(dev);
    switch (res) {
    case 0:
        printf("can_init_can331(%s): successfull\n", pathname);
        break;
    case 1:
    case 2:
    case 3:
    default:
        printf("can_init_can331(%s): res=%d\n", pathname, res);
        return Err_System;
    }

    dev->generic.done=can_done_can331;
    dev->CAN_Send_Short=SD_CAN_331_Send_Short;
    dev->CAN_Read=SD_CAN_331_Read;
    dev->CAN_Init=SD_CAN_331_Init;
    dev->CAN_Search_Dev=SD_CAN_331_Search_Dev;
    dev->CAN_ReadWrite_Short=0;
    dev->CAN_ReadWrite=0;
    dev->CAN_Found_Dev=SD_CAN_331_Found_Dev;
    dev->CAN_Send=SD_CAN_331_Send;

    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
