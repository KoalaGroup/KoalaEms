/*
 * vertex_read_mem.cc
 * created 27.May.2005 PW
 * changed 04.April.2006 SD
 */

#include "config.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <errorcodes.h>
#include <stdio.h>
#include <errno.h>
#include "client_defaults.hxx"

#include <versions.hxx>

VERSION("2005-May-27", __FILE__, __DATE__, __TIME__,
"$ZEL: iseghv_io.cc,v 1.4 2006/06/06 11:58:52 peterka Exp $")

C_VED* ved;
C_instr_system* is;
C_proclist* proclist;
C_readargs* args;
const char* vedname;

#define DATA_READ 1
#define DATA_WRITE 0

// voltage
#define VOLT_MAX 3000
// current in mA
#define CURR_MAX 3000

int working_dev;
int working_chan;

void show_cmds(void);
void switch_channels(void);
void restart(void);
void restart_hard(void);
void set_working_dev(void);
void set_working_chan(void);
void volt_limit(void);
void get_act_volt(void);
void get_act_curr(void);
void get_ramp(void);
void set_volt(void);
void set_curr(void);
void set_ramp(void);
void get_status(void);
void get_general_status(void);
void get_act_volt_block(void);
void get_act_curr_block(void);
void get_status_block(void);
void get_general_status_block(void);
void read_msg(void);
void write_msg(void);
void rw_msg(void);
void exit_iseghv(void);

int num_cmds = 23;
void (*command_func[])(void) = {show_cmds,
				switch_channels, 
				restart,
				restart_hard,
				set_working_dev,
				set_working_chan,
				volt_limit,
				get_act_volt,
				get_act_curr,
				get_ramp,
				set_volt,
				set_curr,
				set_ramp,
				get_status,
				get_general_status,
				get_act_volt_block,
				get_act_curr_block,
				get_status_block,
				get_general_status_block,
				read_msg,
				write_msg,
				rw_msg,
				exit_iseghv,
				NULL};
char *command_names[] = {"show command list",
			 "switch channels on/off",
			 "restart "
			 "(if software current limit has been exceeded)",
			 "restart "
			 "(if hardware current limit has been exceeded)",
			 "set working device",
			 "set working channel",
			 "voltage limit exceeded?",
			 "get actual voltage",
			 "get actual current",
			 "get ramp",
			 "set voltage",
			 "set current",
			 "set ramp",
			 "get status",
			 "get general status",
			 "get actual voltage of all devices",
			 "get actual current of all devices",
			 "get status of all devices",
			 "get general status of all devices",
			 "read last message",
			 "send message",
			 "send message and wait for answer",
			 "exit", 0};


/*****************************************************************************/
void show_buffer(int size, ems_i32* buffer) {
  while(size--)
    cout << *buffer++ << " ";
  cout << endl;
}
/*****************************************************************************/
int read_int(int min, int max) {
  int number;
  do {
    if(cin.fail()) {
      cerr << endl << "try again> ";
      cin.clear();
      cin.ignore(INT_MAX, '\n');
    }
    cin >> number;
  }while(number<min || number>max);
  return number;
}
/*****************************************************************************/
string read_string(int num, string *choices=NULL) {
  string s;
  int i;
  bool search=true;
  do {
    if(cin.fail()) {
      cerr << endl << "try again> ";
      cin.clear();
      cin.ignore(INT_MAX, '\n');
    }
    cin >> s;
    if(num==0)
      break;
    for(i=0; i<num; ++i) {
      if(s.compare(choices[i])==0)
	search=false;
	break;
    }
    if(search)
      cout << "try again> ";
  }while(search);

  return s;
}

