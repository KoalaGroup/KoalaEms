/*
 * lowlevel/can/cpci_can331/cpci_can331.c
 * 
 * $ZEL: cpci_can331.c,v 1.8 2006/08/04 19:09:55 wuestner Exp $
 * 
 * esd331 CAN Bus interface
 * 
 * Autor: Sebastian Detert
 * 
 * created Feb. 2006 Support fuer can331
 *         Mai. 2006 allgemeinere Schnittstellen
 */

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <emsctypes.h>
#include <errorcodes.h>
/*#include "cpci_can331.h"*/
#include "can_esd331.h"
#include "ntcan.h"

/* these definitions belong to ISEG HV but are used inside cpci_can331.c */
#define DataID_LogOn              0xd8    // log on which send after power on from the device
#define     disconnect          0x00
#define     connect             0x01

/* this definition belongs to PEAK PCI but is used inside cpci_can331.c */
#define MSGTYPE_STANDARD      0x00     // marks a standard frame

#define MAX_SEARCH_SEC 15

#define CAN_NET 0
#define CAN_MODE 0
#define CAN_TXQUEUESIZE 10
#define CAN_RXQUEUESIZE 100
#define CAN_TXTIMEOUT 2000
#define CAN_RXTIMEOUT 5000

#define USE_BAUD 6 /* 125 kbit/s */
#define MAX_WAIT_SEC 5

struct can_esd331_info {
  NTCAN_HANDLE hnd;
};

/* Berechnet aus dem data frame einer CAN Nachricht die zugehoerige
 ID des Slaves */
#define REAL_ID(msgID) ((msgID>>3)&0x3f)

#define SLEEP(arg) usleep(arg)

#ifndef CANDEBUG
#define CANDEBUG 0
#endif

 /*
  * loescht alle Nachrichten aus dem Buffer, die bisher gefunden wurden
  */
static void
SD_CAN_331_ClearBuf(struct can_dev* dev) {
   struct can_esd331_info* info=(struct can_esd331_info*)dev->info;	
   canIoctl(info->hnd, NTCAN_IOCTL_FLUSH_RX_FIFO, NULL);
 }
 
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
  struct can_esd331_info* info=(struct can_esd331_info*)dev->info;
  CMSG msg;
  int i;
  int32_t leins = 1;

  if(CANDEBUG) printf("SD_CAN_331_Send_ID ");

  if( dev->found_devices[REAL_ID(identifier)].found == 0) {
    /* printf("SD_CAN_331_Send_ID : device #%d NOT found\n", REAL_ID(identifier)); */
    if(CANDEBUG) printf("SD_CAN_331_Send_ID done\n");
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
    if(CANDEBUG) printf("SD_CAN_331_Send_ID done\n");  
    return 3;
  }
  
  if(CANDEBUG) printf("SD_CAN_331_Send_ID done\n");
  return 0;
} /* int SD_CAN_331_Send_ID( ... ) */

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
    struct can_esd331_info* info=(struct can_esd331_info*)dev->info;
    int diff, start, nodeID;
    int buf[1];
    int32_t len;  
    uint32_t ret;
   
    CMSG msg;

    if(CANDEBUG) printf("SD_CAN_331_Search_Dev ");
     
    buf[0]=disconnect;       

    SD_CAN_331_ClearBuf(dev);

    /* force all devices to send log on messages */
    for(nodeID=0; nodeID<MAX_CAN_DEVICES; ++nodeID) {
        dev->found_devices[nodeID].found=1; // enable sending     
        SD_CAN_331_Send_Short(dev, 0, nodeID, DataID_LogOn, 1, buf);
        dev->found_devices[nodeID].found=0;
        
        usleep(10000);  
    }
      

    start = (int) time(NULL);
    buf[0]=connect;       
  
    do {            
        /* find devices */ 
	ret = canTake(info->hnd, &msg, &len);
	if(ret != 0 || len != 0)
	  printf("%d %d;",ret,len);
        if (ret==NTCAN_SUCCESS && len>0) {            
	  dev->found_devices[REAL_ID(msg.id)].found = 1;
	  
	  // send a log on message                 
	  SD_CAN_331_Send_ID(dev, msg.id-1, DataID_LogOn, 1, buf);          
        }     
        diff = (int)time(NULL) - start; 

	/*nochmal was senden */
	/*nodeID = (nodeID+1)%MAX_CAN_DEVICES;
	if(dev->found_devices[nodeID].found==0) {
	  dev->found_devices[nodeID].found=1; // enable sending     
	  SD_CAN_331_Send_Short(dev, 0, nodeID, DataID_LogOn, 1, buf);
	  dev->found_devices[nodeID].found=0;	  
        }*/
    } while( diff < MAX_SEARCH_SEC );
    
    if(CANDEBUG) printf("SD_CAN_331_Search_Dev done\n");

    return 0;
} /* int SD_CAN_331_Search_Dev() */

