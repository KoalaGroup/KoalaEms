
/* Errorcode IEC-Bus */

/* Aktionen */
#define CON_IEC        0x0100
#define REC_IEC        0x0200
#define TRA_IEC        0x0300
#define COM_IEC        0x0400

/* Grund */
#define NO_TRI         0x0001
#define UnknownCommand 0x0002
#define ERR_SetStt     0x0003
#define ERR_Write      0x0004
#define ERR_Read       0x0005

#define     PATHNAME        "/iecc"
#define     BUFFERSIZE      2048
#define     SIGNAL          7
#define     COMNUM          9       /* Number known of commands */

struct tabent  {
    char    *comstr;
    int     comcode;
};




