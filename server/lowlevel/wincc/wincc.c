#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <xdrfloat.h>
#include <swap.h>
#include "wincc.h"

typedef struct
  {
  int path;
  } wincc_path;

static wincc_path** wincc_pathes;
static int num_wincc_pathes;

typedef struct
  {
  int length;
  int command;    /* wincc_commands */
  int error_code; /* wincc_errors */
  int sequence_number;
  int varcount;
  } pdu_hdr;

#if 0
/* XDR: */
typedef struct
  {
  string name<>;
  wincc_operations operation;
  wincc_verrors var_error;
  union switch (wincc_types)
    {
    case WCC_TBIT:    int val;
    case WCC_TBYTE:   int val;
    case WCC_TCHAR:   string val<>;
    case WCC_TDOUBLE: double val;
    case WCC_TFLOAT:  float val;
    case WCC_TWORD:   int val;
    case WCC_TDWORD:  int val;
    case WCC_TRAW:    opaque val<>;
    case WCC_TSBYTE:  int val;
    case WCC_TSWORD:  int val;
    }
  } var_spec;
#endif

static unsigned char sequence_number=0;
#define MAX_PDU_SIZE 0x10000

/*****************************************************************************/
errcode wincc_low_init(char* arg)
{
printf("wincc_low_init\n");
wincc_pathes=(wincc_path**)0;
num_wincc_pathes=0;
return OK;
}
/*****************************************************************************/
errcode wincc_low_done(void)
{
int i;
printf("wincc_low_done\n");
for (i=0; i<num_wincc_pathes; i++)
  {
  if (wincc_pathes[i]) close(wincc_pathes[i]->path);
  free(wincc_pathes[i]);
  }
free(wincc_pathes);
return OK;
}
/*****************************************************************************/
int wincc_low_printuse(FILE* outpath)
{
return 0;
}
/*****************************************************************************/
plerrcode wincc_open(int* path, char* hostname, int port)
{
int i=0, j, p;
struct in_addr addr;
struct sockaddr_in saddr;
struct hostent *host;

while ((i<num_wincc_pathes) && (wincc_pathes[i])) i++;
if (i==num_wincc_pathes)
  {
  wincc_path** help=(wincc_path**)malloc((num_wincc_pathes+1)*sizeof(wincc_path*));
  for (j=0; j<num_wincc_pathes; j++) help[j]=wincc_pathes[j];
  help[num_wincc_pathes]=(wincc_path*)0;
  free(wincc_pathes);
  wincc_pathes=help;
  num_wincc_pathes++;
  }

if ((addr.s_addr=inet_addr(hostname))==(in_addr_t)-1)
  {
  if ((host=gethostbyname(hostname))==0)
    {
    printf("wincc_open: cannot resolve hostname %s\n", hostname);
    fflush(stdout);
    return plErr_System;
    }
  if (host->h_length!=4)
    {
    printf("wincc_open: sorry, size of address is %d, not 4\n", host->h_length);
    fflush(stdout);
    return plErr_Program;
    }
  bcopy(host->h_addr_list[0], &addr.s_addr, host->h_length);
  }
printf("wincc_open: addr=%s\n", inet_ntoa(addr));

p=socket(AF_INET, SOCK_DGRAM, 0);
if (p<0)
  {
  printf("wincc_open: socket: %s\n", strerror(errno));
  fflush(stdout);
  return plErr_System;
  }
if (fcntl(p, F_SETFD, FD_CLOEXEC)<0)
  {
  printf("lowlevel/wincc/wincc.cfcntl(p, FD_CLOEXEC): %s\n", strerror(errno));
  }
bzero(&saddr, sizeof(struct sockaddr_in));
saddr.sin_family=AF_INET;
saddr.sin_port=htons(port);
saddr.sin_addr=addr;
if (connect(p, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in))<0)
  {
  printf("wincc_open: connect to %s:%d: %s\n", hostname, port, strerror(errno));
  fflush(stdout);
  close(p);
  return plErr_System;
  }
