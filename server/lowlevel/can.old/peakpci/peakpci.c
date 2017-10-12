/*
 * lowlevel/can/peakpci/peakpci.c
 * 
 * $ZEL: peakpci.c,v 1.4 2006/08/04 19:10:04 wuestner Exp $
 * 
 * PEAK CANbus interface
 * 
 * Autor: Sebastian Detert
 * 
 * created Feb. 2006
 */

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <emsctypes.h>
#include <errorcodes.h>
/*#include "peakpci.h"*/
#include "can_peakpci.h"
#include "libpcan.h"

#define DataID_LogOn            0xd8    /* log on which send after power on
                                           from the device */
#define     disconnect          0x00
#define     connect             0x01

#define MAX_SEARCH_SEC 10
#define USE_BAUD CAN_BAUD_125K
#define MAX_WAIT_SEC 5

struct can_peakpci_info {
    HANDLE hnd;
};

#if 0
static int
SD_CAN_Send_Short(struct can_dev* dev, int data_dir, int id, int data_id, int len, int *buf);
static int
SD_CAN_Send_ID(struct can_dev* dev, int identifier,  int data_id, int len, int *buf);
static int
SD_CAN_Init(struct can_dev* dev, char* path);
static int
SD_CAN_Send(struct can_dev* dev, int data_dir, int ext_instr, int nmt, int id, int active, int data_id, int len, int *buf);
static int
SD_CAN_Found_Dev(struct can_dev* dev, int *anz, int **devv);
#endif

/* Berechnet aus dem data frame einer CAN Nachricht die zugehoerige ID des Slaves */
#define REAL_ID(msgID) ((msgID>>3)&0x3f)

 /*
  * CAN Nachricht senden (s.o.), hier mit fertigem Identifier
  *
  * returns:
  * 0 -> fehlerfrei
  * 1 -> Fehler aufgetreten
  * 2 -> Geraet zu dem gesendet werden soll, wurde beim Start nicht gefunden
  */
static int
SD_CAN_Send_ID(struct can_dev* dev, int identifier,  int data_id, int len,
    int *buf)
{
    struct can_peakpci_info* info=(struct can_peakpci_info*)dev->info;
    TPCANMsg msg;
    int i;

    if( dev->found_devices[REAL_ID(identifier)].found == 0) {
        printf("SD_CAN_Send_ID : device #%d NOT found\n", REAL_ID(identifier));
        return 2;
    }
        
    msg.ID = identifier;
    msg.MSGTYPE = MSGTYPE_STANDARD;
    msg.LEN = len + 1;    
    msg.DATA[0] = data_id;

    for (i=1; i<=len; ++i) {
        msg.DATA[i] = buf[i-1];
    }
    for(i=len+1; i<8; ++i) {
      msg.DATA[i] = 0;      
    }
    
    CAN_Write(info->hnd, &msg);
    
    return 0;
}
 
 /*
  * Standard CAN Nachricht senden
  * ext_instr = 0
  * nmt = 0
  * active = 0
  *
  * returns:
  * 0 -> fehlerfrei
  * 1 -> Fehler aufgetreten
  */
static int
SD_CAN_Send_Short(struct can_dev* dev, int data_dir, int id, int data_id,
int len, int *buf)
{
    return SD_CAN_Send_ID(dev, (1<<9)|(id<<3)|data_dir, data_id, len, buf);
}

/*
 * Vorhandene Geraete suchen
 *
 * returns:
 * 0 -> fehlerfrei
 * 1 -> Fehler aufgetreten
 */
