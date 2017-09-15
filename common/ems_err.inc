dnl $Id: ems_err.inc,v 2.2 1998/05/18 12:02:54 wuestner Exp $
dnl
dnl ems_err.inc
dnl
dnl 03.08.94
dnl changed: 14.05.1998
dnl

emse_code(EMSE_NotInitialised,clientcomm_init nicht ausgefuehrt)
emse_code(EMSE_NoMoreVEDs    ,keine VED-Handle mehr)
emse_code(EMSE_IllVEDHandle  ,ungueltiges VED-Handle)
emse_code(EMSE_VEDdied       ,VED gestorben)

emse_code(EMSE_VEDOpen       ,VED ist schon offen)
emse_code(EMSE_VEDNotOpen    ,VED ist nicht offen)

emse_code(EMSE_UnknownVED    ,unbekannter VED-Name)
emse_code(EMSE_UnknownVEDVer ,unbekannte VED-Version)
emse_code(EMSE_HostUnknown   ,Host unbekannt)

emse_code(EMSE_Proto         ,Protokollfehler (intern))
emse_code(EMSE_IntVersion    ,falsche Version (ver_intmsgtab))
emse_code(EMSE_UnsolVersion  ,falsche Version (ver_unsolmsgtab))
emse_code(EMSE_Version       ,falsche Version (ver_msgtab))

emse_code(EMSE_Range         ,Wert nicht im zulaessigen Bereich)
emse_code(EMSE_IllService    ,Dienst unzulaessig)
emse_code(EMSE_System        ,Fehler bei Systemaufruf)
emse_code(EMSE_NotImpl       ,Dienst (noch) nicht implementiert)
emse_code(EMSE_Initialised   ,clientcomm_init bereits ausgefuehrt)
emse_code(EMSE_Unknown       ,unbekannter Fehler)
emse_code(EMSE_CommMem       ,kein Speicher mehr fuer Kommunikation)
emse_code(EMSE_NotReentrant  ,Function ist nicht reentrant)
emse_code(EMSE_InProgress    ,Bearbeitung erfolgt; Meldung kommt spaeter)
emse_code(EMSE_CommuTimeout  ,no response from commu)
