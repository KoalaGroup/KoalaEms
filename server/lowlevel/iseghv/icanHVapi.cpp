//#############################################################################
//	icanHV api October 2005		Peter KŠmmerlinf, fzj zel, P.Kaemmerling@fz-juelich.de
//	
//	based on icanHV	iseg Spezialelektronik GmbH, Jens Ršmer 2004
//#############################################################################

#include <iostream>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>

#include "icanHVapi.h"
#include "libpcan.h"
#include "pcan.h"
#include "ican.h"

HANDLE hnd;
long ret;
TPCANRdMsg tmsg;
DWORD STime;
int errno;
char* str;
bool firstloop;

using namespace std;

//centralized entry point for all exits
static void ExitfCAN(char *ms, int error){
  if (hnd) {
    CAN_Close(hnd);
  }
  printf("icanHVcontrol finished %s: (%d)", ms,error);
  delete str;
  exit(error);
}


//handeles Ctrl-C user interrupt
static void signal_handler(int signal){
    ExitfCAN("",0);
}


// send a CAN meassage
static void Send2CAN(uw16 ID, uw8 *CANbuff)
{
    TPCANMsg msg;

    msg.ID=ID;
    msg.LEN=CANbuff[0];
    for (uw8 i=1; i<=CANbuff[0]; i++) {
        msg.DATA[i-1]=CANbuff[i];
    }
    CAN_Write(hnd, &msg);
}


// verbose *******************************************************************

int verb = 4;	// verbose level =0 quiet	=1 critical errors	=2 errors	=3 status	=4 comments	=5 program flow
int verb_count = 0;
int	verb_first = true;
#define verb_print_quiet	0	//0: do not print quiet		1:print quiet
#define	verb_qui	0
#define	verb_cri	1
#define	verb_err	2
#define	verb_sta	3
#define	verb_com	4
#define verb_flo	5

char*	verb_str[] = {"quiet", "critical error", "error", "status", "comment", "program flow", "test"};

int	vvv(int verb_match, char* verb_message, char* vFile, int vLine){	// output verbose-message 'verb', if 'verb_match' is within verbose_level (verb)
	if (verb_first == true){
		cout << '\n' << "#	level	actual	comment		message	sourcefile	line" << '\n';
		verb_first = false;
	}
	if (verb_match <= verb){
		cout << '\n' << verb_count++ << '\t' << verb << '\t' << verb_match << '\t';
		cout << verb_str[verb_match] << '\t' << ">>" << verb_message << "<<" << '\t' << vFile << '\t' << vLine <<'\n';}
}


int	vvs(int verb_match, char* verb_message, char* vFile, int vLine){	// output, if verb_match is exactly verbose_level (verb)
	if(verb_match == verb) vvv(verb_match, verb_message, vFile, vLine);
}

int	vvv(int verb_match, char* verb_message, int number, char* vFile, int vLine){
	#define strmax 256
	char	str1[strmax], str2[strmax];
	int		number;
	sprintf(str1, "%d", number);
	strcpy(str2, verb_message);
	strcat(str2, str1);
	vvv(verb_match, str2, vFile, vLine);}


//****************************************************************************
  
char *ptr;
uw8  buff[10];
DWORD btr;
firstloop=true;
errno=0;
  
  
int icanHV_init(int baudrate){
	int	ret;
	str1 = new char[255];
	str2 = new char[255];
	vvv(6, "verbose test 1", __FILE__, __LINE__);

	hnd=LINUX_CAN_Open("/dev/pcan0", 04000);  //O_NONBLOCK
	if (hnd!=NULL) {
		ret=CAN_VersionInfo(hnd, str);
		strcat(str2, "CAN version = ");
		strcat(str2, str1);
		vvv(4, str1);
		vvv(6, "verbose test 2", __FILE__, __LINE__);
		ptr=argv[1];
		switch (bautrate){
			case 20: ret=CAN_Init(hnd, CAN_BAUD_20K, CAN_INIT_TYPE_EX);
			case 50: ret=CAN_Init(hnd, CAN_BAUD_50K, CAN_INIT_TYPE_EX);
			case 100: ret=CAN_Init(hnd, CAN_BAUD_100K, CAN_INIT_TYPE_EX);
			case 250: ret=CAN_Init(hnd, CAN_BAUD_250K, CAN_INIT_TYPE_EX);
			case 125:
			default:  ret=CAN_Init(hnd, CAN_BAUD_125K, CAN_INIT_TYPE_EX);}
		vvvi(4,"CAN init result = ", ret);}
	else {
		vvv(1, "No driver found!");
		exit(0);}
}


int	icanHV_quit(){
    ExitfCAN("",0);
}


//	*****************************************************************
int icanHV_io(u_int32 (*buf)[icanHV_bufsize]){

    u_int32_t status;
	str = new char[255];
	str2 = new char[255];

    ret=LINUX_CAN_Read(hnd, &tmsg);
    if (ret==CAN_ERR_OK) {
      if (firstloop) {
        STime=tmsg.dwTime;
        firstloop=false;
      }
      status = CAN_Status(hnd);
	  str = "CAN_Status:\t";
	  strcat(str, status);
	  strcat(str, '\n');
	  vvv(str);
      if ((int)status < 0) {
         errno = nGetLastError();
         ExitfCAN("CAN_Status()", errno);
      }
      else {
        sprintf(str,"0x%x", tmsg.Msg.ID);
        cout << "ID: " << str << '\t';
        cout << "Length: " << toascii(tmsg.Msg.LEN) << '\t';
        cout << "Data: ";
        for (int i=0; i<=tmsg.Msg.LEN-1; i++) {
           sprintf(str,"%x", tmsg.Msg.DATA[i]);
           if (strlen(str)<2)
              cout << "0x0";
           else
              cout << "0x";
           cout << str << " ";
        }
        cout << '\t';
        cout << (tmsg.dwTime-STime)/1000 << "." << (tmsg.dwTime-STime)%1000 << " s\n";
		
		vvv(6, "verbose test 3\t", __FILE__, __LINE__);
			
		if (tmsg.Msg.DATA[0]==DataID_LogOn) {     //a Log on frame has been received
			vvv(4, "Log On Frame", __FILE__, __LINE__);
			buff[0]=2;
			buff[1]=DataID_LogOn;
			buff[2]=connect;
			Send2CAN(tmsg.Msg.ID&0xffe, buff);     //send a log on message

			buff[0]=1;
			buff[1]=DataID_DID;
			Send2CAN(tmsg.Msg.ID|DATA_DIR, buff);  // send a request for serial number etc.

			buff[0]=1;
			buff[1]=DataID_ActualVoltage|Channel_2;
			Send2CAN(tmsg.Msg.ID|DATA_DIR, buff);  // send a request for the actual voltage of channel 2
			}
		}
	}
}



