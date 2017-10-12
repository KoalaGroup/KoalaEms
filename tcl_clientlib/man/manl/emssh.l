.\" This manpage source uses rsml coding.
.so /usr/share/lib/tmac/sml
.so /usr/share/lib/tmac/rsml
.\"
.\"
.\" *********************************************************************
.\" *                                                                   *
.\" *      Forschungszentrum Jülich GmbH                                *
.\" *      Zentrallabor für Elektronik                                  *
.\" *      Peter Wüstner                                                *
.\" *                                                                   *
.\" *********************************************************************
.\"
.TH emssh 1
.SH NAME
.PP
\*Lemssh\*O \- ems shell
.SH SYNOPSIS
.PP
.sS
\*Lemssh\*O file
.sE
.SH PARAMETERS
.PP
.VL 4m
.LI "\*Vfile\*O"
zu interpretierender Quellfile
.LE
.SH DESCRIPTION
.PP
\*Lemssh\*O ist eine window shell (\*Lwish\*O) mit Erweiterungen für EMS.
Anfänglich gibt es fünf zusaetzliche Kommandos:
.VL 4m
.LI "\*Lems_connect"
stellt die Verbindung zu einem Kommunikationsprozeß her
.LI "\*Lems_disconnect"
beendet die Verbindung zu einem Kommunikationsprozeß
.LI "\*Lems_connected"
testet die Existenz einer Verbindung zum Kommunikationsprozeß
.LI "\*Lems_connection"
beschreibt die Verbindung zum Kommunikationsprozeß
.LI "\*Lems_veds"
listet alle bekannten VEDs
.LI "\*Lems_open"
öffnet ein VED
.LE
.PP
Das Öffnen eines VED (\*Lems_open\*O) erzeugt ein weiteres Kommando mit
folgenden Unterkommandos:
.VL 4m
.LI "\*L<VED> close"
schließt das VED
.LI "\*L<VED> closecommand\*O"
legt ein TCL-Kommando fest, das vor dem Schließen des VED ausgeführt wird
.LI "\*L<VED> name\*O"
liefert den VED-Namen
.LI "\*L<VED> type\*O"
liefert den Typ des VED
.LI "\*L<VED> reset\*O"
führt resetved aus
.LI "\*L<VED> var\*O"
eine Gruppe von Kommandos zur Manipulation von EMS-Variablen
.LI "\*L<VED> identify\*O"
liefert (mehr oder weniger nützliche) Informationen zu einem VED
.LI "\*L<VED> namelist\*O"
liefert die Liste definierter Objekte eines VED
.LI "\*L<VED> unsolcommand\*O"
installiert callback für unsolicited Messages
.LI "\*L<VED> event\*O (upload)"
liefert das letzte Event
.LE
.PP
Für VEDs des Typs controller gibt es zusätzlich folgende Unterkommandos:
.VL 4m
.LI "\*L<VED> caplist\*O"
liefert die Liste der lokalen Prozeduren
.LI "\*L<VED> version_separator\*O"
setzt den Versions-Separator für \*Lcaplist\*O und \*Lcommand\*O
.LI "\*L<VED> is\*O"
eine Gruppe von Kommandos zur Manipulation von Instrumentierungssystemen
.LI "\*L<VED> modullist\*O"
Kommandos zum Manipulieren der Modulliste
.LI "\*L<VED> command\*O"
führt eine EMS-Prozedurliste aus
.LI "\*L<VED> command1\*O"
führt eine einzelne EMS-Prozedur aus
.LI "\*L<VED> trigger\*O (create delete upload)"
installiert oder löscht die Triggerprozedur
.LE
.PP
Für VEDs des Typs eventmanager gibt es zusätzlich folgende Unterkommandos:
.VL 4m
.LI "\*L<VED> datain\*O (create delete upload)"
Kommandos zum Manipulieren eines Datain-Objektes
.LI "\*L<VED> dataout\*O (create delete upload wind write)"
Kommandos zum Manipulieren eines Dataout-Objektes
.LE
.PP
VEDs des Typs camac haben zusätzlich zu den controller-Unterkommandos:
.VL 4m
.LI "\*L<VED> CCCC\*O"
CAMAC clear
.LI "\*L<VED> CCCI\*O"
CAMAC inhibit
.LI "\*L<VED> CCCZ\*O"
CAMAC zero
.LI "\*L<VED> CFSA\*O"
CAMAC read oder write
.LI "\*L<VED> CFUBC\*O"
CAMAC blockbefehl
.LE
.PP
VEDs des Typs fastbus haben zusätzlich zu den controller-Unterkommandos:
.VL 4m
.LI "\*L<VED> FWD\*O"
Fastbus write, data space
.LI "\*L<VED> FRD\*O"
Fastbus read, data space
.LI "\*L<VED> FWC\*O"
Fastbus write, control space
.LI "\*L<VED> FRC\*O"
Fastbus read, control space
.LI "\*L<VED> FWDB\*O"
Fastbus block write, data space
.LI "\*L<VED> FRDB\*O"
Fastbus block read, data space
.LI "\*L<VED> FWCB\*O"
Fastbus block write, control space
.LI "\*L<VED> FRCB\*O"
Fastbus block read, control space
.LE
.PP
VEDs des Typs chimaere vereinigen die Unterkommandos von controller und
eventmanager, datain fehlt ihnen. Entsprechendes gilt für
camac_chimaere und fastbus_chimaere.
.PP
Nach Erzeugen eines Instrumentierungssystems
(\*V<VED> is_create\*O oder \*V<VED> is_open\*O)
stehen folgende Instrumentierungssystemkommandos zur Verfügung:
.VL 4m
.LI "\*L<IS> close\*O"
beendet die Verbindung zu einem Instrumentierungssystem
.LI "\*L<IS> delete\*O"
löscht ein Instrumentierungssystem
.LI "\*L<IS> lam\*O (open create delete)"
erzeugt und löscht Programinvocations des Typs LAM
.LI "\*L<IS> readout\*O (open)"
schafft eine Verbindung zu Programinvocations des Types readout
.LI "\*L<IS> memberlist\*O (load delete read)
Kommandos zum Manipulieren der Memberliste
.LI "\*L<IS> readoutlist\*O (download upload delete)
Kommandos zum Manipulieren der Readoutprozeduren
.LI "\*L<IS> enable\*O"
enabled und disabled Instrumentierungssysteme
.LI "\*L<IS> command\*O"
fhrt eine EMS-Prozedurliste aus
.LI "\*L<IS> command1\*O"
fhrt eine einzelne EMS-Prozedur aus
.LE
.PP
\*L<IS> lam\*O und \*L<IS> readout\*O erzeugen weitere Kommandos:
.VL 4m
.LI "\*L<PI> delete\*O"
löscht eine Programinvocation
.LI "\*L<PI> close\*O"
beendet die Verbindung zu einer Programinvocation
.LI "\*L<PI> start\*O"
startet die Programinvocation
.LI "\*L<PI> stop\*O"
hält die Programinvocation vorübergehend an
.LI "\*L<PI> resume\*O"
setzt die Programinvocation fort
.LI "\*L<PI> reset\*O"
hält die Programinvocation an und versetzt sie in den Ausgangszustand
.LI "\*L<PI> upload\*O"
liefert Informationen über die Programinvocation
.LI "\*L<PI> status\*O"
liefert Informationen über den Status der Programinvocation
.LE
.SH RELATED INFORMATION
\*Ltclsh\*O, \*Lwish\*0
.EQ
delim off
.EN
