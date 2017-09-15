/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    SY127CTRL.C                                               	   */
/*                                                                         */
/*    Demonstration about the use of Caenet Routines in communication      */
/*    between A303/A and A1303 modules and SY127 High Voltage System       */
/*    equipped with module A128A High Speed Caenet Communication           */
/*    Controller                                                           */
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
#include  <stdlib.h>
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

#define   ESC                                   0x1b
#define   CR                                    0x0d
#define   BLANK                                 0x20

#define   V0SET                                    0
#define   V1SET                                    1
#define   I0SET                                    2
#define   I1SET                                    3
#define   VMAX                                     4
#define   RUP                                      5
#define   RDWN                                     6
#define   TRIP                                     7
#define   ON_OFF                                   8
#define   CHNAME                                   9

#define   MAKE_CODE(ch,cod)      (((ch)<<8) | (cod))

/*
  Some of the Caenet Codes
*/
#define   IDENT                                  0x0
#define   READ_CH                                0x1
#define   READ_SETTINGS                          0x2
#define   CRATE_MAP                              0x3
#define   READ_PROT                              0x4

#define   FORMAT_EEPROM_1                       0x30
#define   FORMAT_EEPROM_2                       0x31
#define   CLEAR_ALARM                           0x32
#define   SET_PROT                              0x39

/*
  An useful macro
*/
#define caenet_comm(s, w, d) HSCAENETComm(HSCAENETDev, code, cratenum, (s), (w), (d))

/*
  The following structure contains all the useful information about
  the settings and monitorings of a channel
*/
struct hvch
{
 ushort  v0set;
 ushort  v1set;
 ushort  i0set;
 ushort  i1set;
 ushort  rup;
 ushort  rdwn;
 ushort  trip;
 ushort  status;
 ushort  gr_ass;
 ushort  vread;
 ushort  iread;
 ushort  st_phase;
 ushort  st_time;
 ushort  mod_type;
 char    chname[10];
};

/*
  The following structure contains all the useful information about
  the protection word of SY127
*/
struct prot_w
{
 unsigned  pwon    :1;
 unsigned  pswen   :1;
 unsigned  keyben  :1;
 unsigned  alarms  :1;
 unsigned  unused  :12;
};

/*
  Globals
*/
static int            cratenum, code;
static int            HSCAENETDev;
static struct prot_w  protect;

static char *modul_type[] =                     /* Modules types */
{
  " Not Present     ",
  " 2KV    3mA      ",
  " 3KV    3mA      ",
  " 4KV    2mA      ",
  " 8KV    500uA    ",
  " 6KV    1mA      ",
  " 800.0V 500.0uA  ",
  " 8KV    200.0uA  ",
  " 6KV    200.0uA  ",
  " 200.0V 200.0uA  ",
  " 2KV    200.0uA  ",
  " 4KV    200.0uA  ",
  " 6KV    1mA      ",
  " Not Implemented ",
  " 3KV    3mA      ",
  " 4KV    2mA      ",
  " 800.0V 200.0uA  ",
  " 8KV    400.0uA  ",
  " 8KV    200.0uA  ",
  " 10KV   1mA      ",
  " 2KV    6mA      ",
  " 800.0V 400.0uA  ",
  " 10KV   200.0uA  ",
  " 15KV   200.0uA  ",
  " 15KV   1mA      ",
  " 20KV   200.0uA  ",
  " 2.5KV  5mA      ",
  " 1KV    10mA     ",
  " 1KV    200.0uA  ",
  " 20KV   500uA    ",
  " 10KV   2mA      ",
  " I/O Module      ",
  " 200.0V 40uA     ",
  " 800.0V 40uA     ",
  " 2KV    40uA     ",
  " 4KV    40uA     ",
  " 6KV    40uA     ",
  " 8KV    40uA     ",
  " 10KV   40uA     ",
  " 15KV   40uA     ",
  " 20KV   40uA     ",
  " Not Implemented ",
  " Not Implemented ",
  " Not Implemented ",
  " Not Implemented ",
  " Special Module  ",
  " Not Implemented ",
  " Not Implemented ",
};

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
	con_puts(" [B] - Board  0  Monitor ");
	con_puts(" [C] - Board  1  Monitor ");
	con_puts(" [D] - Board  2  Monitor ");
	con_puts(" [E] - Board  3  Monitor ");
	con_puts(" [F] - Board  4  Monitor ");
	con_puts(" [G] - Board  5  Monitor ");
	con_puts(" [H] - Board  6  Monitor ");
	con_puts(" [I] - Board  7  Monitor ");
	con_puts(" [J] - Board  8  Monitor ");
	con_puts(" [K] - Board  9  Monitor ");
	con_puts(" [L] - Speed test              ");
	con_puts(" [M] - Parameter Set           ");
	con_puts(" [N] - Crate Map         ");
	con_puts(" [O] - Format EEPROM           ");
	con_puts(" [P] - Clear Alarms            ");
	con_puts(" [R] - Set Protections         ");
	con_puts("\n\n [Q] - Quit ");

	return toupper(con_getch());
}

