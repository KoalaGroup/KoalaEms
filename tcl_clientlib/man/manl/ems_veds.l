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
.TH ems_veds n
.SH NAME
.PP
\*Lems_veds\*O \- listet bekannte VEDs
.SH SYNOPSIS
.PP
.sS
\*Lems_veds\*O
.sE
.SH PARAMETERS
.PP
.VL 4m
.LI "\*Okeine"
.LE
.SH DESCRIPTION
.PP
\*Lems_veds\*O erzeugt eine Liste mit allen VEDs, die der Kommunikationsprozeß
kennt.
.SH RETURN VALUE
Bei Erfolg wird ein leerer String zurückgegeben.
.nL
Im Fehlerfall wird eine Fehlerbeschreibung zurückgegeben.
.SH RELATED INFORMATION
\*Lems_connect\*O, \*Lems_open\*O
.EQ
delim off
.EN
