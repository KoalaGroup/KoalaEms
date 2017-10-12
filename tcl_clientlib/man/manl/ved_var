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
.TH ved_var n
.SH NAME
.PP
\*L<VED> var\*O \- Variablenkommandos
.SH SYNOPSIS
.PP
.sS
\*L<VED> var create\*O idx [size]
.nL
\*L<VED> var delete\*O idx
.nL
\*L<VED> var read\*O idx [first [num]]
.nL
\*L<VED> var write\*O idx list
.nL
\*L<VED> var write\*O idx value [value ...]
.nL
\*L<VED> var size\*O idx
.sE
.SH PARAMETERS
.PP
.VL 4m
.LI "\*Oint \*Vidx\*O"
Variablenindex
.LI "\*Oint \*Vsize\*O"
Größe, Default ist 1
.LI "\*Oint \*Vfirst\*O"
Index des ersten zu lesenden Wertes, Default ist 0
.LI "\*Oint \*Vnum\*O"
Anzahl der zu lesenden Werte
.LI "\*Vlist\*O"
Liste der zu schreibenden Werte
.LI "\*Oint \*Vvalue\*O"
zu schreibende Werte
.LE
.SH DESCRIPTION
.PP
\*L<VED> var\*O ist eine Gruppe von Kommandos, mit deren Hilfe EMS-Variablen
manipuliert werden können.
.SH RETURN VALUE
\*Lcreate\*O, \*Ldelete\*O und \*Lwrite\*O liefern bei Erfolg einen leeren
String zurück,
\*Lread\*O liefert eine Liste der gelesenen Werte, \*Lsize\*O liefert die Größe
der Variablen.
.nL
Im Fehlerfall wird eine Fehlerbeschreibung zurckgegeben.
.SH EXAMPLES
<VED> var create 17
.nL
<VED> var create 17 3
.nL
<VED> var delete 17
.nL
<VED> var read 17
.nL
<VED> var write 17 {1 2 3}
.nL
<VED> var write 17 1 2 3
.nL
<VED> var size 17
.EQ
delim off
.EN