/***------------------------------------------------------------------------

  Read_Ident

    --------------------------------------------------------------------***/
static void read_ident(void)
{
	int   i,response;
	char  sy127ident[12];
	char  tempbuff[80];

	code = IDENT;                           /* To see if sy127 is present */
	if( ( response = caenet_comm(NULL,0,tempbuff) ) != TUTTOK )
	{
		con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
		con_puts(" Press any key to continue ");
		con_getch();
		return;
	}

	for( i = 0 ; i < 10 ; i++ )
		sy127ident[i] = tempbuff[2*i];
	sy127ident[i] = '\0';

	con_printf(" The module has answered : %s\n",sy127ident);
	con_puts(" Press any key to continue ");
	con_getch();
}


/***------------------------------------------------------------------------

  Crate_Map

    --------------------------------------------------------------------***/
static void crate_map(void)
{
	int  bd, response;
	char cm[10];

	code = CRATE_MAP;
	if( ( response = caenet_comm(NULL,0,cm) ) != TUTTOK )
	{
		con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
		con_puts(" Press any key to continue ");
		con_getch();
		return;
	}

	clrscr();
	con_puts("\n\n                         ---  Crate Map ---       \n\n\n\n\n ");

	for( bd = 0 ; bd < 10 ; bd++ )
	{
		char *type;

		if( cm[bd] == 0 )
			type = " ";
		else if( cm[bd] > 0 )
			type = " Positive ";
		else
			type = " Negative ";
		con_printf(" Slot %d - ",bd);
		con_printf(" %s %s\n",modul_type[cm[bd] & 0x7f], type);
	}
	con_puts("\n\n\n   Press any key to continue ");
	con_getch();
}

/***------------------------------------------------------------------------

  Build_Chrd_Info

    --------------------------------------------------------------------***/
static void build_chrd_info(struct hvch *ch, char *cnetbuff)
{
	int i = sizeof(ch->v0set);

	memcpy(&ch->v0set, cnetbuff,   sizeof(ch->v0set));
	memcpy(&ch->v1set, cnetbuff+i, sizeof(ch->v1set));
	i += sizeof(ch->v1set);
	memcpy(&ch->i0set, cnetbuff+i, sizeof(ch->i0set));
	i += sizeof(ch->i0set);
	memcpy(&ch->i1set, cnetbuff+i, sizeof(ch->i1set));
	i += sizeof(ch->i1set);
	memcpy(&ch->rup, cnetbuff+i, sizeof(ch->rup));
	i += sizeof(ch->rup);
	memcpy(&ch->rdwn, cnetbuff+i, sizeof(ch->rdwn));
	i += sizeof(ch->rdwn);
	memcpy(&ch->trip, cnetbuff+i, sizeof(ch->trip));
	i += sizeof(ch->trip);
	memcpy(&ch->status, cnetbuff+i, sizeof(ch->status));
	i += sizeof(ch->status);
	memcpy(&ch->gr_ass, cnetbuff+i, sizeof(ch->gr_ass));
	i += sizeof(ch->gr_ass);
	memcpy(&ch->vread, cnetbuff+i, sizeof(ch->vread));
	i += sizeof(ch->vread);
	memcpy(&ch->iread, cnetbuff+i, sizeof(ch->iread));
	i += sizeof(ch->iread);
	memcpy(&ch->st_phase, cnetbuff+i, sizeof(ch->st_phase));
	i += sizeof(ch->st_phase);
	memcpy(&ch->st_time, cnetbuff+i, sizeof(ch->st_time));
	i += sizeof(ch->st_time);
	memcpy(&ch->mod_type, cnetbuff+i, sizeof(ch->mod_type));
	i += sizeof(ch->mod_type);
	memcpy(&ch->chname, cnetbuff+i, sizeof(ch->chname));
}

/***------------------------------------------------------------------------

  Ch_monitor

    --------------------------------------------------------------------***/
