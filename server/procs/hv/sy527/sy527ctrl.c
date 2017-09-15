/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    SY527CTRL.C                                                  		   */
/*                                                                         */
/*    Demonstration about the use of Caenet Routines in communication      */
/*    between A303/A and A1303 modules and SY527 Universal Multichannel    */
/*    Power Supply System equipped with module A128A High Speed Caenet     */
/*    Communication Controller                                             */
/*                                                                         */
/*    Source code written in Ansi C                                        */
/*    Must be linked to console.c and to the HSCAENETlib library which is  */
/*    available in dll format (for Win32, the .lib requires Visual Studio) */
/*    or as static or dynamic C Linux library                              */
/*                                                                         */
/*    Created: March 2002                                                  */
/*                                                                         */
/***************************************************************************/
#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <time.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <errno.h>
#include  "console.h"
#include  "HSCAENETLib.h"

#ifndef   uchar
#define   uchar                        unsigned char
#endif
#ifndef   ushort
#define   ushort                      unsigned short
#endif
#ifndef   ulong
#define   ulong                        unsigned long
#endif

#define   MAX_CH_TYPES                            20

#define   IDENT                                    0
#define   READ_STATUS                              1
#define   READ_SETTINGS                            2
#define   READ_BOARD_INFO                          3
#define   READ_CR_CONF                             4
#define   READ_GEN_STATUS                          5
#define   READ_HVMAX                               6

#define   SET_STATUS_ALARM                      0x1a
#define   FORMAT_EEPROM_1                       0x30
#define   FORMAT_EEPROM_2                       0x31
#define   CLEAR_ALARM                           0x32
#define   LOCK_KEYBOARD                         0x33
#define   UNLOCK_KEYBOARD                       0x34
#define   KILL_CHANNELS_1                       0x35
#define   KILL_CHANNELS_2                       0x36

#define   LIST_CH_GRP                           0x40
#define   MON_CH_GRP                            0x41
#define   V0SET_CH_GRP                          0x43
#define   ADD_CH_GRP                            0x50
#define   REM_CH_GRP                            0x51

#define   GRP                                      0
#define   NO_GRP                                   1
#define   ADD                                      0
#define   REM                                      1

#define   V0SET                                    0
#define   V1SET                                    1
#define   I0SET                                    2
#define   I1SET                                    3
#define   VMAX                                     4
#define   RUP                                      5
#define   RDWN                                     6
#define   TRIP                                     7

/*
  An useful macro
*/
#define caenet_comm(s, w, d) HSCAENETComm(HSCAENETDev, code, cratenum, (s), (w), (d))

/*
  The following structure contains all the useful information about
  the settings of a channel
*/
struct hvch
{
 char    chname[12];
 long    v0set, v1set;
 ushort  i0set, i1set;
 short   vmax;
 short   rup, rdwn;
 short   trip, dummy;
 ushort  flag;
};

/*
  The following structure contains all the useful information about
  the status of a channel
*/
struct hvrd
{
 long   vread;
 short  hvmax;
 short  iread;
 ushort status;
};

/*
  The following structure contains all the useful information about the
  characteristics of every board
*/
struct board
{
 char    name[5];
 char    curr_mis;
 ushort  sernum;
 char    vermaior;
 char    verminor;
 char    reserved[20];
 uchar   numch;
 ulong   omog;
 long    vmax;
 short   imax, rmin, rmax;
 short   resv, resi, decv, deci;
 uchar   dummy1;
};

/*
  The following structure contains all the useful information about the
  types of a channel
*/
struct chtype
{
 char   iumis;
 char   dummy;
 char   reserved[4];
 long   vmax;
 short  imax, rmin, rmax;
 short  resv, resi, decv, deci;
 char   dummy1[4];
};

/*
  The following structure contains all the useful information about
  the alarm status of SY527
*/
struct st_al
{
 unsigned  level   :1;
 unsigned  pulsed  :1;
 unsigned  ovc     :1;
 unsigned  ovv     :1;
 unsigned  unv     :1;
 unsigned  unused  :11;
};

static void logga(char *fmt, ...);

/*
  Globals
*/
static struct board       boards[10];
static int    			  code, cratenum;
static int                HSCAENETDev;
static float              pow_10[]={ 1.0, 10.0, 100.0, 1000.0};
static struct st_al       status_alarm;
/*
  The following array contains the type of every channel for a given not
  homogeneous board
*/
static char               ch_to_type[32];
static struct chtype      chtypes[MAX_CH_TYPES];
static char               logfile[80];


/***------------------------------------------------------------------------

  DecodeCAENETResp

    --------------------------------------------------------------------***/
static char *DecodeCAENETResp(int resp)
{
  static char buf[80];

  if( resp == E_OS )
	strcpy(buf, strerror(errno));
  else
    switch( resp )
      {
        case E_LESS_DATA:
          strcpy(buf, "CaeNet Error: Less data received" );
          break;
        case E_TIMEOUT:
          strcpy(buf, "CaeNet Error: Timeout expired" );
          break;
        case E_NO_DEVICE:
          strcpy(buf, "CaeNet Error: High Speed CAENET device not found" );
          break;
        case E_A303_BUSY:
          sprintf(buf, "Another instance of a303lib.dll already loaded" );
          break;
        default:
          sprintf(buf, "CaeNet Error: %d", resp );
          break;
      }

  return buf;
}

/***------------------------------------------------------------------------

  Makemenu

    --------------------------------------------------------------------***/
static int makemenu(void)
{
clrscr();
highvideo();
con_puts("                    - MAIN MENU -      \n\n\n ");
normvideo();
con_puts(" [A] - Read Module Identifier ");
con_puts(" [B] - Crate Map ");
con_puts(" [C] - Channels Monitor        ");
con_puts(" [D] - Speed test              ");
con_puts(" [E] - Parameter Setting       ");
con_puts(" [F] - Clear Alarms            ");
con_puts(" [G] - Set Alarm Type          ");
con_puts(" [H] - Lock Keyboard           ");
con_puts(" [I] - Unlock Keyboard         ");
con_puts(" [J] - Kill ALL Channels       ");
con_puts(" [K] - Front Panel Status      ");
con_puts(" [L] - Groups Operations       ");
con_puts(" [M] - Format EEPROM           ");
con_puts(" [N] - Channels Logging        ");
con_puts("\n\n [Q] - Quit ");

return toupper(con_getch());
}

/***------------------------------------------------------------------------

  Makegrpmenu

    --------------------------------------------------------------------***/
static int makegrpmenu(void)
{
clrscr();
highvideo();
con_puts("                    - GROUPS MENU -      \n\n\n ");
normvideo();
con_puts(" [A] - Add Channels to a Group ");
con_puts(" [B] - Remove Channels from a Group ");
con_puts(" [C] - Add Channels to all Groups ");
con_puts(" [D] - Remove Channels from all Groups ");
con_puts(" [E] - List Channels of a Group ");
con_puts(" [F] - Parameter Setting       ");
con_puts(" [G] - Monitor Channels of a Group       ");
con_puts("\n\n [Q] - Quit ");

return toupper(con_getch());
}

/***------------------------------------------------------------------------

  Read_Ident

    --------------------------------------------------------------------***/
