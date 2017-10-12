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
.TH ems_open n
.SH NAME
.PP
\*Lems_open\*O \- öffnet eine Verbindung zu einem VED
.SH SYNOPSIS
.PP
.sS
\*Lems_open\*O name
.PP
\*Lems_open\*O name type
.PP
\*Lems_open\*O name type priority
.sE
.SH PARAMETERS
.PP
.VL 4m
.LI "\*Ostring \*Vname\*O"
VED-name
.LI "\*Oenum \*Vtype\*O"
{any, eventmanager, controller, camac, fastbus, chimaere, camac_chimaere,
fastbus_chimaere}
.nL
default: any
.LI "\*Oint \*Vpriority\*O"
Festlegung der Reihenfolge beim Starten des Readouts.
.nL
default: je nach typ:
.TS
tab(@);
l n.
    any@0
    eventmanager@4000
    andere@1000
.TE
.SH DESCRIPTION
.PP
\*Lems_open\*O öffnet die Verbindung zu einer VED.
Der Typ des VED wird benötigt, damit die EMS-TCL-Bibliothek die passenden
Kommandos zur Verfügung stellen kann.
Für die Typen gibt es folgende Hierarchie:
.nL
                                any 
.nL
                               /   \\
.nL
                              /     \\
.nL
                             /       \\
.nL
                     eventmanager controller
.nL
                           \\       /   |  \\
.nL
                            \\     /    |   \\
.nL
                             \\   /     |    \\
.nL
                              \\ /      |     \\
.nL
                          chimaere   camac  fastbus
.nL
                              |   \\   /       |
.nL
                              |    \\ /        |
.nL
                              |     X         |
.nL
                              |    / \\        |
.nL
                              |   /   \\       |
.nL
                      camac_chimaere  fastbus_chimaere
.PP

.nL
Die priority ist nur nötig, wenn der Readout für alle VEDs des Systems mit
einem Kommando gestartet werden soll. Das ist noch nicht implementiert.
\*Lems_open\*O erzeugt ein neues TCL-Kommando, das alle VED-Funktionen zur
Verfügung stellt. Der Name des Kommandos ist der VED-Name oder der VED-Name
und ein Index. Er kann mittels \*Lrename\*O geändert werden. Wenn das
VED-Kommando mit rename gelöscht wird, wird auch die Verbindung zum VED beendet.

.SH RETURN VALUE
Bei Erfolg wird der Name des erzeugten TCL-Kommandos zurckgegeben.
.nL
Im Fehlerfall wird eine Fehlerbeschreibung zurückgegeben.
.SH EXAMPLES
ems_open commu
.nL
ems_open commu controller
.nL
ems_open ikpf01 fastbus 200
.SH RELATED INFORMATION
\*Lems_connect\*O, \*Lems_veds\*O, \*L<VED> close\*O, \*Lrename\*O
.EQ
delim off
.EN
