#ifdef NUR512K
%define EVENT_BUFBEG 0x880000
%define EVENT_BUFEND 0x8c0000 /* 256k */
#else
#ifdef LOKAL2M
%define EVENT_BUFBEG 0x800000
%define EVENT_BUFEND 0xa00000 /* 2M */
#endif
#else
#ifdef VAROUT
/* konfigurierbar */
#else
%define EVENT_BUFBEG 0x800000
%define EVENT_BUFEND 0x880000 /* 512k */
#endif
#endif