wincc_pathes[i]=(wincc_path*)malloc(sizeof(wincc_path));
wincc_pathes[i]->path=p;
printf("  wincc_pathes[i=%d]=%p\n", i, wincc_pathes[i]);
*path=i;
printf("wincc_open: path_id=%d, path=%d\n", *path, p);
return plOK;
}
/*****************************************************************************/
plerrcode wincc_close(int path)
{
printf("wincc_close(%d), num_wincc_pathes=%d\n", path, num_wincc_pathes);
if ((unsigned int)path>=num_wincc_pathes) return plErr_ArgRange;
printf("  wincc_pathes[path=%d]=%p\n", path, wincc_pathes[path]);
if (!wincc_pathes[path]) return plErr_ArgRange;
close(wincc_pathes[path]->path);
free(wincc_pathes[path]);
wincc_pathes[path]=0;
return plOK;
}
/*****************************************************************************/
wincc_errors wincc_var(int path, int num, wincc_var_spec* var_specs, int async)
{
wincc_errors wres=WCC_OK;
int res, weiter, pdu_size, i;
int *pdu, *ohelp;
const int* ihelp;
pdu_hdr* pduh;
struct timeval deadline;

printf("wincc_var: path_id=%d, var_num=%d, async=%d\n", path, num, async);

if (((unsigned int)path>=num_wincc_pathes) || !wincc_pathes[path])
  {
  printf("wincc_var: illegal path_id %d\n", path);
  return WCC_NO_PATH;
  }

pdu=malloc(MAX_PDU_SIZE);
if (!pdu)
  {
  printf("wincc_var: malloc: %s\n", strerror(errno));
  return -1;
  }
/* compute pdu-size (int terms of int) */
pdu_size=sizeof(pdu_hdr)/sizeof(int);
for (i=0; i<num; i++)
  {
  pdu_size+=strxdrlen(var_specs[i].name)+3;
  printf("var[%d].type=%d\n", i, var_specs[i].value.typ);
  switch (var_specs[i].value.typ)
    {
    case WCC_TBIT:   /* nobreak */
    case WCC_TBYTE:  /* nobreak */
    case WCC_TFLOAT: /* nobreak */
    case WCC_TWORD:  /* nobreak */
    case WCC_TDWORD: /* nobreak */
    case WCC_TSBYTE: /* nobreak */
    case WCC_TSWORD:
      pdu_size+=1;
    break;
    case WCC_TCHAR:
      if (var_specs[i].value.val.str)
        pdu_size+=strxdrlen(var_specs[i].value.val.str);
      else
        pdu_size+=1;
    break;
    case WCC_TDOUBLE:
      pdu_size+=2;
    break;
    case WCC_TRAW:
      if (var_specs[i].value.val.opaque.size)
        pdu_size+=(var_specs[i].value.val.opaque.size+3)/4+1;
      else
        pdu_size+=1;
    break;
    default:
      printf("wincc_var: unknown var-type %d\n", var_specs[i].value.typ);
      wres=WCC_USER_ERR;
      goto fehler;
    }
  }
if (pdu_size>MAX_PDU_SIZE)
  {
  printf("wincc_var: pdu-size too large: %d byte\n", pdu_size);
  wres=WCC_USER_ERR;
  goto fehler;
  }
pduh=(pdu_hdr*)pdu;
pduh->length=pdu_size*sizeof(int);
pduh->command=(int)WCC_REQ;
pduh->sequence_number=++sequence_number;
pduh->varcount=num;
ohelp=pdu+sizeof(pdu_hdr)/sizeof(int);
for (i=0; i<num; i++)
  {
  printf("var[%d].name=>%s<\n", i, var_specs[i].name);
  ohelp=outstring(ohelp, var_specs[i].name);
  printf("var[%d].operation=%d\n", i, var_specs[i].operation);
  *ohelp++=var_specs[i].operation;
  *ohelp++=0; /* error code */
  *ohelp++=var_specs[i].value.typ;
  switch (var_specs[i].value.typ)
    {
    case WCC_TBIT:   /* nobreak */
    case WCC_TBYTE:  /* nobreak */
    case WCC_TWORD:  /* nobreak */
    case WCC_TDWORD: /* nobreak */
    case WCC_TSBYTE: /* nobreak */
    case WCC_TSWORD:
      printf("int_val=%d\n", var_specs[i].value.val.i);
      *ohelp++=var_specs[i].value.val.i;
    break;
    case WCC_TCHAR:
      printf("str_val=>%s<\n", var_specs[i].value.val.str);
      if (var_specs[i].value.val.str)
        ohelp=outstring(ohelp, var_specs[i].value.val.str);
      else
        *ohelp++=0;
    break;
    case WCC_TFLOAT:
      printf("float_val=%g\n", var_specs[i].value.val.f);
      ohelp=outfloat(ohelp, var_specs[i].value.val.f);
    case WCC_TDOUBLE:
      printf("double_val=%g\n", var_specs[i].value.val.d);
      ohelp=outdouble(ohelp, var_specs[i].value.val.d);
    break;
    case WCC_TRAW:
      if (var_specs[i].value.val.opaque.size)
        ohelp=outnstring(ohelp, var_specs[i].value.val.opaque.vals,
            var_specs[i].value.val.opaque.size);
      else
        *ohelp++=0;
    break;
    }
  }
{
int i;
printf("to send:\n");
for (i=0; i<pdu_size; i++) printf("[%03d] %08x %10d\n", i, pdu[i], pdu[i]);
}
swap_falls_noetig(pdu, pdu_size);

res=send(wincc_pathes[path]->path, pdu, ntohl(pduh->length), 0);
if (res!=ntohl(pduh->length))
  {
  printf("wincc_var: send: res=%d, errno=%s\n", res, strerror(errno));
  fflush(stdout);
  wres=WCC_SYSTEM;
  goto fehler;
  }

do
  {
  bzero(pdu, MAX_PDU_SIZE);

  gettimeofday(&deadline, 0);
  deadline.tv_sec+=5;
  do
    {
    fd_set readfds;
    struct timeval timeout, jetzt;
    gettimeofday(&jetzt, 0);
    timeout.tv_sec=deadline.tv_sec-jetzt.tv_sec;
    timeout.tv_usec=deadline.tv_usec-jetzt.tv_usec;
    if (timeout.tv_usec<0)
      {
      timeout.tv_usec+=1000000;
      timeout.tv_sec--;
      }
    if (timeout.tv_sec<0)
      {
      printf("wincc_var: select: timeout(a)\n");
      fflush(stdout);
      wres=WCC_SYSTEM;
      goto fehler;
      }
    FD_ZERO(&readfds);
    FD_SET(wincc_pathes[path]->path, &readfds);
    res=select(wincc_pathes[path]->path+1, &readfds, 0, 0, &timeout);
    if (res==0)
      {
      printf("wincc_var: select: timeout(b)\n");
      fflush(stdout);
      wres=WCC_SYSTEM;
      goto fehler;
      }
    if ((res<0) && (errno!=EINTR))
      {
      printf("wincc_var: select: res=%d, errno=%s\n", res, strerror(errno));
      fflush(stdout);
      if (errno!=EINTR)
        {
        wres=WCC_SYSTEM;
        goto fehler;
        }
      else
        weiter=1;
      }
    else
      weiter=0;
    }
  while (weiter);

  res=recv(wincc_pathes[path]->path, pdu, MAX_PDU_SIZE, 0);
  if (res<=0)
    {
    printf("wincc_var: recv: res=%d, errno=%s\n", res, strerror(errno));
    fflush(stdout);
    wres=WCC_SYSTEM;
    goto fehler;
    }
  if (res%4!=0)
    {
    printf("wincc_var: recv: res=%d, not multiple of 4\n", res);
    fflush(stdout);
    wres=WCC_USER_ERR;
    goto fehler;
    }
  pdu_size=res/sizeof(int);
  printf("  res=%d, pdu_size=%d words\n", res, pdu_size);
  swap_falls_noetig(pdu, pdu_size);
  {
  int i;
  printf("received (after swap):\n");
  for (i=0; i<pdu_size; i++) printf("[%03d] %08x %10d\n", i, pdu[i], pdu[i]);
  }
  if (pdu_size!=pduh->length/sizeof(int))
    {
    printf("wincc_var: recv: pdu_size=%d, but length/4=%d\n", pdu_size,
        pduh->length/sizeof(int));
    fflush(stdout);
    wres=WCC_USER_ERR;
    goto fehler;
    }
  if (pduh->command!=WCC_RES) printf("wincc_var: command=%d\n", pduh->command);
  if (pduh->sequence_number!=sequence_number)
      printf("wincc_var: sequence_number=%d not %d\n", pduh->sequence_number,
          sequence_number);
  }
while (pduh->sequence_number!=sequence_number);

if (pduh->error_code!=WCC_OK)
  {
  printf("wincc_var: error=%d\n", pduh->error_code);
  wres=pduh->error_code;
  goto fehler;
  }
if (pduh->varcount!=num)
  {
  printf("wincc_var: varcount=%d not %d\n", pduh->varcount, num);
  wres=WCC_USER_ERR;
  goto fehler;
  }

ihelp=pdu+sizeof(pdu_hdr)/sizeof(int);
for (i=0; i<num; i++)
  {
  char* varname;
  wincc_operations operation;
  wincc_verrors var_error;
  wincc_types typ;

  if (pdu+pdu_size-ihelp<1)
    {
    printf("wincc_var[%d]: pdu exhausted (name)\n", i);
    wres=WCC_USER_ERR;
    goto fehler;
    }
  ihelp=xdrstrcdup(&varname, ihelp);
  if (strcmp(varname, var_specs[i].name))
    {
    printf("wincc_var[%d]: varname=>%s< not >%s<\n", i, varname,
        var_specs[i].name);
    free(varname);
    wres=WCC_USER_ERR;
    goto fehler;
    }
  free(varname);
  if (pdu+pdu_size-ihelp<4)
    {
    printf("wincc_var[%d]: pdu exhausted (operation)\n", i);
    wres=WCC_USER_ERR;
    goto fehler;
    }
  operation=(wincc_operations)*ihelp++;
  var_error=(wincc_verrors)*ihelp++;
  typ=(wincc_types)*ihelp++;
  if (operation!=var_specs[i].operation)
    {
    printf("wincc_var[%d]: operation=%d, not %d\n", i, operation,
        var_specs[i].operation);
    wres=WCC_USER_ERR;
    goto fehler;
    }
  if (typ!=var_specs[i].value.typ)
    {
    printf("wincc_var[%d]: type=%d, not %d\n", i, typ, var_specs[i].value.typ);
    wres=WCC_USER_ERR;
    goto fehler;
    }
  if (var_error!=WCC_V_OK)
    {
    printf("wincc_var[%d]: \"%s\": error=%d\n", i, var_specs[i].name, var_error);
    }
  var_specs[i].error=var_error;
  switch (typ)
    {
    case WCC_TBIT:   /* nobreak */
    case WCC_TBYTE:  /* nobreak */
    case WCC_TWORD:  /* nobreak */
    case WCC_TDWORD: /* nobreak */
    case WCC_TSBYTE: /* nobreak */
    case WCC_TSWORD:
      var_specs[i].value.val.i=*ihelp++;
      printf("  value: %d\n", var_specs[i].value.val.i);
    break;
    case WCC_TFLOAT:
      ihelp=extractfloat(&var_specs[i].value.val.f, ihelp);
      printf("  value: %f\n", var_specs[i].value.val.f);
    break;
    case WCC_TCHAR:
      free(var_specs[i].value.val.str);
      ihelp=xdrstrcdup(&var_specs[i].value.val.str, ihelp);
      printf("  value: >%s<\n", var_specs[i].value.val.str);
    break;
    case WCC_TDOUBLE:
      ihelp=extractdouble(&var_specs[i].value.val.d, ihelp);
      printf("  value: %f\n", var_specs[i].value.val.d);
    break;
    case WCC_TRAW:
      free(var_specs[i].value.val.opaque.vals);
      var_specs[i].value.val.opaque.size=*ihelp;
      ihelp=xdrstrcdup(&var_specs[i].value.val.opaque.vals, ihelp);
    break;
    }
  }

fehler:
free(pdu);
return wres;
}
/*****************************************************************************/
/*****************************************************************************/
