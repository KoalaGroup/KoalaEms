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
.TH ved_close n
.SH NAME
.PP
\*L<VED> close\*O \- beendet die Verbindung zu einem VED
.SH SYNOPSIS
.PP
.sS
\*L<VED> close\*O
.sE
.SH PARAMETERS
.PP
.VL 4m
.LI "\*Okeine"
.LE
.SH DESCRIPTION
.PP
\*L<VED> close\*O beendet die Verbindung zu einem VED und löscht das zugeordnete
VED-Kommando. Wenn über \*L<VED> closecommand\*O ein Kommando angegeben wurde,
wird dieses vorher ausgeführt. Der gleiche Effekt wird durch \*Lrename\*O <VED>
{} erreicht.
.SH RETURN VALUE
Bei Erfolg wird ein leerer String zurückgegeben.
.nL
Im Fehlerfall wird eine Fehlerbeschreibung zurückgegeben.
.SH RELATED INFORMATION
\*Lems_open\*O, \*Lved_closecommand\*O, \*Lrename\*O
.EQ
delim off
.EN
