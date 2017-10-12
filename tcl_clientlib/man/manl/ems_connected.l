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
.TH ems_connected n
.SH NAME
.PP
\*Lems_connected\*O \- testet Existenz einer Verbindung zum
Kommunikationsprozeß
.SH SYNOPSIS
.PP
.sS
\*Lems_connected\*O
.sE
.SH PARAMETERS
.PP
.VL 4m
.LI "\*Okeine"
.LE
.SH DESCRIPTION
.PP
\*Lems_connected\*O stellt fest, ob eine Verbindung zu einem
Kommunikationsprozeß aufgenommen wurde.
.SH RETURN VALUE
Wenn eine Verbindung existiert, wird "1" zurückgegeben.
.nL
Wenn keine Verbindung existiert, wird "0" zurückgegeben.
.SH RELATED INFORMATION
\*Lems_connect\*O, \*Lems_disconnect\*O
.EQ
delim off
.EN
