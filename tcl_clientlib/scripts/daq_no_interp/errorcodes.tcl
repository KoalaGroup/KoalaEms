# $Id: errorcodes.tcl,v 1.1 2000/08/31 15:43:07 wuestner Exp $
#   P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

array set ems_errcodes {
 0  {OK}
 1  {illegal request type}
 2  {request is not implemented}

 3  {wrong number of arguments}
 4  {argument outside of valid range}

 5  {object type unknown or not implemented}
 6  {object can't be deleted}
 7  {object is already defined}

 8  {invalid variable index}
 9  {variable not defined}
10  {variable already defined}
11  {variable size does not match}

12  {illegal domain index}
13  {domain does not exist}
14  {domain already defined}
15  {unknown domain type}

16  {illegal instrumentationsystem index}
17  {instrumentation system not defined}
18  {instrumentation system already defined}
19  {no instrumentation system modullist}
20  {no readoutlist for this trigger number}
21  {illegal trigger pattern}

22  {invalid index for program invocation}
23  {program invocation not defined}
24  {program invocation already defined}
25  {unknown program invocation type}
26  {program invocation is active}
27  {program invocation is not active}
28  {no trigger information loaded}
29  {unknown trigger procedure}

30  {capability type not available}
31  {error during test of procedure list}
32  {error during execution of procedure list}

33  {tape already open}
34  {tape not open}
35  {output device is busy}
36  {tape already enabled}
37  {tape not enabled}

38  {not enough memory}
39  {buffer overflow}
40  {VED-ID not known}
41  {error in system call}

42  {program error}
43  {invalid dataout index}
44  {dataout object does not exist}
45  {error in the trigger procedure or its arguments}
45  {other error}
46  {hardware test failure}
47  {hardware failure}
48  {bad modul type}
49  {not owner of object}
50  {addresstype not implemented}
51  {buffertype not implemented}
}

array set ems_plcodes {
 0 plOK
 1 {falscher Prozedur-ID}
 2 {mehr Argumente als die Readoutliste lang ist}
 3 {falsche Anzahl der Argumente}
 4 {ein Argument liegt nicht im zulaessigen Bereich}
 5 {Falscher Variablenindex}
 6 {Variable nicht definiert}
 7 {Variable hat falsche Groesse}
 8 {zu wenig Speicher}
 9 {unzulaessige Hardware-Adresse}
 10 {Modulliste fuer das IS fehlt}
 11 {Modulliste unpassend}
 12 {unzulaessige Adresse in der Modulliste}
 13 {Modultyp nicht gefunden}
 14 {Falscher Modultyp}
 15 {Hardwaretest ist fehlgeschlagen}
 16 {Fehler in der Hardware}
 17 {Variable ist schon definiert}
 18 {anderer Fehler}
 19 {Fehler in Rekursion}
 20 {Fehler bei Systemaufruf}
 21 {Programmfehler}
 22 {Geraet ist beschaeftigt}
}

array set ems_rtcodes {
 0 rtOK
 1 {Fehler im Ausgabegeraet}
 2 {Fehler bei der Triggersynchronisation}
 3 {Falsche Eventnummer}
 4 {Fehler beim Ausfuehren einer Prozedurliste}
 5 {anderer Fehler}
 6 {Fehler im Eingabegeraet}
}

proc ems_errcode {code} {
  global ems_errcodes

  if [info exists ems_errcodes($code)] {
    return $ems_errcodes($code)
  } else {
    return [format {unknown ems errorcode %d} $code]
  }
}

proc ems_plcode {code} {
  global ems_plcodes

  if [info exists ems_plcodes($code)] {
    return $ems_plcodes($code)
  } else {
    return [format {unknown ems plcode %d} $code]
  }
}

proc ems_rtcode {code} {
  global ems_rtcodes

  if [info exists ems_rtcodes($code)] {
    return $ems_rtcodes($code)
  } else {
    return [format {unknown ems rtcode %d} $code]
  }
}