static void ch_monitor(int group)
{
	int                      i,
	                         caratt='P',
		                     response,
	                         chs=group*4;
	char                     cnetbuff[MAX_LENGTH_FIFO];
	float                    scalei,scalev;
	ushort                   channel;
	static int               page=0;
	static struct hvch       ch_read[4];      /* Four channel each board */

	scalev = 1.0;
	scalei = 1.0;
	
	clrscr();
	highvideo();
	if( !page )
	   con_puts
	(" Channel     Vmon     Imon    V0set    I0set    V1set    I1set   Groups  Ch# ");
	else
	   con_puts
	(" Channel     Rup    Rdwn     Trip     Status    STPhase STTime ModType Ch# ");
	normvideo();

	gotoxy(1,23);
	con_puts(" Press 'P' to change page, any other key to exit ");

	while(caratt == 'P') /* Loops until someone presses a key different from P  */
	{
/* First update from Caenet the information about the channels */
		for( i = 0 ; i < 4 ; i++ )
		{
			channel = (uchar)(chs+i);
			code = MAKE_CODE(channel,READ_CH);
			response = caenet_comm(NULL,0,cnetbuff);
			if( response != TUTTOK )
            {
				gotoxy(1,22);
				con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
				con_puts(" Press any key to continue                         ");
				con_getch();
				return;
            }
			build_chrd_info(&ch_read[i], cnetbuff);
        }

/* Display the information */
		if( !page )                                    /* Page 0 of display */
			for( i = 0 ; i < 4 ; i++ )
            {
	             gotoxy(1,i+5);
		         con_printf(" %s",ch_read[i].chname);
			     gotoxy(12,i+5);
				 con_printf
			("%07.2f  %07.2f  %07.2f  %07.2f  %07.2f  %07.2f  %4x    %2d \n",
			ch_read[i].vread/scalev,ch_read[i].iread/scalei,ch_read[i].v0set/scalev,
			ch_read[i].i0set/scalei,ch_read[i].v1set/scalev,ch_read[i].i1set/scalei,
			ch_read[i].gr_ass,chs+i);
            }
		else                                         /* Page 1 of display */
			for( i = 0 ; i < 4 ; i++ )
            {
			    gotoxy(1,i+5);
				 con_printf(" %s",ch_read[i].chname);
				gotoxy(14,i+5);
				con_printf
			("%3d     %3d    %4d       %4x     %4x    %4x   %4x    %2d\n",
			ch_read[i].rup,ch_read[i].rdwn,ch_read[i].trip,
			ch_read[i].status,ch_read[i].st_phase,ch_read[i].st_time,
			ch_read[i].mod_type,chs+i);
            }

/* Test the keyboard */
		if( con_kbhit() )                       /* A key has been pressed */
			if( ( caratt = toupper(con_getch() ) ) == 'P' ) /* Change page */
			{
				highvideo();
				page = !page;
				clrscr();
				if( page == 0 )
					con_puts
			(" Channel     Vmon    Imon    V0set    I0set    V1set    I1set    Groups  Ch# ");
				else
					con_puts
			(" Channel     Rup    Rdwn     Trip     Status    STPhase STTime ModType Ch# ");
				normvideo();
				gotoxy(1,23);
				con_puts(" Press 'P' to change page, any other key to exit ");
			}

	}  /* End while */
}

/***------------------------------------------------------------------------

  Par_set

    --------------------------------------------------------------------***/
