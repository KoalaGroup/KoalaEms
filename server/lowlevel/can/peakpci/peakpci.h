/***********************************************
*
* Funktionen fuer das High Voltage Power Supply von iseg
*
* Autor: Sebastian Detert
*
* Datum: Feb.06
*        Mai.06
*
* $ZEL: peakpci.h,v 1.3 2006/08/04 19:10:04 wuestner Exp $
*
************************************************/

#ifndef _peakpci_h_
#define _peakpci_h_

#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "libpcan.h"
#include "../canbus.h"

/* extracted from ican.h (iseg_HV) */
#define DataID_LogOn            0xd8    /* log on which send after power on
                                           from the device */
#define     disconnect          0x00
#define     connect             0x01

#if 0
/* maximale geraete anzahl */
#ifndef MAX_CAN_DEVICES
#define MAX_CAN_DEVICES 64
#endif

/* maximale suchzeit */
#ifndef MAX_SEARCH_SEC
#define MAX_SEARCH_SEC 10
#endif

/* maximale wartezeit auf antwort */
#ifndef MAX_WAIT_SEC
#define MAX_WAIT_SEC 5
#endif

/* Baudrate */
#ifndef USE_BAUD
#define USE_BAUD CAN_BAUD_125K
#endif

/* Pfad zur Can Schnittstelle */
#ifndef CAN_DEV
#define CAN_DEV "/dev/pcan0"
#endif

/*
 * Vorhandene Geraete suchen
 *
 * returns:
 * 0 -> fehlerfrei
 * 1 -> Fehler aufgetreten
 */
 int SD_CAN_Search_Dev(struct can_dev* dev);

/*
 * Initialisieren des CAN Handles
 *
 * returns:
 * 0 -> fehlerfrei
 * 1 -> Fehler aufgetreten
 */
 int SD_CAN_Init(struct can_dev* dev);

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
  int SD_CAN_Send(struct can_dev* dev, int data_dir, int ext_instr, int nmt, int id, int active, int data_id, int len, int *buf);
 
 /*
  * CAN Nachricht senden (s.o.), heir mit fertigem Identifier
  *
  * returns:
  * 0 -> fehlerfrei
  * 1 -> Fehler aufgetreten
  * 2 -> Geraet zu dem gesendet werden soll, wurde beim Start nicht gefunden
  */
  int SD_CAN_Send_ID(struct can_dev* dev, int identifier,  int data_id, int len, int *buf);
 
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
  int SD_CAN_Send_Short(struct can_dev* dev, int data_dir, int id, int data_id, int len, int *buf);
 
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
 int SD_CAN_Found_Dev(struct can_dev* dev, int *anz, int **devv);
 
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
 int SD_CAN_Read(struct can_dev* dev, TPCANRdMsg *msg);
 
 /*
  * loescht alle Nachrichten aus dem Buffer, die bisher gefunden wurden
  */
 void SD_CAN_ClearBuf(struct can_dev* dev);
 
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
 int SD_CAN_ReadWrite_ID(struct can_dev* dev, int identifier, int data_id, int len, int *buf, TPCANRdMsg *msg);
 int SD_CAN_ReadWrite_Short(struct can_dev* dev, int data_dir, int id, int data_id, int len, int *buf, TPCANRdMsg *msg); 
 int SD_CAN_ReadWrite(struct can_dev* dev, int data_dir, int ext_instr, int nmt, int id, int active, int data_id, int len, int *buf, TPCANRdMsg *msg);

#endif
#endif
