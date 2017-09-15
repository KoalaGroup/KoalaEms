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
.TH ved_closecommand n
.SH NAME
.PP
\*L<VED> closecommand\*O
.SH SYNOPSIS
.PP
.sS
\*L<VED> closecommand command\*O
.sE
.SH PARAMETERS
.PP
.VL 4m
.LI "\*Oprocedure\*Vcommand\*O"
.LE
.SH DESCRIPTION
.PP
\*L<VED> closecommand\*O installiert eine TCL-Prozedur, die vor dem Schließen
einer Verbindung zu einem VED ausgeführt wird. \*Lrename\*O <VED> {} führt auch
zum Schließen des VED und damit zum Ausführen des Kommandos. Vor der Ausführung
wird %n durch den VED-Namen und %c durch den Namen des VED-Kommandos ersetzt.
.SH RETURN VALUE
Bei Erfolg wird das vorher gültige Kommando zurückgegeben.
.nL
Im Fehlerfall wird eine Fehlerbeschreibung zurückgegeben.
.SH EXAMPLES
<VED> closecommand {puts "close VED %n"}
.SH RELATED INFORMATION
\*Lved_close\*O, \*Lrename\*O
.EQ
delim off
.EN