static void par_set(void)
{
	float        input_value,
		         scale;
	static float pow10[] =  { 1.0, 1.0, 1.0};  /* Per ora scale = 1 ... */
	ushort       channel,value;
	int          i,
	             response,
	             par=0;
	char         choiced_param[10], chname[12];
	static char  *param[] =
	{
		"v0set", "v1set", "i0set", "i1set", "vmax",
		"rup", "rdwn", "trip", "on/off","name", NULL
	};

	clrscr();
	con_printf("\n\n Channel: ");                     /* Choice the channel */
	con_scanf("%d",&i);
	channel=(uchar)i;
	con_puts(" Allowed parameters (lowercase only) are:");
	for( i=0 ; param[i] != NULL ; i++ )
		con_puts(param[i]);
	while(!par)
	{
		con_printf("\n Parameter to set: ");      /* Choice the parameter */
		con_scanf("%s",choiced_param);
		for( i=0 ; param[i] != NULL ; i++ )
			if(!strcmp(param[i],choiced_param))
			{
				par=1;
				break;
            }
      
		if(param[i] == NULL)
			con_puts(" Sorry, this parameter is not allowed");
	}
	con_printf(" New value :");                       /* Choice the value         */
	if(i == CHNAME)
	{
		con_puts(" Function not yet implemented ");
		con_puts(" Press any key to continue ");
		con_getch();
		return;
	}
	else
		con_scanf("%f",&input_value);

	switch(i)                                     /* Decode the par.          */
	{
		case V0SET:
			code=MAKE_CODE(channel,16);
			scale=pow10[0];
			input_value*=scale;
			value=(ushort)input_value;
			break;
		case V1SET:
			code=MAKE_CODE(channel,17);
			scale=pow10[0];
			input_value*=scale;
			value=(ushort)input_value;
			break;
		case I0SET:
			code=MAKE_CODE(channel,18);
			scale=pow10[0];
			input_value*=scale;
			value=(ushort)input_value;
			break;
		case I1SET:
			code=MAKE_CODE(channel,19);
			scale=pow10[0];
			input_value*=scale;
			value=(ushort)input_value;
			break;
		case VMAX:
			code=MAKE_CODE(channel,20);
			value=(ushort)input_value;
			break;
		case  RUP:
			code=MAKE_CODE(channel,21);
			value=(ushort)input_value;
			break;
		case RDWN:
			code=MAKE_CODE(channel,22);
			value=(ushort)input_value;
			break;
		case TRIP:
			code=MAKE_CODE(channel,23);
			input_value*=10;                       /* Trip is in 10-th of sec  */
			value=(ushort)input_value;
			break;
		case  ON_OFF:
			code=MAKE_CODE(channel,24);
			value=(ushort)input_value;
			break;
		case CHNAME:
			code=MAKE_CODE(channel,25);
	       break;
	}

	if(i == CHNAME)
	{
		if((response=caenet_comm(chname,sizeof(chname),NULL)) != TUTTOK)
		{
			con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
			con_puts(" Press any key to continue ");
			con_getch();
		}
	}
	else
	{
		if((response=caenet_comm(&value,sizeof(ushort),NULL)) != TUTTOK)
		{
			con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
			con_puts(" Press any key to continue ");
			con_getch();
		}
	}
}

/***------------------------------------------------------------------------

  Speed_test

    --------------------------------------------------------------------***/
