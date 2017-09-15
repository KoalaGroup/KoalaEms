/*
 * show_scaler.cc
 * 
 * created: 2006-Aug-20 PW
 * 
 * modified from read_scaler, 2007-04-09 VH 
 */

#include "config.h"
#include <iostream>
#include <cmath>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <errorcodes.h>
#include <ncurses.h> // ncurses defines OK as 0 which collides with errorcodes

#include <versions.hxx>
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <conststrings.h>
#include <xdrstring.h>
#include <modultypes.h>
#include <sockets.hxx>

VERSION("2007-Apr-11", __FILE__, __DATE__, __TIME__,
"$ZEL: show_scaler.cc,v 1.3 2007/04/11 13:34:08 wuestner Exp $")
#define XVERSION

#define DEFUSOCK "/var/tmp/emscomm"
#define DEFISOCK 4096

C_readargs* args;
C_VED* ved;
C_instr_system *is;
int ems_var;
int latch_mask, p;
int last_day;

ems_u64 timestamp; /* usec */
ems_u64 lasttimestamp; /* usec */
ems_u32 nr_modules;
ems_u32 nr_channels[32]; /* no more than 32 modules...*/
ems_u64 val[32][32]; /* val[module][channel] */
ems_u64 lastval[32][32]; /* val[module][channel] */

ems_u32 nr_bct;
ems_u32 *val_bct;

tcp_socket* bct;
bool showbct;

int statusline;

pthread_mutex_t mutx = PTHREAD_MUTEX_INITIALIZER;

