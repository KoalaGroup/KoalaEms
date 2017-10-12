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
.TH ved_reset n
.SH NAME
.PP
\*L<VED> reset\*O \- EMS-Kommando reset
.SH SYNOPSIS
.PP
.sS
\*L<VED> reset\*O
.sE
.SH PARAMETERS
.PP
.VL 4m
.LI "\*Okeine"
.LE
.SH DESCRIPTION
.PP
\*L<VED> reset\*O sendet das EMS-Kommando 'reset' an ein VED.
.SH RETURN VALUE
Bei Erfolg wird ein leerer String zurückgegeben.
.nL
Im Fehlerfall wird eine Fehlerbeschreibung zurückgegeben.
.EQ
delim off
.EN