static void speed_test(void)
{
	int  i,response;
	char sy127ident[12],loopdata[12];
	char tempbuff[80];

	code = IDENT;                                    /* To see if sy127 is present */
	if( ( response = caenet_comm(NULL,0,tempbuff) ) != TUTTOK )
	{
		con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
		con_puts(" Press any key to continue ");
		con_getch();
		return;
	}

	for( i = 0 ; i < 10 ; i++ )
		sy127ident[i] = tempbuff[2*i];
	sy127ident[i] = '\0';

	con_puts(" Looping, press any key to exit ... ");
	/* Loop until one presses a key */
	while( !con_kbhit() )
    {
		if( ( response = caenet_comm(NULL,0,tempbuff) ) != TUTTOK )
		{
			con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
		    con_puts(" Press any key to continue ");
			con_getch();
			return;
        }

		for( i = 0 ; i < 10 ; i++ )
			loopdata[i] = tempbuff[2*i];
		loopdata[i]='\0';
		if( strcmp(sy127ident,loopdata) )    /* Data read in loop are not good */
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

  Format_EEPROM

    --------------------------------------------------------------------***/
static void format_eeprom(void)
{
	int c, response;

	clrscr();
	gotoxy(2,9);
	con_printf("FORMAT EEPROM. Are you sure ? (Y/N) [N]: ");
	
	for(;;)
	{
		c = tolower(con_getch());
		if( c == 'y' || c == 'n' || c == CR )
			break;
	}
	if( c == 'n' || c == CR )
		return;

	con_putch('Y');

	code = FORMAT_EEPROM_1;
	if( ( response = caenet_comm(NULL,0,NULL) ) != TUTTOK )
	{
		con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
		con_puts(" Press any key to continue ");
		con_getch();
	}

	con_printf("\n\n Executing ... \n");

	code = FORMAT_EEPROM_2;
	if( ( response = caenet_comm(NULL,0,NULL) ) != TUTTOK )
	{
		con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
		con_puts(" Press any key to continue ");
		con_getch();
	}
}

/***------------------------------------------------------------------------

  Clear_Alarm

    --------------------------------------------------------------------***/
static void clear_alarm(void)
{
	int response;

	code = CLEAR_ALARM;
	if( ( response = caenet_comm(NULL,0,NULL) ) != TUTTOK )
	{
		con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
		con_puts(" Press any key to continue ");
		con_getch();
	}
}

/***------------------------------------------------------------------------

  Print_Power_On

    --------------------------------------------------------------------***/
static void print_power_on(void)
{
	gotoxy(31,11);
	if( protect.pwon )
		con_printf("Enabled ");
	else
		con_printf("Disabled");
}

/***------------------------------------------------------------------------

  Print_Passw

    --------------------------------------------------------------------***/
static void print_passw(void)
{
	gotoxy(31,12);
	if( protect.pswen )
		con_printf("Enabled ");
	else
		con_printf("Disabled");
}

/***------------------------------------------------------------------------

  Print_Keyb

    --------------------------------------------------------------------***/
static void print_keyb(void)
{
	gotoxy(31,13);
	if( protect.keyben )
		con_printf("Enabled ");
	else
		con_printf("Disabled");
}

/***------------------------------------------------------------------------

  Print_Alarms

    --------------------------------------------------------------------***/
static void print_alarms(void)
{
	gotoxy(31,15);
	if( protect.alarms )
		con_printf("Active   ");
	else
		con_printf("No Active");
}

/***------------------------------------------------------------------------

  Prot_Menu

    --------------------------------------------------------------------***/
static int  prot_menu(void)
{
	int      response;
	short    pr;

	clrscr();

	code = READ_PROT;
	if( ( response = caenet_comm(NULL,0,&pr) ) != TUTTOK )
	{
		con_printf(" Caenet_comm: %s\n",DecodeCAENETResp(response));
		con_puts(" Press any key to continue ");
		con_getch();
		return 0;
	}

	pr &= 7;

	memcpy(&protect,&pr,sizeof(short));

	gotoxy(7,9);
	highvideo();
	con_printf("Select Protections");
	normvideo();

	gotoxy(1,11);
	con_printf("      A)  Power ON    :");
	print_power_on();

	gotoxy(1,12);
	con_printf("      B)  Password    :");
	print_passw();

	gotoxy(1,13);
	con_printf("      C)  Keyboard    :");
	print_keyb();

	gotoxy(1,15);
	con_printf("          Alarms      :");
	print_alarms();

	gotoxy(1,17);
	con_printf("      Q)  Quit");

	gotoxy(7,23);
	con_printf("Select item\r\n");

	return 1;
}

/***------------------------------------------------------------------------

  Protections

    --------------------------------------------------------------------***/
static void protections(void)
{
	int c, modified, response;

	if(!prot_menu())
	   return;

	while(1)
	{
		modified = 0;

		c = tolower(con_getch());

		switch (c)
		{
			case  'a' : 
				protect.pwon = !protect.pwon;
				print_power_on();
				modified = 1;
                break;

			case  'b' : 
				protect.pswen = !protect.pswen;
                print_passw();
                modified = 1;
                break;

			case  'c' : 
				protect.keyben = !protect.keyben;
                print_keyb();
                modified = 1;
                break;
		}  /* end switch */

		if(modified)
		{
			code = SET_PROT;
			if( ( response = caenet_comm(&protect,sizeof(short),NULL) ) != TUTTOK )
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

  End

    --------------------------------------------------------------------***/
static void end(void)
{
	HSCAENETCardEnd(HSCAENETDev);
	con_end();
	exit(0);
}

/***------------------------------------------------------------------------

  Usage

    --------------------------------------------------------------------***/
static void usage(void)
{
    puts("\n\n Usage: SY127ctrl <Par1> <Par2> <Par3>");
    puts("    ");
    puts(" where: ");
    puts("    ");
    puts(" <Par1> = Name of the H.S. CAENET Device (can be: A303, A303A or A1303)");
    puts(" <Par2> = If <Par1> is A303 or A303A, this is its I/O Base address (in hex),");
    puts("          otherwise it enumerates the A1303: 0, 1, ... ");
    puts(" <Par3> = SY127 Caenet number (in hex)   ");
}

/***------------------------------------------------------------------------

  Main Program

    --------------------------------------------------------------------***/
int main(int argc, char **argv)
{
	ulong  a303addr;
	ulong  timeout = 100;         // Corresponds to 1 sec
	int    c = 0, response, a1303index;

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
		switch( c = makemenu() )
		{
			case 'A':
				read_ident();
				break;
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'H':
			case 'I':
			case 'J':
			case 'K':
				ch_monitor( c-'B' );
				break;
			case 'L':
				speed_test();
				break;
			case 'M':
				par_set();
				break;
			case 'N':
				crate_map();
				break;
			case 'O':
				format_eeprom();
				break;
			case 'P':
				clear_alarm();
				break;
			case 'R':
				protections();
				break;
			case 'Q':
				end();
				break;
			default:
				break;
		}
}