/*
 * Initialisieren des CAN Handles
 *
 * returns:
 * 0 -> fehlerfrei
 * 1 -> LINUX_CAN_Open fehlgeschlagen
 * 2 -> CAN_Init fehlgeschlagen
 */
static int
SD_CAN_331_Init(struct can_dev* dev) {
  struct can_esd331_info* info=(struct can_esd331_info*)dev->info;
  int ret;
  long i, j;
  uint32_t baudrate;
    
  if(CANDEBUG) printf("SD_CAN_331_Init ");

  if( (ret = canOpen(CAN_NET, CAN_MODE, CAN_TXQUEUESIZE, 
		     CAN_RXQUEUESIZE, CAN_TXTIMEOUT, CAN_RXTIMEOUT, 
		     &(info->hnd))) != NTCAN_SUCCESS )
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
     
  if(CANDEBUG) printf("SD_CAN_331_Init done\n");

  return 0;
    
} /* int SD_CAN_331_Init(void) */

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
    
 } /* int SD_CAN_331_Send( ... ) */
  

/*
 * Gibt eine Liste der gefundenen Geraete zurueck
 *
 * Parameter:
 * *anz -> enthaelt nach fehlerfreier Ausfuehrung die Anzahl der gefundenen
 *         Geraete
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
SD_CAN_331_Found_Dev(struct can_dev* dev, int *anz, int **devv)
{
  int nodeID;

  *anz = 0;
  
  if(CANDEBUG) printf("SD_CAN_331_Found_Dev ");

  for(nodeID=0; nodeID<MAX_CAN_DEVICES; ++nodeID) {
    if(dev->found_devices[nodeID].found==1) {
      *(devv[(*anz)++]) = nodeID;
    } 
  }
    
  if(CANDEBUG) printf("SD_CAN_331_Found_Dev done\n");

  return 0;
} /* int SD_CAN_331_Found_Dev(int *anz, int **dev) */

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
SD_CAN_331_Read(struct can_dev* dev, TPCANRdMsg *msg) {
    uint32_t ret;
    int32_t len;
    struct can_esd331_info* info=(struct can_esd331_info*)dev->info;

    CMSG msg331;
   
    if(CANDEBUG) printf("SD_CAN_331_Read ");

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
	if(CANDEBUG) printf("SD_CAN_331_Read done\n");
	return 1;
      }
    }
    else {
      if(CANDEBUG) printf("SD_CAN_331_Read done\n");
      return 1;
    }
        
    if(CANDEBUG) printf("SD_CAN_331_Read done\n");
    return 0;
 } /* SD_CAN_331_Read(TPCANRdMsg *msg) */
 
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
        int len, int *buf, TPCANRdMsg *msg)
{
    int ret;
    int sID;
    int start;
    int sendagain = 1;
    
    if(CANDEBUG) printf("SD_CAN_331_ReadWrite_ID ");

    SD_CAN_331_ClearBuf(dev);
    ret = SD_CAN_331_Send_ID(dev, identifier, data_id, len, buf);
    if(ret != 0) {
      if(CANDEBUG) printf("SD_CAN_331_ReadWrite_ID done\n");
    	return ret;
    }
    
    msg->Msg.ID = -1;
    sID = REAL_ID(identifier);
    start = (int) time(NULL);
    do {
    	ret = SD_CAN_331_Read(dev, msg);
        if(ret == 0 && REAL_ID(msg->Msg.ID) == sID && msg->Msg.DATA[0] == data_id) {
	  if(CANDEBUG) printf("SD_CAN_331_ReadWrite_ID done\n");
    		return 0;
    	}
        if((int) time(NULL) - start > MAX_WAIT_SEC) {
	  if(CANDEBUG) printf("SD_CAN_331_ReadWrite_ID done\n");
        	return 3;
        }
        if((int) time(NULL) - start > MAX_WAIT_SEC/2 && sendagain) {
	  sendagain = 0;
	  /* erneut senden, falls erste Nachricht nicht angekommen ist */
	  ret = SD_CAN_331_Send_ID(dev, identifier, data_id, len, buf);
	  if(ret != 0) {
	    if(CANDEBUG) printf("SD_CAN_331_ReadWrite_ID done\n");
	    return ret;
	  }
        }
    } while(1);
    
    if(CANDEBUG) printf("SD_CAN_331_ReadWrite_ID done\n");
    return 1;
 } /* SD_CAN_331_ReadWrite_ID */ 
 
