static const char* cvsid __attribute__((unused))=
    "$ZEL: wattcp.c,v 1.2 2011/04/06 20:30:21 wuestner Exp $";

#include <stdio.h>
#include <tcp.h>

#include <sconf.h>
#include <errorcodes.h>
#include <swap.h>
#include <debug.h>
#include <rcs_ids.h>

static tcp_Socket s;
static int p,conn;

RCS_REGISTER(cvsid, "")

errcode _init_comm(port)
char *port;
{
  T(_init_comm)
  p=DEFPORT;
  if(port){
    if(sscanf(port, "%d", &p)!=1){
      printf("kann Portnummer %s nicht deuten\n",port);
      return(Err_ArgRange);
    }
  }
  D(D_COMM, printf("using socket on port %d\n", p);)
  if(!_sock_init()){
    printf("kann TCP nicht initialisieren\n");
    return(Err_System);
  }
  tcp_listen(&s,(word)p,0,0,NULL,0);
  conn=0;
  return(OK);
}

errcode _done_comm(){
  sock_exit();
}

int open_connection(){
  word status;

  T(open_connection)
  if(conn)printf("Fehler: open while connected\n");
  sock_wait_established(&s,0,NULL,&status);
  conn=1;
  return(1);
sock_err:
  printf("Fehler in open_connection\n");
  return(-1);
}

int poll_connection(){
  word status;

  T(poll_connection)
  if(conn)printf("Fehler: poll while connected\n");
  sock_tick(&s,&status);
  if(!tcp_established(&s))return(-1);
  conn=1;
  return(1);
sock_err:
  printf("Fehler in poll_connection\n");
  return(-1);
}

void close_connection(){
  word status;

  if(!conn)printf("Fehler: close while not connected\n");
  sock_close(&s);
/*  sock_wait_closed(&s,0,NULL,&status);*/
  tcp_listen(&s,(word)p,0,0,NULL,0);
  conn=0;
  return;
sock_err:
  printf("Fehler in close_connection\n");
  exit(0); 
}

int recv_data(buf,len)
int *buf,len;
{
  int da;

  T(recv_data)
  D(D_COMM,printf("lese %d Worte\n",len);)
  if(!conn)printf("Fehler: recv while not connected\n");
  da=sock_read(&s,buf,len*4);
  if(da!=len*4){
    printf("Lesefehler\n");
    exit(0);
  }
  swap_falls_noetig(buf,len);
  D(D_COMM,printf("gelesen\n");)
  return(0);
}

int poll_data(buf,len)
int *buf,len;
{
  int da;
  word status;

  T(poll_data)
  D(D_COMM,printf("lese %d Worte\n",len);)
  if(!conn)printf("Fehler: poll while not connected\n");
  sock_tick(&s,&status);
  if(!sock_dataready(&s))return(0);
  da=sock_read(&s,buf,len*4);
  if(da!=len*4)printf("Lesefehler\n");
  swap_falls_noetig(buf,len);
  D(D_COMM,printf("gelesen\n");)
  return(1);
sock_err:
  printf("Fehler in poll_data\n");
  return(-1);
}

int send_data(buf,len)
int *buf,len;
{
  int weg;

  T(send_data)
  D(D_COMM,printf("schreibe %d Worte\n",len);)
  swap_falls_noetig(buf,len);
  weg=sock_write(&s,buf,len*4);
  if(weg!=len*4)printf("Schreibfehler\n");
  D(D_COMM,printf("geschrieben\n");)
  return(0);
}
