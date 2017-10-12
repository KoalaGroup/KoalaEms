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
.TH ved_is n
.SH NAME
.PP
\*L<VED> is\*O \- Instrumentierungssystemkommandos
.SH SYNOPSIS
.PP
.sS
\*L<VED> is list\*O
.nL
\*L<VED> is exists\*O idx
.nL
\*L<VED> is create\*O idx id
.nL
\*L<VED> is open\*O idx [id]
.nL
\*L<VED> is delete\*O idx
.nL
\*L<VED> is id\*O idx
.nL
.sE
.SH PARAMETERS
.PP
.VL 4m
.LI "\*Oint \*Vidx\*O"
Index des Instrumentierungssystems
.LI "\*Oint \*Vid\*O"
Identifikator des Instrumentierungssystems
.LE
.SH DESCRIPTION
.PP
\*L\*O
.SH RETURN VALUE
Bei Erfolg wird ein leerer String zurückgegeben.
.nL
Im Fehlerfall wird eine Fehlerbeschreibung zurückgegeben.
.SH EXAMPLES
.SH RELATED INFORMATION
\*L\*O
.EQ
delim off
.EN