/*****************************************************************************/
char read_char(int num, char *choices=NULL) {
  char c;
  int i;
  bool search=true;
  do {
    if(cin.fail()) {
      cerr << endl << "try again> ";
      cin.clear();
      cin.ignore(INT_MAX, '\n');
    }
    cin >> c;
    if(num==0)
      break;
    for(i=0; i<num; ++i) {      
      if((c == choices[i])==1) {
	search=false;
	break;
      }
    }
    if(search)
      cout << "try again> ";
  }while(search);

  return c;
}
/*****************************************************************************/
int get_device(void) {
  if(working_dev >= 0)
    return working_dev;

  cout << "Type in device id (0 to 63)" << endl;
  return read_int(0, 63);
}
/*****************************************************************************/
int get_channel(int num=1) {
  int chanid;
  
  if(working_chan >= 0)
    return working_chan;

  if(num==1) {
    cout << "Type in channel (0 to 7)" << endl;
    return read_int(0, 7);
  }
  else {
    cout << "Type in channels (0 to 255)" << endl;
    return read_int(0, 255);
  }
}
/*****************************************************************************/
void show_cmds(void) {
  int i = 0;
  while(*(command_names+i) != 0) {
      cout << i << ") " << *(command_names+i) << endl;
      i++;
    }
  return;
}
/*****************************************************************************/
void switch_channels(void) {
  char c;
  int oo = -1; // on/off
  int dev, chan;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;
  
  dev = get_device();
  
  char *choices = new char[2];
  choices[0] = 'y';
  choices[1] = 'n';
  chan = 0;
  for(int i=0; i<8; ++i) {
    cout << "switch channel " << i << " on? [y/n] ";
    c = read_char(2, choices);
    if(c == 'y') {
      chan += (1<<i);
    }
  }

  cout << "setting channels ..." << endl;
  cout << "iseghv_write " << DATA_WRITE << " " << dev << " "
       << 0xcc << " " << 2 << " " << 0 << " " << chan << endl;

  proc_id=is->procnum("iseghv_write");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 5, dev, 0xcc, 2, 0, oo?chan:~chan);
  buffer=(ems_u32*)conf->buffer()+2; // skip error codes
  
  // get channel status

  //iseghv_rw 1 2 0xcc 0

  proc_id=is->procnum("iseghv_rw");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 4, DATA_READ, dev, 0xcc, 0);  
  buffer=(ems_u32*)conf->buffer(); // skip error codes
  if(conf->size() < 8) {
    cout << "could not get channel info: " << buffer[0] << endl;
  }
  else {
    cout << "channel status: " << buffer[6] << " " << buffer[7] << endl;
  }
  
  delete conf;

  return;
}
/*****************************************************************************/
void restart(void) {
  int dev;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;

  dev = get_device();
  
  cout << "set voltage of all channels to 0" << endl;

  proc_id=is->procnum("iseghv_set_volt");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 3, dev, 255, 0);

  proc_id=is->procnum("iseghv_rw");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 4, DATA_READ, dev, 0xf8, 0);  
  buffer=(ems_u32*)conf->buffer(); // skip error codes
  
  cout << "set all channels on" << endl;
  
  proc_id=is->procnum("iseghv_write");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 5, dev, 0xcc, 2, 0, 255);

  proc_id=is->procnum("iseghv_rw");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 4, DATA_READ, dev, 0xcc, 0);  

  buffer=(ems_u32*)conf->buffer(); // skip error codes
  if(conf->size() < 8) {
    cout << "could not get channel info: " << buffer[0] << endl;
  }
  else {
    cout << "channel status: " << buffer[6] << " " << buffer[7] << endl;
  }
  delete conf;
  
  return;
}
/*****************************************************************************/
void set_working_dev(void) {
  cout << "Enter device number (0 to 63) or -1, "
    "if you don't want to use this option" << endl;
  working_dev = read_int(-1, 63);
}
/*****************************************************************************/
void set_working_chan(void) {
  cout << "Enter channel number (0 to 7) or -1, "
    "if you don't want to use this option" << endl;
  working_chan = read_int(-1, 7);
}
/*****************************************************************************/
void restart_hard(void) {
  int dev;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;

  dev = get_device();

  cout << "set voltage of all channels to 0" << endl;

  proc_id=is->procnum("iseghv_set_volt");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 3, dev, 255, 0);

  proc_id=is->procnum("iseghv_rw");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 4, DATA_READ, dev, 0xc8, 0);  
  buffer=(ems_u32*)conf->buffer(); // skip error codes
  
  cout << "set all channels on" << endl;
  
  proc_id=is->procnum("iseghv_write");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 5, dev, 0xcc, 2, 0, 255);

  proc_id=is->procnum("iseghv_rw");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 4, DATA_READ, dev, 0xcc, 0);  

  buffer=(ems_u32*)conf->buffer(); // skip error codes
  if(conf->size() < 8) {
    cout << "could not get channel info: " << buffer[0] << endl;
  }
  else {
    cout << "channel status: " << buffer[6] << " " << buffer[7] << endl;
  }
  delete conf;
  
  return;
}
/*****************************************************************************/
void volt_limit(void) {
  int dev;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;

  dev = get_device();

  proc_id=is->procnum("iseghv_rw");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 4, DATA_READ, dev, 0xc4, 0);  
  show_buffer(conf->size(), conf->buffer());
  buffer=(ems_u32*)conf->buffer(); // skip error codes
  if(conf->size() < 8) {
    cout << "could not get voltage limit: " << buffer[0] << endl;
  }
  else {
    cout << "voltage limit flag: " << buffer[6] << " " << buffer[7] << endl;
  }
  delete conf;

  
  return;
}
/*****************************************************************************/
void get_act_volt(void) {
  int dev, chan;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;

  dev = get_device();
  chan = get_channel(1);

  proc_id=is->procnum("iseghv_get_act_volt");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 2, dev, chan); 
  show_buffer(conf->size(), conf->buffer());
  buffer=(ems_u32*)conf->buffer()+2; // skip error codes
  if(conf->size() < 9) {
    cout << "could not get voltage: " << buffer[0] << endl;
  }
  else {
    cout << "actual voltage: " << (ems_i32)(buffer[buffer[2]+3]) << " [V] "
	 << endl;
  }
  delete conf;
  
  return;
}
/*****************************************************************************/
void get_act_curr(void) {
  int dev, chan;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;

  dev = get_device();
  chan = get_channel(1);

  proc_id=is->procnum("iseghv_get_act_curr");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 2, dev, chan); 
  show_buffer(conf->size(), conf->buffer());
  buffer=(ems_u32*)conf->buffer()+2; // skip error codes
  if(conf->size() < 9) {
    cout << "could not get current: " << buffer[0] << endl;
  }
  else {
    cout << "actual current: " << (ems_i32)(buffer[buffer[2]+3]) << " [uA]" 
	 << endl;
  }
  delete conf;
  
  return;
}
/*****************************************************************************/
void get_ramp(void) {
  int dev, chan;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;

  dev = get_device();

  proc_id=is->procnum("iseghv_get_ramp");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 1, dev); 
  show_buffer(conf->size(), conf->buffer());
  buffer=(ems_u32*)conf->buffer()+2; // skip error codes
  if(conf->size() < 9) {
    cout << "could not get ramp: " << buffer[0] << endl;
  }
  else {
    cout << "ramp: " << (ems_i32)(buffer[buffer[2]+3]/10) << "[V/s]" << endl;
  }
  delete conf;
  
  return;
}
/*****************************************************************************/
void set_volt(void) {
  int dev, chan, volt;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;

  dev = get_device();
  chan = get_channel(1);
  
  cout << "Type new voltage (0 to " << VOLT_MAX << " [V]): "; 
  volt = read_int(0, VOLT_MAX);

  proc_id=is->procnum("iseghv_set_volt");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 3, dev, chan, volt);
  delete conf;
  
  return ;
}
/*****************************************************************************/
void set_curr(void) {
  int dev, chan, curr;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;

  dev = get_device();
  chan = get_channel(1);
  
  cout << "Type new current (0 to " << CURR_MAX << " [uA]): "; 
  curr = read_int(0, CURR_MAX);

  proc_id=is->procnum("iseghv_set_curr");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 3, dev, chan, curr);
  delete conf;
  
  return ;
}
/*****************************************************************************/
void set_ramp(void) {
   int dev, ramp;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;

  dev = get_device();
  
  cout << "Type new ramp (12 to 3000 [1/10 V/s]) " << endl; 
  ramp = read_int(12, 3000);

  proc_id=is->procnum("iseghv_set_ramp");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 2, dev, ramp);
  
  delete conf;
  return ;
}
/*****************************************************************************/
void get_status(void) {
  int dev, chan;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;

  dev = get_device();
  chan = get_channel();

  proc_id=is->procnum("iseghv_get_status");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 2, dev, chan); 
  show_buffer(conf->size(), conf->buffer());
  buffer=(ems_u32*)conf->buffer()+2; // skip error codes
  if(conf->size() < 8) {
    cout << "could not get status: " << buffer[0] << endl;
  }
  else {
    cout << "status:              " 
	 << buffer[4] << " " << buffer[5] << endl
	 << "current trip:        " 
	 << (buffer[5]&1?"shut of":"channel is ok") << endl 
	 << "sum eror:            " 
	 << (buffer[5]&2?"found":"channel is ok") << endl 
	 << "input error:         "
	 << (buffer[4]&2?"wrong message send":"no") << endl
	 << "switch channel to:   " 
	 << (buffer[4]&4?"on":"off") << endl
	 << "ramping:             "
	 << (buffer[4]&8?"voltage ramps":"voltage is stable") << endl
	 << "emergency cut off:   "
	 << (buffer[4]&16?"cut off voltage":"channel works") << endl
	 << "kill function:       "
	 << (buffer[4]&32?"enabled":"disabled") << endl
	 << "current limit error: "
	 << (buffer[4]&64?"shut off":"channel is ok") << endl
	 << "voltage limit error: "
	 << (buffer[4]&128?"shut off":"channel is ok") << endl;
  }
  delete conf;
  
  return;
}
/*****************************************************************************/
void get_general_status(void) {
   int dev, chan;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;

  dev = get_device();

  proc_id=is->procnum("iseghv_get_general_status");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 1, dev); 
  show_buffer(conf->size(), conf->buffer());
  buffer=(ems_u32*)conf->buffer()+2; // skip error codes
  if(conf->size() < 7) {
    cout << "could not get general status: " << buffer[0] << endl;
  }
  else {
    cout << "general status:      " 
	 << buffer[4] << endl
	 << "sum error:           "
	 << (buffer[4]&1?"no errors":"error @ v/c limit or trip") << endl
	 << "ramping:             "
	 << (buffer[4]&2?"no channel is ramping":
	     "at least one channel is ramping") << endl
	 << "safety sloop:        "
	 << (buffer[4]&4?"closed":"broken") << endl
	 << "stable:              "
	 << (buffer[4]&8?"all channels are stable":
	     "at least one channel is ramping") << endl
	 << "fine adjustment:     "
	 << (buffer[4]&16?"on":"off") << endl
	 << "supply voltages:     "
	 << (buffer[4]&32?"in range":"out of range") << endl
	 << "kill function:       "
	 << (buffer[4]&32?"enabled":"disabled") << endl
	 << "save set values:     "
	 << (buffer[4]&64?"store all set values to EEPROM":
	     "no write access to EEPROM") << endl;
  }
  delete conf;
  
  return;
}
/*****************************************************************************/
void read_msg(void) {
  int dev, chan;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;
  
  try {
    proc_id=is->procnum("iseghv_read");
    is->execution_mode(immediate);
    conf=is->command(proc_id, 0); 
    show_buffer(conf->size(), conf->buffer());
    
    buffer=(ems_u32*)conf->buffer()+2; // skip error codes
    if(conf->size() < 3) {
      cout << "could not read: " << buffer[0] << endl;
    }
    else {
      cout << "last message: " << endl
	   << "ID: " << (ems_i32)(buffer[0]) << endl
	   << "MSGTYPE: " << (ems_i32)(buffer[1]) << endl
	   << "LEN: " << (ems_i32)(buffer[2]) << endl
	   << "DATA: ";
      for(int i=1; i<=(ems_i32)(buffer[2]); ++i) {
	cout << (ems_i32)(buffer[2+i]) << " ";
      }
      cout << endl;
    }
    delete conf;
  }
  catch (C_error* e) {
    cout << "could not read" << endl;
  }

  return;
}
/*****************************************************************************/
void get_act_volt_block(void) {
  int dev;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;
  int curr, cchan;

  dev = -1;

  proc_id=is->procnum("iseghv_get_act_volt_block");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 1, dev); 
  show_buffer(conf->size(), conf->buffer());
  buffer=(ems_u32*)conf->buffer()+1; // skip error codes
  cout << "got " << buffer[0] << " devices" << endl;
  for(curr=0; curr<buffer[0]; ++curr) {
    cout << "device #" << buffer[curr*9+1] << ": ";
    for(cchan=1; cchan<=8; ++cchan) {
      cout << buffer[curr*9+1+cchan] << " ";
    }
    cout << "\n";
  }
  delete conf;
  
  return;
}
/*****************************************************************************/
void get_act_curr_block(void) {
  int dev;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;
  int curr, cchan;

  dev = -1;

  proc_id=is->procnum("iseghv_get_act_curr_block");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 1, dev); 
  show_buffer(conf->size(), conf->buffer());
  buffer=(ems_u32*)conf->buffer()+1; // skip error codes
  cout << "got " << buffer[0] << " devices" << endl;
  for(curr=0; curr<buffer[0]; ++curr) {
    cout << "device #" << buffer[curr*9+1] << ": ";
    for(cchan=1; cchan<=8; ++cchan) {
      cout << buffer[curr*9+1+cchan] << " ";
    }
    cout << "\n";
  }
  delete conf;
  
  return;
}
/*****************************************************************************/
void get_status_block(void) {
  int dev;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;
  int curr, cchan, b0, b1;
  char c;
  char *choices = new char[2];
  choices[0] = 'y';
  choices[1] = 'n';

  dev = -1;
  cout << "detailed information? [y/n] ";
  c = read_char(2, choices);

  proc_id=is->procnum("iseghv_get_status_block");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 1, dev); 
  show_buffer(conf->size(), conf->buffer());
  buffer=(ems_u32*)conf->buffer()+1; // skip error codes
  cout << "got " << buffer[0] << " devices" << endl;
  for(curr=0; curr<buffer[0]; ++curr) {
    cout << "device #" << buffer[curr*15+1] << "\n";
    for(cchan=1; cchan<=8; ++cchan) {
      b0 = curr*15+1+cchan;
      b1 = b0+1;
      cout << "status @channel " << cchan-1 << " "
	   << buffer[b0] << " " << buffer[b1] << endl;
      if(c == 'y') {
	cout << "current trip:        " 
	     << (buffer[b1]&1?"shut of":"channel is ok") << endl 
	     << "sum eror:            " 
	     << (buffer[b1]&2?"found":"channel is ok") << endl 
	     << "input error:         "
	     << (buffer[b0]&2?"wrong message send":"no") << endl
	     << "switch channel to:   " 
	     << (buffer[b0]&4?"on":"off") << endl
	     << "ramping:             "
	     << (buffer[b0]&8?"voltage ramps":"voltage is stable") << endl
	     << "emergency cut off:   "
	     << (buffer[b0]&16?"cut off voltage":"channel works") << endl
	     << "kill function:       "
	     << (buffer[b0]&32?"enabled":"disabled") << endl
	     << "current limit error: "
	     << (buffer[b0]&64?"shut off":"channel is ok") << endl
	     << "voltage limit error: "
	     << (buffer[b0]&128?"shut off":"channel is ok") << endl;
      }
    }
  }

  delete conf;
  
  return;
}
/*****************************************************************************/
void get_general_status_block(void) {
  int dev;
  C_confirmation* conf;
  int proc_id;
  ems_u32* buffer;
  int curr, cchan, b0, b1;
  char c;
  char *choices = new char[2];
  choices[0] = 'y';
  choices[1] = 'n';

  dev = -1;
  cout << "detailed information? [y/n] ";
  c = read_char(2, choices);

  proc_id=is->procnum("iseghv_get_general_status_block");
  is->execution_mode(immediate);
  conf=is->command(proc_id, 1, dev); 
  show_buffer(conf->size(), conf->buffer());
  buffer=(ems_u32*)conf->buffer()+1; // skip error codes
  cout << "got " << buffer[0] << " devices" << endl;
  for(curr=0; curr<buffer[0]; ++curr) {
    cout << "device #" << buffer[curr*2] << "\n";
    cout << "general status:      " 
	 << buffer[curr*2+1] << endl;      
    if(c == 'y') {
      cout << "sum error:           "
	   << (buffer[curr*2+1]&1?
	       "no errors":"error @ v/c limit or trip") << endl
	   << "ramping:             "
	   << (buffer[curr*2+1]&2?"no channel is ramping":
	       "at least one channel is ramping") << endl
	   << "safety sloop:        "
	   << (buffer[curr*2+1]&4?"closed":"broken") << endl
	   << "stable:              "
	   << (buffer[curr*2+1]&8?"all channels are stable":
	       "at least one channel is ramping") << endl
	   << "fine adjustment:     "
	   << (buffer[curr*2+1]&16?"on":"off") << endl
	   << "supply voltages:     "
	   << (buffer[curr*2+1]&32?"in range":"out of range") << endl
	   << "kill function:       "
	   << (buffer[curr*2+1]&32?"enabled":"disabled") << endl
	   << "save set values:     "
	   << (buffer[curr*2+1]&64?"store all set values to EEPROM":
	       "no write access to EEPROM") << endl;
    }
  }
  
  delete conf;
  
  return;
}
/*****************************************************************************/
void write_msg(void) {
  C_confirmation* conf;
  int proc_id, len;
  ems_i32 dev, data_id; 
  ems_u32 data[7];

  dev = get_device();
  cout << "enter command (data_id) (0-255): ";
  data_id = read_int(0, 255);
  cout << "enter number of following data bytes (0-7): ";
  len = read_int(0, 7);
  for(int i=0; i<len; i++) {
    cout << "enter byte " << i << ": ";
    data[i] = read_int(0, 255);
  }
  cout << "please wait" << endl;

  proc_id = is->procnum("iseghv_write");
  is->execution_mode(delayed);
  is->command(proc_id);
  is->add_param(3, dev, data_id, len);
  is->add_param(len, data);
  conf=is->execute();
  delete conf;
  cout << "wrote data" << endl;
}
/*****************************************************************************/
void rw_msg(void) {
  C_confirmation* conf;
  int proc_id, len;
  ems_i32 data_dir, dev, data_id;
  ems_u32* buffer, data[7];

  cout << "enter data_dir bit (0:write or 1:read): ";
  data_dir = read_int(0, 1);
  dev = get_device();
  cout << "enter command (data_id) (0-255): ";
  data_id = read_int(0, 255);
  cout << "enter number of following data bytes (0-7): ";
  len = read_int(0, 7);
  for(int i=0; i<len; i++) {
    data[i] = read_int(0, 255);
  }
  cout << "please wait" << endl;

  proc_id = is->procnum("iseghv_rw");
  is->execution_mode(delayed);
  is->command(proc_id);
  is->add_param(4, data_dir, dev, data_id, len);
  is->add_param(len, data);
  conf=is->execute();

  show_buffer(conf->size(), conf->buffer());

  buffer=(ems_u32*)conf->buffer()+2; // skip error codes
  if(conf->size() < 9) {
    cout << "no answer received: " << buffer[0] << endl;
  }
  else {
    cout << "got message: " << endl
	 << "ID: " << (ems_i32)(buffer[0]) << endl
	 << "MSGTYPE: " << (ems_i32)(buffer[1]) << endl
	 << "LEN: " << (ems_i32)(buffer[2]) << endl
	 << "DATA: ";
    for(int i=1; i<=(ems_i32)(buffer[2]); ++i) {
      cout << (ems_i32)(buffer[2+i]) << " ";
    }
    cout << endl;
  }

  delete conf;
  cout << "wrote data" << endl;
}
/*****************************************************************************/
void exit_iseghv(void) {
  string input;
  cout << "Exit? (yes/no)" << endl;
  cin.clear();
  cin.ignore(INT_MAX, '\n');
  cin >> input;
  if(input.compare("yes") == 0) {
    exit(0);
  }
  else {
    cout << "type 'yes' to exit!" << endl;
  }
  return;
}
/*****************************************************************************/
static int
readargs(void)
{
    int res;
    args->addoption("hostname", "h", "localhost",
        "host running commu", "hostname");
    args->use_env("hostname", "EMSHOST");
    args->addoption("port", "p", DEFISOCK,
        "port for connection with commu", "port");
    args->use_env("port", "EMSPORT");
    args->addoption("sockname", "s", DEFUSOCK,
        "socket for connection with commu", "socket");
    args->use_env("sockname", "EMSSOCKET");
    /*args->addoption("crate", "crate", 0, "crate index", "crate");
    args->addoption("module", "module", 0, "module address", "module");
    args->addoption("addr", "addr", 0, "start address", "addr");
    args->addoption("size", "size", 0, "size in bytes", "size");
    args->addoption("memtype", "type", 0, "type of memory (0: seq, 1: table)",
    "memorytype");*/
    args->addoption("vedname", 0, "", "Name des VED", "vedname");
    //args->addoption("filename", 1, "", "filename", "filename");

    args->hints(C_readargs::required, "vedname", 0);
    args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
    args->hints(C_readargs::exclusive, "port", "sockname", 0);

    if ((res=args->interpret(1))!=0) return res;

    /*crate=args->getintopt("crate");
    module=args->getintopt("module");
    addr=args->getintopt("addr");
    memtype=args->getintopt("memtype");
    maxmem=memtype?maxmem_tab:maxmem_seq;
    if (args->isdefault("size")) {
        size=maxmem;
    } else {
        size=args->getintopt("size");
        if (size>maxmem)
            size=maxmem;
    }*/
    vedname=args->getstringopt("vedname");
    //filename=args->getstringopt("filename");

    return 0;
}
/*****************************************************************************/
static int
open_ved(void)
{
    try {
        if (!args->isdefault("hostname") || !args->isdefault("port")) {
            communication.init(args->getstringopt("hostname"),
                    args->getintopt("port"));
        } else {
            communication.init(args->getstringopt("sockname"));
        }
    } catch (C_error* e) {
        cerr << (*e) << endl;
        delete e;
        return -1;
    }

    try {
        ved=new C_VED(vedname);
        is=ved->is0();
    } catch (C_error* e) {
        cerr << (*e) << endl;
        delete e;
        communication.done();
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
read_mem(ems_u32* data, int size)
{
    C_confirmation* conf;
    int proc_id=is->procnum("iseghv_found_devices");
    is->execution_mode(immediate);
    // conf=is->command(proc_id, 5, crate, module, memtype, addr, size);
    conf=is->command(proc_id, 0);
    //is->command throws exception in case of error
    /*if ((conf->size()-1)!=size/4) {
        cerr<<"got "<<conf->size()-1<<" words but expected "<<size/4<<endl;
        delete conf;
        return -1;
	}*/
    ems_u32* buffer=(ems_u32*)conf->buffer()+1; // skip error code
    for (int i=conf->size(); i>0; i--) {
      cout << *buffer << endl;
      *data++=*buffer++;
    }

    delete conf;
    return 0;
}
/*****************************************************************************/
static int
iseghv_init(int cc)
{
    C_confirmation* conf;
    int proc_id;

    // init
    proc_id=is->procnum("iseghv_init");
    //is->execution_mode(immediate);
    cout << "initialising ... this takes several seconds" << endl;
    //conf=is->command(proc_id, 1, cc);

    is->execution_mode(delayed);
    is->command(proc_id);
    is->add_param(1, cc);
    conf=is->execute();

    cout << "done" << endl;


    // print found devices
    proc_id=is->procnum("iseghv_found_devices");
    is->execution_mode(immediate);
    conf=is->command(proc_id, 0);
    ems_u32* buffer=(ems_u32*)conf->buffer()+2; // skip error codes
    cout << "found " << *buffer << " devices: ";
    for (int i=(*buffer++); i>0; i--) {
      cout << *buffer << " ";
    }
    cout << endl;

    delete conf;
    return 0;
} 
/*****************************************************************************/
static int
user_interface(void)
{
  int command;
  
  show_cmds();
  do {
    cout << "Choose a command> ";
    
    command = -1;
    if(cin.fail()) {
      cin.clear();
      cin.ignore(INT_MAX, '\n');
    }
    cin >> command;

    if(command >= 0 && command < num_cmds) {
      try {
	command_func[command]();
      } catch (C_error* e) {
	cerr << (*e) << endl;
	delete e;
      }
    }
    else {
      cout << "Command not found!" << endl;
    }
  } while(1);

  return 0;
}
/*****************************************************************************/
main(int argc, char* argv[])
{
  int cc;
  try {
    args=new C_readargs(argc, argv);
    if (readargs()!=0)
      return(1);
    
    proclist=0;
    if (open_ved()<0)
      return 2;
    
    cout << "Select can card!" << endl;
    cout << "0) peakpci" << endl;
    cout << "1) esd can331" << endl;
    cc =read_int(0, 1);
    
    if (iseghv_init(cc)<0)
      return 6;
    working_dev = -1;
    working_chan = -1;
  } catch (C_error* e) {
    cerr << (*e) << endl;
    delete e;
    return 5;
  }
  user_interface();
  
  delete ved;
  communication.done();
  return 0;
}
/*****************************************************************************/
