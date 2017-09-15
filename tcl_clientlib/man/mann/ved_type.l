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
.TH ved_type
.SH NAME
.PP
\*L<VED> type\*O \- liefert den Typ eines VED
.SH SYNOPSIS
.PP
.sS
\*L<VED> type\*O
.sE
.SH PARAMETERS
.PP
.VL 4m
.LI "\*Okeine"
.LE
.SH DESCRIPTION
.PP
\*L<VED> type\*O liefert den Type eines VED. Mögliche Werte sind: any,
eventmanager, controller, chimaere, camac, fastbus, camac_chimaere,
fastbus_chimaere. Der Typ ist keine Eigenschaft des VED, er wird beim Öffnen
festgelegt.
.SH RETURN VALUE
Bei Erfolg wird der Typ zurückgegeben.
.nL
Im Fehlerfall wird eine Fehlerbeschreibung zurückgegeben.
.SH RELATED INFORMATION
\*Lems_open\*O
.EQ
delim off
.EN