/*****************************************************************************/
static int
readargs(void)
{
    args->addoption("hostname", "h", "localhost",
        "host running commu", "hostname");
    args->use_env("hostname", "EMSHOST");
    args->addoption("port", "p", DEFISOCK,
        "port for connection with commu", "port");
    args->use_env("port", "EMSPORT");
    args->addoption("sockname", "s", DEFUSOCK,
        "socket for connection with commu", "socket");
    args->use_env("sockname", "EMSSOCKET");
    args->addoption("vedname", "ved", "scaler", "name of the VED", "vedname");
    args->addoption("is", "is", 1, "Instrumentation system", "is");
    args->addoption("ems_var", "var", 1, "ems variable", "var");
    args->addoption("latch_mask", "latchmask", 0x0f, "latch mask for sis3100", "lmask");
    args->addoption("interval", "interval", 5.0, "Interval", "interval");
    args->addoption("bcthost","bcthost","ikpwt03","server for COSY info","bcthost");
    args->addoption("bctport","bctport",22222,"port for COSY info","bctport");
    args->addoption("showbct","showbct",false,"show COSY info","showbct");
    //args->hints(C_readargs::required, "vedname", 0);
    args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
    args->hints(C_readargs::exclusive, "port", "sockname", 0);
    return args->interpret();
}
/*****************************************************************************/
static void
start_communication(void)
{
    if (!args->isdefault("hostname") || !args->isdefault("port")) {
        communication.init(args->getstringopt("hostname"),
                args->getintopt("port"));
    } else {
        communication.init(args->getstringopt("sockname"));
    }
}
/*****************************************************************************/
static void
start_bct(void)
{
    showbct = args->getboolopt("showbct");
    if (showbct) {
      bct = new tcp_socket("data");
      try {
        bct->create();
        bct->connect(args->getstringopt("bcthost"), args->getintopt("bctport"));
      } catch (C_error *e) {
	cerr << (*e) << endl;
        delete bct;
	exit(1);
      }
    }
}
/*****************************************************************************/
static void
open_ved(void)
{
    ved=new C_VED(args->getstringopt("vedname"));
}
/*****************************************************************************/
static void
open_is(void)
{
    is=new C_instr_system(ved, args->getintopt("is"), C_instr_system::open);
    is->execution_mode(delayed);
}
/*****************************************************************************/
static int
read_scaler(void)
{
    C_confirmation* conf;
    ems_u32* buf;

    is->clear_command();
    is->command("Timestamp");
    is->add_param(1, 1);
    is->command("sis3800ShadowUpdateAll");
    is->add_param(2, ems_var, latch_mask);
    conf=is->execute();
    buf=reinterpret_cast<ems_u32*>(conf->buffer());
    #if 0
    for (int i=0; i<conf->size(); i++) {
        printf("%d ", buf[i]);
    }
    printf("\n");
    #endif

    if (buf[0]) {
        cerr<<"error in EMS procedure: "<<buf[0]<<endl;
        delete conf;
        return -1;
    }
    buf++;

    timestamp=*buf++;   /* sec */
    timestamp*=1000000; /* converted to usec */
    timestamp+=*buf++;  /* usec */
    nr_modules=*buf++;
    for (unsigned int mod=0; mod<nr_modules; mod++) {
        nr_channels[mod]=*buf++;
        for (unsigned int chan=0; chan<nr_channels[mod]; chan++) {
            ems_u64 v;
            v=*buf++;
            v<<=32;
            v+=*buf++;
            val[mod][chan]=v;
        }
    }
    delete conf;
    return 0;
}
/*****************************************************************************/
static int
read_bct(void)
{
    ems_u32 num;

    int res = read(*bct,&num,sizeof(num));
    if (res<0) {
        if (errno!=EINTR) {
            mvprintw(statusline,0,"Error reading COSY info");
            clrtoeol();
            refresh();
            bct->close();
            showbct = false;
            return -1;
        }
    }

    num = ntohl(num);
    if (num!=11) {
        mvprintw(statusline,0,"Strange length of COSY info: %d",num);
        bct->close();
        showbct = false;
        return -1;
    }

    if (nr_bct != num) {
        if (nr_bct) {
            delete [] val_bct;
	}
	nr_bct = num;
	val_bct = new ems_u32[nr_bct];
    }

    int length = num*sizeof(ems_u32);
    char* buf  = reinterpret_cast<char*>(val_bct);
    while (length) {
        res=read(*bct, buf, length);
        if (res<0) {
            if (errno!=EINTR) {
                mvprintw(statusline,0,"Error reading COSY info");
                clrtoeol();
                refresh();
                bct->close();
                showbct = false;
                return -1;
            }
        } else {
            length -= res;
            if (res==0) break;
        }
    }
    
    if (length != 0) {
        mvprintw(statusline,0,"Error reading COSY info");
        clrtoeol();
        refresh();
        bct->close();
        showbct = false;
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static void
plot_scaler(bool rate=true)
{
    time_t stamp = timestamp/1000000;
    double dt  = (timestamp - lasttimestamp)/1000000.;
    lasttimestamp = timestamp;
    unsigned int tis = val[4][7] & 0xffffffff;
    move(0,0);
    clrtoeol();
    mvprintw(0,0,"Scaler time: %s",ctime(&stamp));
    mvprintw(0,38,"(localtime)");
    mvprintw(0,57,"Time in cycle: %3d sec",tis/100000);
    for(int i=0; i<5; i++) {
      mvprintw(1,i*16+7,"Module %d",i);
      for (unsigned int j=0; j<nr_channels[i]; j++) {
        mvprintw(j+2,i*16,"%3d:  ",i*32 + j); 
        if (rate) 
             mvprintw(j+2,i*16+4,"%11.1f ", (val[i][j]-lastval[i][j])/dt);
        else 
             mvprintw(j+2,i*16+4,"%11u ", val[i][j]);
        lastval[i][j] = val[i][j];
      }
    }
    refresh();
}
/*****************************************************************************/
static void
plot_bct(void)
{
    if (nr_bct<2) {
      mvprintw(statusline,0,"Waiting for COSY data ...");
      clrtoeol();
      refresh();
      return;
    }
    time_t stamp = ntohl(val_bct[0]);
    move(35,0);
    clrtoeol();
    mvprintw(35,0,"COSY time: %s",ctime(&stamp));
    mvprintw(35,36,"(localtime)");
    if (nr_bct<11) return;
    for(unsigned int j=0; j<2; j++) {
      for (unsigned int i=0; i<4; i++) {
        float val=static_cast<int32_t>(ntohl(val_bct[i*2 + j + 3]))/27648.*10.;
	mvprintw(j+36,i*16,"%3d:  ",i*2 + j);
	mvprintw(j+36,i*16+4,"%11.5f ", val);
      }
    }
    refresh();
}
/*****************************************************************************/
char
wait_interval(int tsec)
{
    /* nsec ist ignored */
    struct timeval now;
    ems_u64 usec_now, usec_next, usec=tsec*100000;
    int  tsectowait;
    char retval;
    int  again;

    gettimeofday(&now, 0);
    usec_now=static_cast<ems_u64>(now.tv_sec)*1000000
            +static_cast<ems_u64>(now.tv_usec);
    usec_next=usec_now+usec;

    halfdelay(10);
    do {
        again=0;
        retval = getch();
        if (retval>=0) return retval;
        gettimeofday(&now, 0);
        usec_now=static_cast<ems_u64>(now.tv_sec*1000000)
                +static_cast<ems_u64>(now.tv_usec);
        tsectowait=(usec_next-usec_now)/100000;
        if (tsectowait>0) {
          again = 1;
          halfdelay(tsectowait);
        }
    } while (again);
    return -1;
}
/*****************************************************************************/
char
show_help()
{
    int l=2;
    clear();
    mvprintw(l++,2,"Help:");
    mvprintw(l++,2,"-----");
    l++;
    mvprintw(l++,2,"h - show this help");
    mvprintw(l++,2,"s - show sums from scaler");
    mvprintw(l++,2,"r - show rate");
    mvprintw(l++,2,"p - print screen to file 'scaler.txt'");
    mvprintw(l++,2,"q - quit program");
    l++;
    mvprintw(l++,2,"Important channels:");
    mvprintw(l++,2,"-------------------");
    mvprintw(l++,2,"Scaler ch 64: pellet rate:");
    mvprintw(l++,2,"Scaler ch 65: accepted trigger rate");
    mvprintw(l++,2,"Scaler ch 66: available trigger rate");
    mvprintw(l++,2,"Cosy   ch  0: BCT signal");   
    mvprintw(l++,2,"Cosy   ch  3: Pressure in vacuum chamber");
  
    mvprintw(statusline,0,"Press any key to return to scaler display");
  
    char answer = wait_interval(100);
    clear();
    return answer;
}
/*****************************************************************************/
void
dump_screen(const char* filename)
{
  char line[100];

  FILE *out = fopen(filename,"w");
  if (!out) {
    mvprintw(statusline,0,"Screen dump failed.");
    clrtoeol();
    refresh();
    return;
  }

  for(unsigned int i=0; i<40;i++) {
    move(i,0);
    innstr(line,100);
    fprintf(out,"%s\n",line);
  }

  fclose(out);
  mvprintw(statusline,0,"Screen dumped to file %s",filename);
  clrtoeol();
  refresh();
  return;
}
/*****************************************************************************/
void*
bct_child(void *arg)
{
    int retval;

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    do
    {
      pthread_mutex_lock(&mutx);
      retval = read_bct();
      pthread_mutex_unlock(&mutx);
      if (retval<0) break;
    } while (1);

    return 0;
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    float interval;
    int tsec;
    int error_count = 0;

    int row,col;

    char answer;
    bool rate = true;

    nr_bct = 0;
    val_bct = 0;
    pthread_t pchild;

    args=new C_readargs(argc, argv);
    if (readargs())
        return 1;

    statusline = 34;
    start_bct();
    if (showbct) {
      if (pthread_create(&pchild, 0, bct_child, 0) != 0) {
	perror("Creating bct thread");
	exit(-1);
      }
      statusline += 4;
    }

    interval=args->getrealopt("interval");
    tsec=static_cast<int>(interval*10.);

    ems_var=args->getintopt("ems_var");
    latch_mask=args->getintopt("latch_mask");

    initscr();
    
    getmaxyx(stdscr,row,col);
    if (row<(statusline+2) || col<80) {
      cerr << "Minimum terminal size required: " << statusline+2 << "x80" << endl;
      return 1;
    }

    cbreak();
    halfdelay(1);
    noecho();
    p=-1;

    do {
      try {
        for(int i=0; i<32; i++) {
          for(int j=0; j<32; j++) {
            lastval[i][j] = 0;
          }
        }

        start_communication();
        open_ved();
        open_is();
        bool abort_loop = false;
        mvprintw(statusline,0,"Press h for help");
        clrtoeol();
        refresh();
        while (1) {
          if (read_scaler()==0) {
            plot_scaler(rate); 
          } else {
            abort_loop = true;
            mvprintw(statusline,0,"Error in read, aborting loop.");
            clrtoeol();
            refresh();
          }
          if (showbct) plot_bct();

          answer = wait_interval(tsec);
          mvprintw(statusline,0,"Press h for help");
          clrtoeol();
          refresh();
          if (answer<0) continue;
          switch (answer) {
           case 'q':
            endwin();
            if (showbct) {
              pthread_cancel (pchild);
              pthread_join(pchild, 0);
            }
            return 0;
           case 's':
            mvprintw(statusline,0,"Display sums");
            clrtoeol();
            refresh();
            rate = false;
            break;
           case 'r':
            mvprintw(statusline,0,"Display rate");
            clrtoeol();
            refresh();
            rate = true;
            break;
           case 'h':
            show_help();
            break;
           case 'p':
            dump_screen("scaler.txt");
            break;
           case ' ':
            break;
           default:
            mvprintw(statusline,0,"Unknown command %c",answer);
            clrtoeol();
            refresh();
          }
          if (abort_loop) break;
        } 
      } catch (C_error *e) {
        struct timespec wait;
        mvprintw(statusline,0,e->message().c_str());
        clrtoeol();
        mvprintw(statusline+1,0,"Restarted: %d times",++error_count);
        clrtoeol();
        refresh();
        delete e;
        communication.done();
        wait.tv_sec = 10;
        wait.tv_nsec = 0;
        nanosleep(&wait, 0);
      }
    } while (1);

    endwin();
    if (showbct) {
      pthread_cancel (pchild);
      pthread_join(pchild, 0);
    }
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
