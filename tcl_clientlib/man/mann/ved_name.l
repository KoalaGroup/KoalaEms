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
.TH ved_name n
.SH NAME
.PP
\*L<VED> name\*O \- liefert den Namen eines VED
.SH SYNOPSIS
.PP
.sS
\*L<VED> name\*O
.sE
.SH PARAMETERS
.PP
.VL 4m
.LI "\*Okeine"
.LE
.SH DESCRIPTION
.PP
\*L<VED> name\*O liefert den namen eines geöffneten VEDs.
.SH RETURN VALUE
Bei Erfolg wird der VED-Name zurückgegeben.
.nL
Im Fehlerfall wird eine Fehlerbeschreibung zurückgegeben.
.SH RELATED INFORMATION
\*L<VED> open\*O
.EQ
delim off
.EN