static int
SD_CAN_Search_Dev(struct can_dev* dev) {
    struct can_peakpci_info* info=(struct can_peakpci_info*)dev->info;
    int diff, start, nodeID;
    int buf[1];  
    uint32_t ret;
    TPCANRdMsg tmsg;

    buf[0]=disconnect;       

    /* force all devices to send log on messages */
    for(nodeID=0; nodeID<MAX_CAN_DEVICES; ++nodeID) {
        dev->found_devices[nodeID].found=1; // enable sending                    
        SD_CAN_Send_Short(dev, 0, nodeID, DataID_LogOn, 1, buf);
        dev->found_devices[nodeID].found=0;
        
        usleep(100);
    }

    /* clear buffer */
    do {} while(LINUX_CAN_Read_Timeout(info->hnd, &tmsg, 2)==CAN_ERR_OK);

    start = (int) time(NULL);
    buf[0]=connect;       

    do {        
        u_int32_t status;

        /* find devices */
        ret=LINUX_CAN_Read_Timeout(info->hnd, &tmsg, 2);
        if (ret==CAN_ERR_OK) {
            status = CAN_Status(info->hnd);
            if ((int)status < 0) {
                return 1;               
            }
            else {
                dev->found_devices[REAL_ID(tmsg.Msg.ID)].found = 1;
                                
                // send a log on message                 
                SD_CAN_Send_ID(dev, tmsg.Msg.ID-1, DataID_LogOn, 1, buf);
            }              
        }     
        diff = (int)time(NULL) - start; 
    } while( diff < MAX_SEARCH_SEC );
    
    return 0;
} /* int SD_CAN_Search_Dev() */

/*
 * Initialisieren des CAN Handles
 *
 * returns:
 * 0 -> fehlerfrei
 * 1 -> LINUX_CAN_Open fehlgeschlagen
 * 2 -> CAN_Init fehlgeschlagen
 */
static int
SD_CAN_Init(struct can_dev* dev, char* path) {
    struct can_peakpci_info* info=(struct can_peakpci_info*)dev->info;

    if( (info->hnd=LINUX_CAN_Open(path, O_NONBLOCK)) == NULL ) {
        return 1;
    }
    
    if( CAN_Init(info->hnd, USE_BAUD, CAN_INIT_TYPE_EX) != 0 ) {
    	return 2;   
    }
    
    return 0;
    
} /* int SD_CAN_Init(void) */

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
SD_CAN_Send(struct can_dev* dev, int data_dir, int ext_instr, int nmt, int id,
    int active, int data_id, int len, int *buf)
{   
    return SD_CAN_Send_ID(dev,
            (active<<9)|(id<<3)|(nmt<<2)|(ext_instr<<1)|data_dir,
            data_id, len, buf);
}

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
SD_CAN_Found_Dev(struct can_dev* dev, int *anz, int **devv)
{
   int nodeID;

    *anz = 0;
    
    for(nodeID=0; nodeID<MAX_CAN_DEVICES; ++nodeID) {
        if(dev->found_devices[nodeID].found==1) {
        	*(devv[(*anz)++]) = nodeID;
        } 
    }
    
    return 0;
 } /* int SD_CAN_Found_Dev(int *anz, int **dev) */

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
SD_CAN_Read(struct can_dev* dev, TPCANRdMsg *msg) {
  DWORD ret;
  int status;
  struct can_peakpci_info* info=(struct can_peakpci_info*)dev->info;
        
    ret=LINUX_CAN_Read_Timeout(info->hnd, msg, 5);
    if (ret!=CAN_ERR_OK) {
        /* printf("LINUX_CAN_Read_Timeout returns %d\n", ret); */
    	return 1;
    }
    
    status = CAN_Status(info->hnd);
    if ((int)status < 0) {
        /* printf("CAN_STATUS = %d\n", (int)status); */
    	return 1;
    }
    
    return 0;
 } /* SD_CAN_Read(TPCANRdMsg *msg) */
 
 /*
  * loescht alle Nachrichten aus dem Buffer, die bisher gefunden wurden
  */