static int
SD_CAN_331_ReadWrite_Short(struct can_dev* dev, int data_dir, int id,
        int data_id, int len, int *buf, TPCANRdMsg *msg)
{
    return SD_CAN_331_ReadWrite_ID(dev,
            (1<<9)|(id<<3)|data_dir,
            data_id, len, buf, msg);
}
 
static int
SD_CAN_331_ReadWrite(struct can_dev* dev, int data_dir, int ext_instr, int nmt,
        int id, int active, int data_id, int len, int *buf, TPCANRdMsg *msg)
{
    return SD_CAN_331_ReadWrite_ID(dev,
            (active<<9)|(id<<3)|(nmt<<2)|(ext_instr<<1)|data_dir,
            data_id, len, buf, msg);
}
/*****************************************************************************/
static errcode
can_done_esd331(struct generic_dev* gdev)
{
    struct can_dev* dev=(struct can_dev*)gdev;
    struct can_esd331_info* info=(struct can_esd331_info*)dev->info;

    canClose(info->hnd);
    free(info);
    dev->info=0;

    return OK;
}
/*****************************************************************************/
errcode
can_init_esd331(char* pathname, struct can_dev* dev)
{
    struct can_esd331_info* info;
    errcode res;

    info=(struct can_esd331_info*) calloc(1, sizeof(struct can_esd331_info));
    if (!info) {
        printf("can_init_esd331: calloc(info): %s\n", strerror(errno));
        return Err_System;
    }
    dev->info=info;

    res=SD_CAN_331_Init(dev);
    switch (res) {
    case 0:
        printf("can_init_esd331(%s): successfull\n", pathname);
        break;
    case 1:
    case 2:
    case 3:
    default:
        printf("can_init_esd331(%s): res=%d\n", pathname, res);
        return Err_System;
    }

    dev->generic.done=can_done_esd331;
    dev->CAN_Send=SD_CAN_331_Send;
    dev->CAN_Send_ID=SD_CAN_331_Send_ID;
    dev->CAN_Send_Short=SD_CAN_331_Send_Short;
    dev->CAN_Read=SD_CAN_331_Read;
    dev->CAN_ClearBuf=SD_CAN_331_ClearBuf;
    dev->CAN_Search_Dev=SD_CAN_331_Search_Dev;
    dev->CAN_ReadWrite_Short=SD_CAN_331_ReadWrite_Short;
    dev->CAN_ReadWrite_ID=SD_CAN_331_ReadWrite_ID;
    dev->CAN_ReadWrite=SD_CAN_331_ReadWrite;
    dev->CAN_Found_Dev=SD_CAN_331_Found_Dev;

    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
