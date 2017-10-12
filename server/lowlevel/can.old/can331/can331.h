/***********************************************
* $ZEL: can331.h,v 1.1 2006/09/15 17:50:56 wuestner Exp $
*
* Funktionen fuer das High Voltage Power Supply von iseg
*
* Autor: Sebastian Detert
*
* Datum: Feb.06
*        Apr.06 support fuer cpci can331
*
************************************************/
#if 0

#ifndef _can331_h_
#define _can331_h_

#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include <errno.h>

#include "pcan.h"
#include "ican.h"
#include "ntcan.h"

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

/* CAN einstellungen */
#ifndef CAN_NET
#define CAN_NET 0
#endif
#ifndef CAN_MODE
#define CAN_MODE 0
#endif
#ifndef CAN_TXQUEUESIZE
#define CAN_TXQUEUESIZE 10
#endif
#ifndef CAN_RXQUEUESIZE
#define CAN_RXQUEUESIZE 100
#endif
#ifndef CAN_TXTIMEOUT
#define CAN_TXTIMEOUT 2000
#endif
#ifndef CAN_RXTIMEOUT
#define CAN_RXTIMEOUT 5000
#endif

/* Baudrate */
#ifndef USE_BAUD
#define USE_BAUD 6 /* 125 kbit/s */
#endif

#if 0
/*
 * Vorhandene Geraete suchen
 *
 * returns:
 * 0 -> fehlerfrei
 * 1 -> Fehler aufgetreten
 */
int SD_CAN_331_Search_Dev(void);
#endif

#if 0
/*
 * Initialisieren des CAN Handles
 *
 * returns:
 * 0 -> fehlerfrei
 * 1 -> Fehler aufgetreten
 */
int SD_CAN_331_Init(void);
#endif

#if 0
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
 int SD_CAN_331_Send(int data_dir, int ext_instr, int nmt, int id, int active, int data_id, int len, int *buf);
#endif

#if 0
 /*
  * CAN Nachricht senden (s.o.), heir mit fertigem Identifier
  *
  * returns:
  * 0 -> fehlerfrei
  * 1 -> Fehler aufgetreten
  * 2 -> Geraet zu dem gesendet werden soll, wurde beim Start nicht gefunden
  */
 int SD_CAN_331_Send_ID(int identifier,  int data_id, int len, int *buf);
#endif

#if 0
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
 int SD_CAN_331_Send_Short(int data_dir, int id, int data_id, int len, int *buf);
#endif

#if 0
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
 int SD_CAN_331_Found_Dev(int *anz, int **dev);
#endif
 
#if 0
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
 int SD_CAN_331_Read(TPCANRdMsg *msg);
#endif

#if 0
 /*
  * loescht alle Nachrichten aus dem Buffer, die bisher gefunden wurden
  */
 void SD_CAN_331_ClearBuf(void);
#endif
 
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
 int SD_CAN_331_ReadWrite_ID(int identifier, int data_id, int len, int *buf, TPCANRdMsg *msg);
 int SD_CAN_331_ReadWrite_Short(int data_dir, int id, int data_id, int len, int *buf, TPCANRdMsg *msg); 
 int SD_CAN_331_ReadWrite(int data_dir, int ext_instr, int nmt, int id, int active, int data_id, int len, int *buf, TPCANRdMsg *msg);
#endif

#endif
#endif /* if 0 */