static void
SD_CAN_ClearBuf(struct can_dev* dev) {
	TPCANRdMsg tmsg;
	struct can_peakpci_info* info=(struct can_peakpci_info*)dev->info;
    
 	do {} while(LINUX_CAN_Read_Timeout(info->hnd, &tmsg, 5)==CAN_ERR_OK);
 }
 
 /*
  * sendet eine Nachricht und gibt die erste Meldung des angeschriebenen Moduls
  * zurueck
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
SD_CAN_ReadWrite_ID(struct can_dev* dev, int identifier, int data_id,
        int len, int *buf, TPCANRdMsg *msg) {
    int ret;
    int sID;
    int start;
    int sendagain = 1;
    
    SD_CAN_ClearBuf(dev);
    ret = SD_CAN_Send_ID(dev, identifier, data_id, len, buf);
    if(ret != 0) {
    	return ret;
    }
    
    msg->Msg.ID = -1;
    sID = REAL_ID(identifier);
    start = (int) time(NULL);
    do {
    	ret = SD_CAN_Read(dev, msg);
        if(ret == 0 && REAL_ID(msg->Msg.ID) == sID && msg->Msg.DATA[0] == data_id) {
    		return 0;
    	}
        if((int) time(NULL) - start > MAX_WAIT_SEC) {
        	return 3;
        }
        if((int) time(NULL) - start > MAX_WAIT_SEC/2 && sendagain) {
	  sendagain = 0;
	  /* erneut senden, falls erste Nachricht nicht angekommen ist */
	  ret = SD_CAN_Send_ID(dev, identifier, data_id, len, buf);
	  if(ret != 0) {
	    return ret;
	  }
        }
    } while(1);
    
    return 1;
} /* SD_CAN_ReadWrite_ID */ 
 
static int
SD_CAN_ReadWrite_Short(struct can_dev* dev, int data_dir, int id,
        int data_id, int len, int *buf, TPCANRdMsg *msg)
{
        return SD_CAN_ReadWrite_ID(dev, (1<<9)|(id<<3)|data_dir, data_id, len,
                buf, msg);
} /* SD_CAN_ReadWrite_Short */
 
static int
SD_CAN_ReadWrite(struct can_dev* dev, int data_dir, int ext_instr, int nmt, int id, int active, int data_id, int len, int *buf, TPCANRdMsg *msg) {   
    return SD_CAN_ReadWrite_ID(dev, (active<<9)|(id<<3)|(nmt<<2)|(ext_instr<<1)|data_dir, data_id, len, buf, msg);
 } /* int SD_CAN_ReadWrite( ... ) */
/*****************************************************************************/
static errcode
can_done_peakpci(struct generic_dev* gdev)
{
    struct can_dev* dev=(struct can_dev*)gdev;
    struct can_peakpci_info* info=(struct can_peakpci_info*)dev->info;

    CAN_Close(info->hnd);
    free(info);
    dev->info=0;

    return OK;
}
/*****************************************************************************/
errcode
can_init_peakpci(char* pathname, struct can_dev* dev)
{
    struct can_peakpci_info* info;
    errcode res;

    info=(struct can_peakpci_info*) calloc(1, sizeof(struct can_peakpci_info));
    if (!info) {
        printf("can_init_peakpci: calloc(info): %s\n", strerror(errno));
        return Err_NoMem;
    }
    dev->info=info;

    res=SD_CAN_Init(dev, pathname);
    switch (res) {
    case 0:
        printf("can_init_peakpci(%s): successfull\n", pathname);
        break;
    case 1:
    case 2:
    case 3:
    default:
        printf("can_init_peakpci(%s): res=%d\n", pathname, res);
        return Err_System;
    }

    dev->generic.done=can_done_peakpci;
    dev->CAN_Send=SD_CAN_Send;
    dev->CAN_Send_ID=SD_CAN_Send_ID;
    dev->CAN_Send_Short=SD_CAN_Send_Short;
    dev->CAN_Read=SD_CAN_Read;
    dev->CAN_ClearBuf=SD_CAN_ClearBuf;
    dev->CAN_Search_Dev=SD_CAN_Search_Dev;
    dev->CAN_ReadWrite_Short=SD_CAN_ReadWrite_Short;
    dev->CAN_ReadWrite_ID=SD_CAN_ReadWrite_ID;
    dev->CAN_ReadWrite=SD_CAN_ReadWrite;
    dev->CAN_Found_Dev=SD_CAN_Found_Dev;
    dev->generic.online=1;

    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