static void read_ident(void)
{
int i,response;
char sy527ident[12];
char tempbuff[80];
code=IDENT;                                 /* To see if sy527 is present */
if((response=caenet_comm(NULL,0,tempbuff)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
   return;
  }
for(i=0;i<11;i++)
    sy527ident[i]=tempbuff[2*i];
sy527ident[i]='\0';
con_printf(" The module has answered : %s\n",sy527ident);
con_puts(" Press any key to continue ");
con_getch();
}

/***------------------------------------------------------------------------

  Swap

    --------------------------------------------------------------------***/
static void swap(char *a, char *b)
{
char temp;

temp = *a;
*a = *b;
*b = temp;
}

/***------------------------------------------------------------------------

  Swap_Byte

    --------------------------------------------------------------------***/
static void swap_byte(char *buff,int size)
{
int  i;

for( i=0 ; i<size ; i += 2 )
    swap(buff+i,buff+i+1);
}

/***------------------------------------------------------------------------

  Swap_Long

    --------------------------------------------------------------------***/
static void swap_long(void *buff)
{
swap((char *)buff,(char *)buff+3);
swap((char *)buff+1,(char *)buff+2);
}

/***------------------------------------------------------------------------

  Size_of_Board
  We need the true size in bytes as SY527 sends it via CAENET; it is different
  from sizeof(struct board)

    --------------------------------------------------------------------***/
static int size_of_board(void)
{
 struct board b;

 return   sizeof(b.curr_mis) + sizeof(b.deci)     + sizeof(b.decv)     + sizeof(b.imax) 
        + sizeof(b.name)     + sizeof(b.numch)    + sizeof(b.omog)     + sizeof(b.reserved)
        + sizeof(b.resi)     + sizeof(b.resv)     + sizeof(b.rmax)     + sizeof(b.rmin) 
        + sizeof(b.sernum)   + sizeof(b.vermaior) + sizeof(b.verminor) + sizeof(b.vmax)
        + sizeof(b.dummy1);
}

/***------------------------------------------------------------------------

  Size_of_Chtype
  We need the true size in bytes as SY527 sends it via CAENET; it is different
  from sizeof(struct chtype)

    --------------------------------------------------------------------***/
static int size_of_chtype(void)
{
 struct chtype c;

 return   sizeof(c.deci) + sizeof(c.decv)  + sizeof(c.dummy)    + sizeof(c.dummy1)
        + sizeof(c.imax) + sizeof(c.iumis) + sizeof(c.reserved) + sizeof(c.resi)
        + sizeof(c.resv) + sizeof(c.rmax)  + sizeof(c.rmin)     + sizeof(c.vmax);
}

/***------------------------------------------------------------------------

  Size_of_Chset
  We need the true size in bytes as SY527 sends it via CAENET; it is different
  from sizeof(struct hvch)

    --------------------------------------------------------------------***/
static int size_of_chset(void)
{
 struct hvch ch;

 return   sizeof(ch.chname) + sizeof(ch.dummy) + sizeof(ch.flag) + sizeof(ch.i0set)
        + sizeof(ch.i1set)  + sizeof(ch.rdwn)  + sizeof(ch.rup)  + sizeof(ch.trip) 
        + sizeof(ch.v0set)  + sizeof(ch.v1set)  + sizeof(ch.vmax);
}

/***------------------------------------------------------------------------

  Size_of_Chrd
  We need the true size in bytes as SY527 sends it via CAENET; it is different
  from sizeof(struct hvrd)

    --------------------------------------------------------------------***/
static int size_of_chrd(void)
{
 struct hvrd ch;

 return   sizeof(ch.hvmax) + sizeof(ch.iread) + sizeof(ch.status) + sizeof(ch.vread);
}

/***------------------------------------------------------------------------

  Build_Bd_Info

    --------------------------------------------------------------------***/
static int build_bd_info(struct board *bd, char *cnetbuff)
{
int i = sizeof(bd->name);

swap_byte(cnetbuff, size_of_board());

memcpy(bd, cnetbuff, i);

memcpy(&bd->curr_mis, cnetbuff+i, sizeof(bd->curr_mis));
i += sizeof(bd->curr_mis);

memcpy(&bd->sernum, cnetbuff+i, sizeof(bd->sernum));
swap_byte((char *)&bd->sernum,sizeof(bd->sernum));
i += sizeof(bd->sernum);

memcpy(&bd->vermaior, cnetbuff+i, sizeof(bd->vermaior));
i += sizeof(bd->vermaior);

memcpy(&bd->verminor, cnetbuff+i, sizeof(bd->verminor));
i += sizeof(bd->verminor) + sizeof(bd->reserved);

memcpy(&bd->numch, cnetbuff+i, sizeof(bd->numch));
i += sizeof(bd->numch);

memcpy(&bd->omog, cnetbuff+i, sizeof(bd->omog));
swap_long(&bd->omog);
i += sizeof(bd->omog);

memcpy(&bd->vmax, cnetbuff+i, sizeof(bd->vmax));
swap_long(&bd->vmax);
i += sizeof(bd->vmax);

memcpy(&bd->imax, cnetbuff+i, sizeof(bd->imax));
swap_byte((char *)&bd->imax,sizeof(bd->imax));
i += sizeof(bd->imax);

memcpy(&bd->rmin, cnetbuff+i, sizeof(bd->rmin));
swap_byte((char *)&bd->rmin,sizeof(bd->rmin));
i += sizeof(bd->rmin);

memcpy(&bd->rmax, cnetbuff+i, sizeof(bd->rmax));
swap_byte((char *)&bd->rmax,sizeof(bd->rmax));
i += sizeof(bd->rmax);

memcpy(&bd->resv, cnetbuff+i, sizeof(bd->resv));
swap_byte((char *)&bd->resv,sizeof(bd->resv));
i += sizeof(bd->resv);

memcpy(&bd->resi, cnetbuff+i, sizeof(bd->resi));
swap_byte((char *)&bd->resi,sizeof(bd->resi));
i += sizeof(bd->resi);

memcpy(&bd->decv, cnetbuff+i, sizeof(bd->decv));
swap_byte((char *)&bd->decv,sizeof(bd->decv));
i += sizeof(bd->decv);

memcpy(&bd->deci, cnetbuff+i, sizeof(bd->deci));
swap_byte((char *)&bd->deci,sizeof(bd->deci));
i += sizeof(bd->deci) + sizeof(bd->dummy1);

if( bd->omog & (1L<<17) )      /* The board is not homogeneous */
  {
   int    j, n = bd->numch;
   short  typ;

   memcpy(&typ,cnetbuff+i,sizeof(typ));
   i += sizeof(typ);

   memcpy(ch_to_type, cnetbuff+i, (n&1) ? n+1 : n);
   i += ( (n&1) ? n+1 : n );
   swap_byte(ch_to_type,sizeof(ch_to_type));

   for( j = 0 ; j < typ ; j++ )
      {
       swap_byte(cnetbuff+i, size_of_chtype());
     
       memcpy(&chtypes[j].iumis, cnetbuff+i, sizeof(chtypes[j].iumis));
       i += sizeof(chtypes[j].iumis) + sizeof(chtypes[j].dummy) + sizeof(chtypes[j].reserved);
 
       memcpy(&chtypes[j].vmax, cnetbuff+i, sizeof(chtypes[j].vmax));
       swap_long(&chtypes[j].vmax);
       i += sizeof(chtypes[j].vmax);

       memcpy(&chtypes[j].imax, cnetbuff+i, sizeof(chtypes[j].imax));
       swap_byte((char *)&chtypes[j].imax, sizeof(chtypes[j].imax));
       i += sizeof(chtypes[j].imax);

       memcpy(&chtypes[j].rmin, cnetbuff+i, sizeof(chtypes[j].rmin));
       swap_byte((char *)&chtypes[j].rmin, sizeof(chtypes[j].rmin));
       i += sizeof(chtypes[j].rmin);

       memcpy(&chtypes[j].rmax, cnetbuff+i, sizeof(chtypes[j].rmax));
       swap_byte((char *)&chtypes[j].rmax, sizeof(chtypes[j].rmax));
       i += sizeof(chtypes[j].rmax);

       memcpy(&chtypes[j].resv, cnetbuff+i, sizeof(chtypes[j].resv));
       swap_byte((char *)&chtypes[j].resv, sizeof(chtypes[j].resv));
       i += sizeof(chtypes[j].resv);

       memcpy(&chtypes[j].resi, cnetbuff+i, sizeof(chtypes[j].resi));
       swap_byte((char *)&chtypes[j].resi, sizeof(chtypes[j].resi));
       i += sizeof(chtypes[j].resi);

       memcpy(&chtypes[j].decv, cnetbuff+i, sizeof(chtypes[j].decv));
       swap_byte((char *)&chtypes[j].decv, sizeof(chtypes[j].decv));
       i += sizeof(chtypes[j].decv);

       memcpy(&chtypes[j].deci, cnetbuff+i, sizeof(chtypes[j].deci));
       swap_byte((char *)&chtypes[j].deci, sizeof(chtypes[j].deci));
       i += sizeof(chtypes[j].deci) + sizeof(chtypes[j].dummy1);
      }
  }

return TUTTOK;
}

/***------------------------------------------------------------------------

  Build_Chset_Info

    --------------------------------------------------------------------***/
static void build_chset_info(struct hvch *ch, char *cnetbuff)
{
int i = sizeof(ch->chname);

swap_byte(cnetbuff, size_of_chset());

memcpy(&ch->chname, cnetbuff, i);

memcpy(&ch->v0set,cnetbuff+i, sizeof(ch->v0set));
swap_long(&ch->v0set);
i += sizeof(ch->v0set);

memcpy(&ch->v1set,cnetbuff+i, sizeof(ch->v1set));
swap_long(&ch->v1set);
i += sizeof(ch->v1set);

memcpy(&ch->i0set,cnetbuff+i, sizeof(ch->i0set));
swap_byte((char *)&ch->i0set, sizeof(ch->i0set));
i += sizeof(ch->i0set);

memcpy(&ch->i1set,cnetbuff+i, sizeof(ch->i1set));
swap_byte((char *)&ch->i1set, sizeof(ch->i1set));
i += sizeof(ch->i1set);

memcpy(&ch->vmax,cnetbuff+i, sizeof(ch->vmax));
swap_byte((char *)&ch->vmax, sizeof(ch->vmax));
i += sizeof(ch->vmax);

memcpy(&ch->rup,cnetbuff+i, sizeof(ch->rup));
swap_byte((char *)&ch->rup, sizeof(ch->rup));
i += sizeof(ch->rup);

memcpy(&ch->rdwn,cnetbuff+i, sizeof(ch->rdwn));
swap_byte((char *)&ch->rdwn, sizeof(ch->rdwn));
i += sizeof(ch->rdwn);

memcpy(&ch->trip,cnetbuff+i, sizeof(ch->trip));
swap_byte((char *)&ch->trip, sizeof(ch->trip));
i += sizeof(ch->trip);

memcpy(&ch->dummy,cnetbuff+i, sizeof(ch->dummy));
swap_byte((char *)&ch->dummy, sizeof(ch->dummy));
i += sizeof(ch->dummy);

memcpy(&ch->flag,cnetbuff+i, sizeof(ch->flag));
swap_byte((char *)&ch->flag, sizeof(ch->flag));
i += sizeof(ch->flag);
}

/***------------------------------------------------------------------------

  Build_Chrd_Info

    --------------------------------------------------------------------***/
static void build_chrd_info(struct hvrd *ch, char *cnetbuff)
{
int i = sizeof(ch->vread);

swap_byte(cnetbuff, size_of_chrd());

memcpy(&ch->vread, cnetbuff, sizeof(ch->vread));
swap_long(&ch->vread);

memcpy(&ch->hvmax, cnetbuff+i, sizeof(ch->hvmax));
swap_byte((char *)&ch->hvmax, sizeof(ch->hvmax));
i += sizeof(ch->hvmax);

memcpy(&ch->iread, cnetbuff+i, sizeof(ch->iread));
swap_byte((char *)&ch->iread, sizeof(ch->iread));
i += sizeof(ch->iread);

memcpy(&ch->status, cnetbuff+i, sizeof(ch->status));
swap_byte((char *)&ch->status, sizeof(ch->status));
i += sizeof(ch->status);
}

/***------------------------------------------------------------------------

  Get_Cr_Info

    --------------------------------------------------------------------***/
static int get_cr_info(ushort *cr_cnf)
{
int   i,response;
short bd;
char  cnetbuff[MAX_LENGTH_FIFO];

code=READ_CR_CONF;
if((response=caenet_comm(NULL,0,cr_cnf)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
   return response;
  }

code=READ_BOARD_INFO;
for( bd=0, i=1 ; bd<10 ; bd++, i = i << 1 )
    if(*cr_cnf & i)
      {
       if((response=caenet_comm(&bd,sizeof(bd),cnetbuff)) != TUTTOK)
         {
          con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
          con_puts(" Press any key to continue ");
          con_getch();
          return response;
         }
       else
          build_bd_info((struct board *)&boards[bd],cnetbuff);
      }

return TUTTOK;
}

/***------------------------------------------------------------------------

  Crate_Map

    --------------------------------------------------------------------***/
static void crate_map(void)
{
static char *curr_umis[] =
{
 " A",
 "mA",
 "uA",
 "nA"
};
int         i,bd;
float       im;
ushort      cr_conf;

if(get_cr_info(&cr_conf) != TUTTOK) /* Get information about the Crate Configuration */
   return;

clrscr();
con_puts("\n\n                         ---  Crate Map ---       \n\n\n\n\n ");

for( bd=0, i=1 ; bd<10 ; bd++, i = i << 1 )
   {
    con_printf(" Slot %d - ",bd);

    if(cr_conf & i)
      {
       char bdname[6];

       strncpy(bdname,boards[bd].name,5);
       bdname[5] = '\0';
       con_printf(" Mod. %-5s  %3d CH ",bdname,boards[bd].numch);
       con_printf("  %4ldV",boards[bd].vmax);
       im = (float)boards[bd].imax/pow_10[boards[bd].deci];
       con_printf("  %8.2f",im);
       con_printf("%s",curr_umis[(int)boards[bd].curr_mis]);
       con_printf(" --- Ser. %3d, Rel. %d.%02d\n",
               boards[bd].sernum,boards[bd].vermaior,boards[bd].verminor);
      }
    else
       con_printf(" Not Present \n");

   }
con_puts("\n\n\n   Press any key to continue ");
con_getch();
}

/***------------------------------------------------------------------------

  Ch_monitor

    --------------------------------------------------------------------***/
static void ch_monitor(void)
{
int                      temp,caratt='P',
                         response;
ushort                   bd,ch,ch_addr;
char                     cnetbuff[MAX_LENGTH_FIFO];
static int               page=0;
static struct hvch       ch_set[32];        /* Settings of 32 chs.   */
static struct hvrd       ch_read[32];       /* Status   of 32 chs.   */

do
  {
   clrscr();
   con_printf(" Input Board Number [0 ... 9]: ");
   con_scanf("%d",&temp);
  }
while(temp < 0 || temp > 9);

bd = temp;
code=READ_BOARD_INFO;
if((response=caenet_comm(&bd,sizeof(bd),cnetbuff)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
   return;
  }
else
   build_bd_info(&boards[bd], cnetbuff);

clrscr();
con_printf(" Input Board Number [0 ... 9]: %d\n",bd);
highvideo();
if(!page)
   con_puts
("\n Channel     Vmon     Imon    V0set    I0set    V1set    I1set   Flag    Ch# ");
else
   con_puts
("\n Channel     Vmax     Rup    Rdwn     Trip     Status    Ch# ");
normvideo();

gotoxy(1,23);
con_puts(" Press 'P' to change page, any other key to exit ");

while(caratt == 'P') /* Loops until someone presses a key different from P  */
     {

/* First update from Caenet the information about the channels */
      for( ch=0 ; ch < 16 && ch < boards[bd].numch ; ch++ )
         {
          ch_addr = (bd<<8) | ch;

          code = READ_STATUS;
          response = caenet_comm(&ch_addr,sizeof(ch_addr), cnetbuff);
          if( response != TUTTOK )
            {
             con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
             con_puts(" Press any key to continue ");
             con_getch();
             return;
            }
          build_chrd_info(&ch_read[ch], cnetbuff);

          code = READ_SETTINGS;
          response = caenet_comm(&ch_addr,sizeof(ch_addr), cnetbuff);
          if( response != TUTTOK )
            {
             con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
             con_puts(" Press any key to continue ");
             con_getch();
             return;
            }
          build_chset_info(&ch_set[ch], cnetbuff);
         }   /* end for( ch ) */

      if(!page)                                    /* Page 0 of display */
         for( ch=0 ; ch < 16 && ch < boards[bd].numch ; ch++ )
            {
             float scalev, scalei;

             if( boards[bd].omog & (1L<<17) )
               {
                scalev = pow_10[ chtypes[ (int)ch_to_type[ch] ].decv ];
                scalei = pow_10[ chtypes[ (int)ch_to_type[ch] ].deci ];
               }
             else
               {
                scalev = pow_10[ boards[bd].decv ];
                scalei = pow_10[ boards[bd].deci ];
               }

             gotoxy(1,ch+5);
             con_printf(" %9s",ch_set[ch].chname);
             gotoxy(12,ch+5);
             con_printf
("%07.2f  %07.2f  %07.2f  %07.2f  %07.2f  %07.2f  %4x     %2d ",
ch_read[ch].vread/scalev,ch_read[ch].iread/scalei,ch_set[ch].v0set/scalev,
ch_set[ch].i0set/scalei,ch_set[ch].v1set/scalev,ch_set[ch].i1set/scalei,
ch_set[ch].flag,ch);
            }
      else                                         /* Page 1 of display */
         for( ch=0 ; ch < 16 && ch < boards[bd].numch ; ch++ )
            {
             gotoxy(1,ch+5);
             con_printf(" %9s",ch_set[ch].chname);
             gotoxy(14,ch+5);
             con_printf
("%4d     %3d     %3d    %05.1f       %4x     %2d ",
ch_set[ch].vmax,ch_set[ch].rup,ch_set[ch].rdwn,ch_set[ch].trip/10.0,
ch_read[ch].status,ch);
            }

/* Test the keyboard */
      if(con_kbhit())                             /* A key has been pressed */
         if((caratt=toupper(con_getch())) == 'P') /* They want to change page */
           {
            page = !page;
            clrscr();
            con_printf(" Input Board Number [0 ... 9]: %d\n",bd);
            if(page == 0)
               con_puts
("\n Channel     Vmon    Imon    V0set    I0set    V1set    I1set    Flag    Ch# ");
            else
               con_puts
("\n Channel     Vmax     Rup    Rdwn     Trip     Status    Ch# ");
            gotoxy(1,23);
            con_puts(" Press 'P' to change page, any other key to exit ");
           }

     }  /* End while */
}

/***------------------------------------------------------------------------

  Ch_logging
  Added June 2001
  The setup is read from sy527setup.dat
  The data are saved into sy527xy.dat (where x is the channel in the board 
  and y is the board)
  The errors are logged into sy527.log

  Modified July 2001 in the following:

  - Data are saved into sy527_cr_bd_ch.dat
  - The log file is now opened in "a+" mode instead of "w"

    --------------------------------------------------------------------***/
static void ch_logging(void)
{
int                      b,ch,cr,iter,
                         savecratenum, response;
FILE                     *f;
time_t                   t;
ushort                   ch_addr;
char                     *tok,line[100],cnetbuff[MAX_LENGTH_FIFO];
static struct hvch       ch_set;
static struct hvrd       ch_read;
static int               bdch[100][10][32];

clrscr();

strcpy(logfile,"sy527.log");
if( ( f = fopen(logfile,"a+") ) == NULL )
  {
   con_printf("\n Problems opening %s\n",logfile);
   con_getch();
   return;
  }
else
   fclose(f);

if( ( f = fopen("sy527setup.dat","r") ) == NULL )
  {
    con_puts("Error opening sy527setup.dat");
    con_getch();
    return;
  }

for( cr = 0 ; cr < 100 ; cr++ )
    for( b = 0 ; b < 10 ; b++ )
        for( ch = 0 ; ch < 32 ; ch++ )
            bdch[cr][b][ch] = 0;

savecratenum = cratenum;

/*  
    The format of sy527setup.dat is :

    cr1 b1 ca cb cc
    crn bn cd ce cf

    Where cr1 and crn are the SY527's CAENET Crate Numbers, b1 and bn are the 
    board id (0...9) and ca, cb, .. are the channel offset in the given board
*/
for( ; fgets(line,sizeof(line),f) ; )
  {
    tok = strtok(line," ");
    sscanf(tok,"%d",&cr);
    tok = strtok(NULL, " ");
    sscanf(tok,"%d", &b);
    while( 1 )
      {
        tok = strtok(NULL," ");
        if( tok == NULL )
           break;
        sscanf(tok,"%d",&ch);  
        bdch[cr][b][ch] = 1;
      }
  }

highvideo();
con_puts("\n\n                         ---  Channels Logging ---       \n\n\n\n\n ");
normvideo();

gotoxy(1,23);
con_puts(" Press any key to exit ");

iter = 0;
while( !con_kbhit() )
    {
      gotoxy(1,10);
      con_printf(" Iteration Nr. %d ...",iter);
      for( cr = 0 ; cr < 100 ; cr++ )
        for( b = 0 ; b < 10 ; b++ )
          for( ch = 0 ; ch < 32 ; ch++ )
            if( bdch[cr][b][ch] )
              {
                if( con_kbhit() )
                    break;

                ch_addr = (b<<8) | ch;
  
                code = READ_STATUS;
                cratenum = cr;
                response = caenet_comm(&ch_addr,sizeof(ch_addr), cnetbuff);
                if( response != TUTTOK )
                  {
                   logga(" Caenet_comm: %s\n Crate %0d, Board %0d, Channel %0d\n\n",
                           DecodeCAENETResp(response), cr, b, ch);
                   gotoxy(1,14);
                   con_printf(" Caenet_comm: %s  Crate %0d, Board %0d, Channel %0d\n", 
                     DecodeCAENETResp(response), cr, b, ch);
                   continue;
                  }
                build_chrd_info(&ch_read, cnetbuff);

                code = READ_SETTINGS;
                response = caenet_comm(&ch_addr,sizeof(ch_addr), cnetbuff);
                if( response != TUTTOK )
                  {
                   logga(" Caenet_comm: %s\n Crate %0d, Board %0d, Channel %0d\n\n",
                           DecodeCAENETResp(response), cr, b, ch);
                   gotoxy(1,14);
                   con_printf(" Caenet_comm: %s  Crate %0d, Board %0d, Channel %0d\n", 
                     DecodeCAENETResp(response), cr, b, ch);
                   continue;
                  }
                build_chset_info(&ch_set, cnetbuff);

                time(&t);
                sprintf(line,"sy527_%0d_%0d_%0d.dat",cr,b,ch);
                f = fopen(line,"a+");
                fprintf(f,"%lu\t%ld\t%d\t%ld\n",t,ch_read.vread,ch_read.iread,ch_set.v0set);
                fclose(f);
               }

       delay(2000);

       iter++;
     }  /* End while */

cratenum = savecratenum;

}

/***------------------------------------------------------------------------

  Par_set

    --------------------------------------------------------------------***/
static void par_set(int type)
{
float       input_value,
            scale;
ushort      channel = 0, bd, cnet_buff[2];
int         temp,ch,i,g,
            response,
            par=0;
char        choiced_param[10], cnetbuff[MAX_LENGTH_FIFO];
static char *param[] =
{
 "v0set", "v1set", "i0set", "i1set", "vmax", "rup", "rdwn", "trip", NULL
};

clrscr();
if( type == NO_GRP )
  {
   con_printf("\n\n Board: ");                      /* Choice the board   */
   con_scanf("%d",&temp);
   bd = temp;
   code=READ_BOARD_INFO;
   if((response=caenet_comm(&bd,sizeof(bd),cnetbuff)) != TUTTOK)
     {
      con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
      con_puts(" Press any key to continue ");
      con_getch();
      return;
     }
   else
      build_bd_info((struct board *)&boards[bd],cnetbuff);

   con_printf("\n\n Channel: ");                    /* Choice the channel */
   con_scanf("%d",&ch);
   channel = (bd<<8) | ch;
  }
else
  {
   con_printf("\n\n Group: ");                      /* Choice the group   */
   con_scanf("%d",&g);
  }

con_puts(" Allowed parameters (lowercase only) are:");
for( i = 0 ; param[i] != NULL ; i++ )
    con_puts(param[i]);

while(!par)
     {
      con_printf("\n Parameter to set: ");        /* Choice the parameter */
      con_scanf("%s",choiced_param);
      for( i = 0 ; param[i] != NULL ; i++ )
          if(!strcmp(param[i],choiced_param))
            {
             par=1;
             break;
            }
      if( param[i] == NULL )
         con_puts(" Sorry, this parameter is not allowed");
     }
con_printf(" New value :");                       /* Choice the value         */
con_scanf("%f",&input_value);

if( type == NO_GRP )
   cnet_buff[0] = channel;
else
   cnet_buff[0] = g;
switch(i)                                     /* Decode the par.          */
  {
   case V0SET:
       code = ( ( type == NO_GRP ) ? 0x10 : 0x52 );
       if( type == NO_GRP )
         {
          if( boards[bd].omog & (1L<<17) )
             scale = pow_10[ chtypes[ (int)ch_to_type[ch] ].decv ];
          else
             scale = pow_10[ boards[bd].decv ];
         }
       else
          scale = 10.0;
       input_value *= scale;
       cnet_buff[1]=(ushort)input_value;
       break;
   case V1SET:
       code = ( ( type == NO_GRP ) ? 0x11 : 0x53 );
       if( type == NO_GRP )
         {
          if( boards[bd].omog & (1L<<17) )
             scale = pow_10[ chtypes[ (int)ch_to_type[ch] ].decv ];
          else
             scale = pow_10[ boards[bd].decv ];
         }
       else
          scale = 10.0;
       input_value*=scale;
       cnet_buff[1]=(ushort)input_value;
       break;
   case I0SET:
       code = ( ( type == NO_GRP ) ? 0x12 : 0x54 );
       if( type == NO_GRP )
         {
          if( boards[bd].omog & (1L<<17) )
             scale = pow_10[ chtypes[ (int)ch_to_type[ch] ].deci ];
          else
             scale = pow_10[ boards[bd].deci ];
         }
       else
          scale = 10.0;                 /* Not very correct ... */
       input_value*=scale;
       cnet_buff[1]=(ushort)input_value;
       break;
   case I1SET:
       code = ( ( type == NO_GRP ) ? 0x13 : 0x55 );
       if( type == NO_GRP )
         {
          if( boards[bd].omog & (1L<<17) )
             scale = pow_10[ chtypes[ (int)ch_to_type[ch] ].deci ];
          else
             scale = pow_10[ boards[bd].deci ];
         }
        else
          scale = 10.0;                 /* Not very correct ... */
       input_value*=scale;
       cnet_buff[1]=(ushort)input_value;
       break;
   case VMAX:
       code = ( ( type == NO_GRP ) ? 0x14 : 0x56 );
       cnet_buff[1]=(ushort)input_value;
       break;
   case  RUP:
       code = ( ( type == NO_GRP ) ? 0x15 : 0x57 );
       cnet_buff[1]=(ushort)input_value;
       break;
   case RDWN:
       code = ( ( type == NO_GRP ) ? 0x16 : 0x58 );
       cnet_buff[1]=(ushort)input_value;
       break;
   case TRIP:
       code = ( ( type == NO_GRP ) ? 0x17 : 0x59 );
       input_value*=10;                       /* Trip is in 10-th of sec  */
       cnet_buff[1]=(ushort)input_value;
       break;
  }

if((response=caenet_comm(cnet_buff,sizeof(cnet_buff),NULL)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
  }
}

/***------------------------------------------------------------------------

  Speed_test

    --------------------------------------------------------------------***/
static void speed_test(void)
{
int i,response;
char sy527ident[12],loopdata[12];
char tempbuff[22];
code=IDENT;                                    /* To see if sy527 is present */
if((response=caenet_comm(NULL,0,tempbuff)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
   return;
  }
for(i=0;i<11;i++)
    sy527ident[i]=tempbuff[2*i+1];
sy527ident[i]='\0';

con_puts(" Looping, press any key to exit ... ");
/* Loop until one presses a key */
while(!con_kbhit())
     {
      if((response=caenet_comm(NULL,0,tempbuff)) != TUTTOK)
        {
         con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
         con_puts(" Press any key to continue ");
         con_getch();
         return;
        }

      for(i=0;i<11;i++)
          loopdata[i]=tempbuff[2*i+1];
      loopdata[i]='\0';
      if(strcmp(sy527ident,loopdata))    /* Data read in loop are not good */
        {
         con_printf(" Test_loop error: String read = %s\n",loopdata);
         con_puts(" Press any key to continue ");
         con_getch();
         return;
        }
     }  /* end while */
con_getch();
}

/***------------------------------------------------------------------------

  Clear_Alarm

    --------------------------------------------------------------------***/
static void clear_alarm(void)
{
int response;

code = CLEAR_ALARM;
if((response=caenet_comm(NULL,0,NULL)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
  }
}

/***------------------------------------------------------------------------

  Print_Status_Level_Value

    --------------------------------------------------------------------***/
static void print_status_level_value(void)
{
gotoxy(31,12);
if(status_alarm.level)
   con_printf("High");
else
   con_printf("Low ");
}

/***------------------------------------------------------------------------

  Print_Status_Pulsed_Value

    --------------------------------------------------------------------***/
static void print_status_pulsed_value(void)
{
gotoxy(31,13);
if(status_alarm.pulsed)
   con_printf("Pulsed ");
else
   con_printf("Level  ");
}

/***------------------------------------------------------------------------

  Print_Status_Ovc_Value

    --------------------------------------------------------------------***/
static void print_status_ovc_value(void)
{
gotoxy(31,15);
if(status_alarm.ovc)
   con_printf("On  ");
else
   con_printf("Off ");
}

/***------------------------------------------------------------------------

  Print_Status_Ovv_Value

    --------------------------------------------------------------------***/
static void print_status_ovv_value(void)
{
gotoxy(31,16);
if(status_alarm.ovv)
   con_printf("On  ");
else
   con_printf("Off ");
}

/***------------------------------------------------------------------------

  Print_Status_Unv_Value

    --------------------------------------------------------------------***/
static void print_status_unv_value(void)
{
gotoxy(31,17);
if(status_alarm.unv)
   con_printf("On  ");
else
   con_printf("Off ");
}

/***------------------------------------------------------------------------

  Status_Menu

    --------------------------------------------------------------------***/
static int  status_menu(void)
{
int     response;
ushort  value[2];

clrscr();

code = READ_GEN_STATUS;
if((response=caenet_comm(NULL,0,value)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
   return 0;
  }

memcpy(&status_alarm,value,sizeof(short));

gotoxy(7,9);
highvideo();
con_printf("Select Status Alarm Mode");
normvideo();

gotoxy(1,12);
con_printf("      A)  Normal Level:");
print_status_level_value();

gotoxy(1,13);
con_printf("      B)  Alarm  Type :");
print_status_pulsed_value();

gotoxy(1,15);
con_printf("      D)  OVC    Alarm:");
print_status_ovc_value();

gotoxy(1,16);
con_printf("      E)  OVV    Alarm:");
print_status_ovv_value();

gotoxy(1,17);
con_printf("      F)  UNV    Alarm:");
print_status_unv_value();

gotoxy(1,20);
con_printf("      Q)  Quit");

gotoxy(7,23);
con_printf("Select item\r\n");

return 1;
}

/***------------------------------------------------------------------------

  Set_Status_Alarm

    --------------------------------------------------------------------***/
static void set_status_alarm(void)
{
int c, modified, response;

if(!status_menu())
   return;

while(1)
  {
   modified = 0;

   c = tolower(con_getch());

   switch (c)
     {
      case  'a' : status_alarm.level = !status_alarm.level;
                  print_status_level_value();
                  modified = 1;
                  break;

      case  'b' : status_alarm.pulsed = !status_alarm.pulsed;
                  print_status_pulsed_value();
                  modified = 1;
                  break;

      case  'd' : status_alarm.ovc = !status_alarm.ovc;
                  print_status_ovc_value();
                  modified = 1;
                  break;

      case  'e' : status_alarm.ovv = !status_alarm.ovv;
                  print_status_ovv_value();
                  modified = 1;
                  break;

      case  'f' : status_alarm.unv = !status_alarm.unv;
                  print_status_unv_value();
                  modified = 1;
                  break;
     }  /* end switch */

   if(modified)
     {
      code = SET_STATUS_ALARM;
      if((response=caenet_comm(&status_alarm,sizeof(short),NULL)) != TUTTOK)
        {
         con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
         con_puts(" Press any key to continue ");
         con_getch();
         return;
        }
      modified = 0;
     }
   if(c == 'q')
      break;

   gotoxy(1,24);
  }     /* end while(1) */
}

/***------------------------------------------------------------------------

  Lock_Keyboard

    --------------------------------------------------------------------***/
static void lock_keyboard(void)
{
int response;

code = LOCK_KEYBOARD;
if((response=caenet_comm(NULL,0,NULL)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
  }
}

/***------------------------------------------------------------------------

  Unlock_Keyboard

    --------------------------------------------------------------------***/
static void unlock_keyboard(void)
{
int response;

code = UNLOCK_KEYBOARD;
if((response=caenet_comm(NULL,0,NULL)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
  }
}

/***------------------------------------------------------------------------

  Kill_Channels

    --------------------------------------------------------------------***/
static void kill_channels(void)
{
int c, response;

clrscr();
gotoxy(2,9);
con_printf("KILL ALL Channels. Are you sure ? (Y/N) [N]: ");
for(;;)
   {
    c = tolower(con_getch());
    if( c == 'y' || c == 'n' || c == '\r' )
       break;
   }
if( c == 'n' || c == '\r' )
   return;

con_putch('Y');

code = KILL_CHANNELS_1;
if((response=caenet_comm(NULL,0,NULL)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
  }

con_printf("\n\n Executing ... \n");

code = KILL_CHANNELS_2;
if((response=caenet_comm(NULL,0,NULL)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
  }
}

/***------------------------------------------------------------------------

  Format_EEPROM

    --------------------------------------------------------------------***/
static void format_eeprom(void)
{
int c, response;

clrscr();
gotoxy(2,9);
con_printf("Format EEPROM. Are you sure ? (Y/N) [N]: ");
for(;;)
   {
    c = tolower(con_getch());
    if( c == 'y' || c == 'n' || c == '\r' )
       break;
   }
if( c == 'n' || c == '\r' )
   return;

con_putch('Y');

code = FORMAT_EEPROM_1;
if((response=caenet_comm(NULL,0,NULL)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
  }

con_printf("\n\n Executing ... \n");

code = FORMAT_EEPROM_2;
if((response=caenet_comm(NULL,0,NULL)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
  }
}

/***------------------------------------------------------------------------

  Fpan_Stat

    --------------------------------------------------------------------***/
static void fpan_stat(void)
{
int     response;
ushort  value[2];

clrscr();

code = READ_GEN_STATUS;
if((response=caenet_comm(NULL,0,value)) != TUTTOK)
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
   return;
  }

gotoxy(7,5);
highvideo();
con_printf("SY527 Front Panel Status");
normvideo();

gotoxy(1,10);
con_printf("              V Selected       :");
gotoxy(1,12);
con_printf("              I Selected       :");
gotoxy(1,14);
con_printf("              KILL Status      :");
gotoxy(1,16);
con_printf("              Interlock Status :");
gotoxy(1,18);
con_printf("              HV Enable        :");

gotoxy(7,23);
con_printf("Press any key to exit");

highvideo();

while(!con_kbhit())
  {
   gotoxy(34,10);
   con_printf( (value[1] & (1<<0)) ? "V1SEL" : "V0SEL" );
   gotoxy(34,12);
   con_printf( (value[1] & (1<<1)) ? "I1SEL" : "I0SEL" );
   gotoxy(34,14);
   con_printf( (value[1] & (1<<2)) ? "On " : "Off" );
   gotoxy(34,16);
   con_printf( (value[1] & (1<<3)) ? "On " : "Off" );
   gotoxy(34,18);
   con_printf( (value[1] & (1<<4)) ? "On " : "Off" );

   if((response=caenet_comm(NULL,0,value)) != TUTTOK)
     {
      con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
      con_puts(" Press any key to continue ");
      break;
     }

   delay(500);
  }

con_getch();
normvideo();
}

/***------------------------------------------------------------------------

  List_ch_grp

    --------------------------------------------------------------------***/
static void list_ch_grp(void)
{
int    i, j, response,
       g, endloop = 0;
short  group, d, data[6+320*2];

do
  {
   clrscr();
   gotoxy(1,3);
   con_printf(" Input Group Number [0 .. 15]: ");
   con_scanf("%d",&g);
   if( g >= 0 && g < 16 )
      endloop = 1;
  }
while( !endloop );

code = LIST_CH_GRP;
group = g;

if( ( response = caenet_comm(&group,sizeof(group),data) ) != TUTTOK )
  {
   con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
   con_puts(" Press any key to continue ");
   con_getch();
   return;
  }

clrscr();
con_printf(" Channels List of Group %02d",g);
for( i = 0, j = 0 ; ( d = data[6+2*i] ) != -1 ;  )
   {
    gotoxy(1+5*j,3+(i%22));
    con_printf("%d.%02d",d>>8,d&0xff);
    i++;
    if( !(i%22) )
       j++;
   }
con_getch();
}

/***------------------------------------------------------------------------

  Add_rem_ch_grp

    --------------------------------------------------------------------***/
static void add_rem_ch_grp(int type)
{
int    bd, ch, loop,
       response,
       g, endloop = 0;
short  cbuff[2];
char   *file = ( type == ADD ) ? "addchgrp.txt" : "remchgrp.txt";
FILE   *fp;
time_t t1, t2;

do
  {
   clrscr();
   con_printf("\n\n Input Group Number [1 .. 15]: ");
   con_scanf("%d",&g);
   if( g >= 1 && g < 16 )
      endloop = 1;
  }
while( !endloop );

if( ( fp = fopen(file,"r") ) == NULL )
  {
   con_printf("\n Add_rem_ch_grp: Problems opening file %s\n",file);
   con_puts(" Press any key to continue ");
   con_getch();
   return;
  }

code = ( type == ADD ) ? ADD_CH_GRP : REM_CH_GRP;

/*
  File Format:
  0  10
  2  1
  3  3
  3  4
  3  5
  -1
  Where the first number is the board and the second one is the channel
*/
endloop = loop = 0;
time(&t1);
do
  {
   fscanf(fp,"%d",&bd);
   if( bd == -1 )
      endloop = 1;
   else
     {
      fscanf(fp,"%d",&ch);
      cbuff[0] = g;
      cbuff[1] = (bd<<8) | ch;
      do
        {
         if(    ( response = caenet_comm(cbuff,sizeof(cbuff),NULL ) ) != TUTTOK
             && ( response != -256 ) )
             con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
         if( response == -256 )
            delay(100);
        }
      while( response != TUTTOK );
      loop++;
     }
  }
while( !endloop );
time(&t2);
con_printf("\n %s %d Channels %s Group %d: %ld seconds elapsed\n",
        (type==ADD)?"Added":"Removed",loop,
        (type==ADD)?"to":"from",g,t2-t1);
con_getch();
fclose(fp);
}

/***------------------------------------------------------------------------

  Add_rem_all_grp

    --------------------------------------------------------------------***/
static void add_rem_all_grp(int type)
{
int    bd, ch, loop,
       response,
       g, endloop = 0;
short  cbuff[2];
char   *file = ( type == ADD ) ? "addchgrp.txt" : "remchgrp.txt";
FILE   *fp;
time_t t1, t2;

code = ( type == ADD ) ? ADD_CH_GRP : REM_CH_GRP;

if( ( fp = fopen(file,"r") ) == NULL )
  {
   con_printf("\n Add_rem_ch_grp: Problems opening file %s\n",file);
   con_puts(" Press any key to continue ");
   con_getch();
   return;
  }

for( g = 1 ; g < 16 ; g++ )
   {
/*
  File Format:
  0  10
  2  1
  3  3
  3  4
  3  5
  -1
  Where the first number is the board and the second one is the channel
*/
    endloop = loop = 0;
    time(&t1);
    fseek(fp,0L,SEEK_SET);
    do
     {
      fscanf(fp,"%d",&bd);
      if( bd == -1 )
         endloop = 1;
      else
        {
         fscanf(fp,"%d",&ch);
         cbuff[0] = g;
         cbuff[1] = (bd<<8) | ch;
         do
           {
            if(    ( response = caenet_comm(cbuff,sizeof(cbuff),NULL ) ) != TUTTOK
                && ( response != -256 ) )
              {
               con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
               con_puts(" Press any key to continue ");
               con_getch();
               break;
              }
            if( response == -256 )
               delay(100);
           }
         while( response != TUTTOK );
         loop++;
        }
     }
   while( !endloop );
   time(&t2);
   con_printf("\n %s %d Channels %s Group %d: %ld seconds elapsed\n",
           (type==ADD)?"Added":"Removed",loop,
           (type==ADD)?"to":"from",g,t2-t1);

  }

con_getch();
fclose(fp);
}

/***------------------------------------------------------------------------

  Logga

    --------------------------------------------------------------------***/
static void logga(char *fmt, ...)
{
va_list  argptr;
time_t   t;
FILE     *fp;

time(&t);

fp = fopen(logfile,"a+");
fprintf(fp,ctime(&t));
va_start(argptr,fmt);
vfprintf(fp,fmt,argptr);
fclose(fp);
va_end(argptr);
}

/***------------------------------------------------------------------------

  Mon_ch_grp
  Continously reads group data from crate 1 and 2 (old version)

  July  2001
  Similar to ch_logging

  The setup is read from sy527setupgrp.dat
  The data are saved into sy527_cr_bd_ch.dat
  The errors are logged into sy527.log

    --------------------------------------------------------------------***/
static void mon_ch_grp(void)
{
#if 0

// Old Version
  
FILE                *fp;
unsigned long       i;
short               group;
int                 response,
                    g, endloop = 0;
static struct hvrd  chrd[320];

do
  {
   clrscr();
   con_printf("\n\n Input Group Number [0 .. 15]: ");
   con_scanf("%d",&g);
   if( g >= 0 && g < 16 )
      endloop = 1;
  }
while( !endloop );

con_printf("\n\n Input Log File Name: ");
con_scanf("%s",logfile);

if( ( fp = fopen(logfile,"w") ) == NULL )
  {
   con_printf("\n Problems opening %s\n",logfile);
   con_getch();
   return;
  }
else
   fclose(fp);

code = MON_CH_GRP;
group = g;

con_printf("\n\n Reading loop, press any key to exit ... ");
logga("Start of test\n\n");

for( i = 0 ; !con_kbhit() ; i++ )
   {
    gotoxy(1,12);
    con_printf(" Iteration Nr. %ld",i);
	cratenum = 1; 
	if(( response = caenet_comm(&group,sizeof(group),chrd) ) != TUTTOK )
	  {
	   gotoxy(1,14);
     con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
	   logga("Caenet_comm: Crate %d, %s\n\n",cratenum,DecodeCAENETResp(response));
	   delay(500);
	  }
    else
     {
      gotoxy(1,14);
      con_printf("                                                          ");
     }

	cratenum = 2;
	if(( response = caenet_comm(&group,sizeof(group),chrd) ) != TUTTOK )
      {
       gotoxy(1,14);
       con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
	     logga("Caenet_comm: Crate %d, %s\n\n",cratenum,DecodeCAENETResp(response));
	     delay(500);
      }
	else
	 {
	  gotoxy(1,14);
    con_printf("                                                           ");
	 }
   } /* end for !con_kbhit */

 logga("End of test\nNumber of Iterations: %ld\n",i);

#else

// New Version

int                      g,cr,b,ch,index,iter,groups[100][16],
                         savecratenum, response;
FILE                     *f;
time_t                   t;
ushort                   group;
short                    d,data[6+320*2];
char                     *tok,line[100],cnetbuff1[MAX_LENGTH_FIFO],cnetbuff2[MAX_LENGTH_FIFO];
static struct hvrd       ch_read;
static struct v0i0set   { 
                          long v0; ushort i0;
                        } set; 
int                     size_of_set = sizeof(set.v0) + sizeof(set.i0);

clrscr();

strcpy(logfile,"sy527.log");
if( ( f = fopen(logfile,"a+") ) == NULL )
  {
   con_printf("\n Problems opening %s\n",logfile);
   con_getch();
   return;
  }
else
   fclose(f);

if( ( f = fopen("sy527setupgrp.dat","r") ) == NULL )
  {
    con_puts("Error opening sy527setupgrp.dat");
    con_getch();
    return;
  }


savecratenum = cratenum;

for( cr = 0 ; cr < 100 ; cr++ )
    for( g = 0 ; g < 16 ; g++ )
        groups[cr][g] = 0;

/*  
    The format of sy527setupgrp.dat is :

    cr1 gra grb grc
    crn grd gre grf

    Where cr1 and crn are the SY527's CAENET Crate Numbers, gra, ... grf are the groups 
*/
for( ; fgets(line,sizeof(line),f) ; )
  {
    tok = strtok(line," ");
    sscanf(tok,"%d",&cr);
    while( 1 )
      {
        tok = strtok(NULL," ");
        if( tok == NULL )
           break;
        sscanf(tok,"%d",&g);  
        groups[cr][g] = 1;
      }
  }

highvideo();
con_puts("\n\n                         ---  Channels Groups Logging ---       \n\n\n\n\n ");
normvideo();

gotoxy(1,23);
con_puts(" Press any key to exit ");

iter = 0;
while( !con_kbhit() )
    {
      gotoxy(1,10);
      con_printf(" Iteration Nr. %d ...",iter);
      for( cr = 0 ; cr < 100 ; cr++ )
        for( g = 0 ; g < 16 ; g++ )
          if( groups[cr][g] )
            {
             if( con_kbhit() )
                break;

             code = LIST_CH_GRP;
             group = g;
             cratenum = cr;

             if( ( response = caenet_comm(&group,sizeof(group),data) ) != TUTTOK )
               {
                 logga(" Caenet_comm: %s\n Crate %0d, Group %0d\n\n",
                        DecodeCAENETResp(response), cr, g);
                 gotoxy(1,14);
                 con_printf(" Caenet_comm: %s  Crate %0d, Group %0d\n", 
                   DecodeCAENETResp(response), cr, g);
                 continue;
               }

             code = MON_CH_GRP;

             response = caenet_comm(&group,sizeof(group),cnetbuff1);
             if( response != TUTTOK )
               {
                 logga(" Caenet_comm: %s\n Crate %0d, Group %0d\n\n",
                        DecodeCAENETResp(response), cr, g);
                 gotoxy(1,14);
                 con_printf(" Caenet_comm: %s  Crate %0d, Group %0d\n", 
                   DecodeCAENETResp(response), cr, g);
                 continue;
               }

             code = V0SET_CH_GRP;

             response = caenet_comm(&group,sizeof(group),cnetbuff2);
             if( response != TUTTOK )
               {
                 logga(" Caenet_comm: %s\n Crate %0d, Group %0d\n\n",
                        DecodeCAENETResp(response), cr, g);
                 gotoxy(1,14);
                 con_printf(" Caenet_comm: %s  Crate %0d, Group %0d\n", 
                   DecodeCAENETResp(response), cr, g);
                 continue;
               }

            for( index = 0 ; ( d = data[6+2*index] ) != -1 ; index++ )
               {
                 b = d >> 8;
                 ch = d&0xff;

                 build_chrd_info(&ch_read,&cnetbuff1[index*size_of_chrd()]);

                 swap_byte(&cnetbuff2[index*size_of_set], size_of_set);

                 memcpy(&set.v0,&cnetbuff2[index*size_of_set],sizeof(set.v0));
                 swap_long(&set.v0);

                 time(&t);
                 sprintf(line,"sy527_%0d_%0d_%0d.dat",cr,b,ch);
                 f = fopen(line,"a+");
                 fprintf(f,"%lu\t%ld\t%d\t%ld\n",t,ch_read.vread,ch_read.iread,set.v0);
                 fclose(f);
               }
            }
       delay(2000);

       iter++;
     }  /* End while */

cratenum = savecratenum;
#endif
}

/***------------------------------------------------------------------------

  Grpmenu

    --------------------------------------------------------------------***/
static void grpmenu(void)
{
for(;;)
    switch(makegrpmenu())
       {
     case 'A':
        add_rem_ch_grp(ADD);
        break;
     case 'B':
        add_rem_ch_grp(REM);
        break;
     case 'C':
        add_rem_all_grp(ADD);
        break;
     case 'D':
        add_rem_all_grp(REM);
        break;
     case 'E':
        list_ch_grp();
        break;
     case 'F':
        par_set(GRP);
        break;
     case 'G':
        mon_ch_grp();
        break;
     case 'Q':
        return;
     default:
        break;
       }
}



/***------------------------------------------------------------------------

  End

    --------------------------------------------------------------------***/
static void end(void)
{
	clrscr();
	HSCAENETCardEnd(HSCAENETDev);
	con_end();
	exit(0);
}

/***------------------------------------------------------------------------

  Usage

    --------------------------------------------------------------------***/
static void usage(void)
{
    puts("\n\n Usage: SY527ctrl <Par1> <Par2> <Par3>");
    puts("    ");
    puts(" where: ");
    puts("    ");
    puts(" <Par1> = Name of the H.S. CAENET Device (can be: A303, A303A or A1303)");
    puts(" <Par2> = If <Par1> is A303 or A303A, this is its I/O Base address (in hex),");
    puts("          otherwise it enumerates the A1303: 0, 1, ... ");
    puts(" <Par3> = SY527 Caenet number (in hex)   ");
}

/***------------------------------------------------------------------------

  Main Program

    --------------------------------------------------------------------***/
int main(int argc, char **argv)
{
	ulong  a303addr;
	ulong  timeout = 100;         // Corresponds to 1 sec
	int response, a1303index;

	if(argc != 4)
	{
		usage();
		exit(0);
	}

	sscanf(argv[3],"%x",&cratenum);

/*
   HSCAENETCardInit is the first CAENET routine to call !!
*/

	if( !strcmp(argv[1], "A303") || !strcmp(argv[1], "A303A") )
	{
		sscanf(argv[2],"%lx",&a303addr);

		HSCAENETDev = HSCAENETCardInit("A303", &a303addr);
		if( HSCAENETDev == -1 )
		{
		    puts(" HSCAENETCardInit failed");
			exit(0);
		}
	}
	else if( !strcmp(argv[1], "A1303") )
	{
		sscanf(argv[2], "%d", &a1303index);

		HSCAENETDev = HSCAENETCardInit("A1303", &a1303index);
		if( HSCAENETDev == -1 )
		{
		    puts(" HSCAENETCardInit failed");
			exit(0);
		}
	}
	else
	{
		usage();
		exit(0);
	}

	con_init();

	if( ( response = HSCAENETCardReset(HSCAENETDev) ) != TUTTOK )
	{
		con_printf(" HSCAENETCardReset failed: %s\n",DecodeCAENETResp(response));
		end();
	}

	if( ( response = HSCAENETTimeout(HSCAENETDev, timeout) ) != TUTTOK )
	{
		con_printf(" HSCAENETTimeout failed: %s\n",DecodeCAENETResp(response));
		end();
	}


	// Main Loop

	for( ; ; )
	    switch(makemenu())
		{
			case 'A':
				read_ident(); 
		        break;
			case 'B':
				crate_map();
				break;
			case 'C':
				ch_monitor();
				break;
			case 'D':
				speed_test();
				break;
			case 'E':
				par_set(NO_GRP);
				break;
			case 'F':
				clear_alarm();
				break;
			case 'G':
				set_status_alarm();
				break;
			case 'H':
				lock_keyboard();
				break;
			case 'I':
				unlock_keyboard();
				break;
			case 'J':
				kill_channels();
				break;
			case 'K':
				fpan_stat();
				break;
			case 'L':
				grpmenu();
				break;
			case 'M':
    			format_eeprom();
				break;
			case 'N':
    			ch_logging();
				break;
			case 'Q':
				end();
				break;
			default:
				break;
       }
}
