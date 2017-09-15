/******************************************************************************
*                                                                             *
* evsave.c                                                                    *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* created 16.07.94                                                            *
* last change 17.07.94                                                        *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/dir.h>
#include <pwd.h>
#include <unistd.h>
#include <getevent.h>

extern char grund[];

char* rindex(char*, char);

/*****************************************************************************/

int debug=0;
int verbose=0;
char* gzippath=0;
int maxsize=-1;
int freesize=-1;
int istime;
char* name=0;
char* xname=0;
char* dname=0;
char* user=0;
int uid, gid;
FILE* cons;
FILE* outfile;

/*****************************************************************************/

char *usage[]=
  {
  "usage: %s [-d] [-v] [-g <gzip_path>] [-s <max size>] [-f <free size>] \\\n",
  "  [-u <username>] [-n] name\n",
  "\n",
  NULL,
  };

char optstring[]="dvg:s:f:hn:u:";

/*****************************************************************************/

void printusage(char *name)
{
int i;

i=0;
while (usage[i]!=NULL)
  {
  printf(usage[i], name);
  i++;
  }
}

/*****************************************************************************/

int getint(char* arg, int* val)
{
int res;
char dummy[1024];
res=sscanf(arg, "%d%s", val, dummy);
if (res!=1)
  {
  printf("cannot convert \"%s\"\n", arg);
  return(-1);
  }
return(0);
}

/*****************************************************************************/

int readargs(int argc, char *argv[])
{
int optchar;
extern int optind;
extern char *optarg;
int optfail=0;

optfail=0;
while (((optchar=getopt(argc, argv, optstring))!=EOF) && !optfail)
  {
  switch (optchar)
    {
    case '?':
    case 'h': optfail=1; break;
    case 'd': debug=1; break;
    case 'v': debug=1; verbose=1; break;
    case 'g': gzippath=optarg; break;
    case 'n': name=optarg; break;
    case 'u': user=optarg; break;
    case 's': if (getint(optarg, &maxsize)!=0) optfail=1; break;
    case 'f': if (getint(optarg, &freesize)!=0) optfail=1; break;
    default: optfail=1; break;
    }
  }
if (argv[optind]!=NULL)
  {
  name=argv[optind++];
  }
if (argv[optind]!=NULL)
  {
  printf("argument \"%s\" not expected\n", argv[optind]);
  optfail=1;
  }
if (!optfail && (name==0))
  {
  printf("name must be given\n");
  optfail=1;
  }
if (optfail)
  {
  int i;
  for (i=0; i<argc; i++) printf("%s%s", argv[i], i+1<argc?", ":"\n");
  printusage(argv[0]);
  return(-1);
  }
return(0);
}

/*****************************************************************************/

#ifdef __osf__

int df(char* path)
{
struct statfs buffer;

res=statfs(path, &buffer, sizeof(struct statfs));
if (res==-1)
  {
  perror("statfs");
  return(-1);
  }

return(buffer.f_bavail*2);
}

#else

int df(char* path)
{
struct fs_data buffer;
int res;

res=getmnt(0, &buffer, 0, STAT_ONE, path);
if (res==-1)
  {
  perror("getmnt");
  return(-1);
  }
else if (res==0)
  {
  printf("not mounted\n");
  return(-1);
  }

return(buffer.fd_req.bfreen);
}

#endif

/*****************************************************************************/

char* make_dname(char* name)
{
char* p;

p=rindex(name, '/');
if (p==0)
  {
  return(".");
  }
else
  {
  int n;
  n=p-name;
  while ((n>0) && (name[n-1]=='/')) n--;
  if (n==0)
    return("/");
  else
    {
    char* dname;
    dname=(char*)malloc(n+1);
    strncpy(dname, name, n);
    return(dname);
    }
  }
}

/*****************************************************************************/

char* make_fname(char* name)
{
char *p, *fname;

p=rindex(name, '/');
if (p==0)
  p=name;
else
  p++;

fname=(char*)malloc(strlen(p)+1);
strcpy(fname, p);
return(fname);
}

/*****************************************************************************/

int setuser(char* user)
{
struct passwd* buffer;

buffer=getpwnam(user);
if (buffer==0)
  {
  printf("user \"%s\" not found\n", user);
  return(-1);
  }
uid=buffer->pw_uid;
gid=buffer->pw_gid;
if (setgid(gid)!=0)
  {
  perror("setgid");
  return(-1);
  }
if (setuid(uid)!=0)
  {
  perror("setuid");
  return(-1);
  }
if (debug)
  {
  fprintf(cons, "evsave: set uid=%d; gid=%d\n", getuid(), getgid());
  fflush(cons);
  }
return(0);
}

/*****************************************************************************/

char* _indexedname(char* name) /* dumme Version */
{
int idx, ok;
struct stat buf;
char* xname;

xname=(char*)malloc(strlen(name)+5);

ok=0;
for (idx=0; ok==0; idx++)
  {
  sprintf(xname, "%s_%03d", name, idx);
  if (stat(xname, &buf)!=0)
    {
    if (errno==ENOENT)
      ok=1;
    else
      {
      perror("stat");
      free(xname);
      return(0);
      }
    }
  }
return(xname);
}

/*****************************************************************************/

char* indexedname(char* name)
{
DIR* dirp;
struct direct* dp;
int len, idx, maxidx;
char dummy[1024];
char* fname;
char* xname;
char* dname;

dname=make_dname(name);
fname=make_fname(name);
if (verbose)
  {
  fprintf(cons, "iname: dname=>%s<, fname=>%s<\n", dname, fname);
  fflush(cons);
  }
len=strlen(fname);

dirp=opendir(dname);
if (dirp==0)
  {
  perror("opendir");
  return(0);
  }

maxidx=-1;
for (dp=readdir(dirp); dp != NULL; dp = readdir(dirp))
  {
  if (strncmp(dp->d_name, fname, len)==0)
    {
    int res;
    res=sscanf(dp->d_name+len+1, "%d%s", &idx, dummy);
    if (res!=1)
      printf("cannot interpret \"%s\"\n", dp->d_name);
    else
      {if (idx>maxidx) maxidx=idx;}
    }
  }
closedir(dirp);
idx=maxidx+1;

if (strlen(dname)==0)
  {
  xname=(char*)malloc(strlen(name)+5);
  sprintf(xname, "%s_%03d", fname, idx);
  }
else if (strcmp(dname, "/")==0)
  {
  xname=(char*)malloc(strlen(name)+6);
  sprintf(xname, "/%s_%03d", fname, idx);
  }
else
  {
  xname=(char*)malloc(strlen(dname)+strlen(name)+6);
  sprintf(xname, "%s/%s_%03d", dname, fname, idx);
  }
return(xname);
}

/*****************************************************************************/

void do_events(int in, FILE* out)
{
int* event;
int ok, filesize, msize, eventnum;

msize=maxsize*256; /* kB in sizeof(int) */
ok=1; filesize=0;
for (;;)
  {
  if (istime)
    {
    int free;
    free=df(dname);
    if (verbose)
      {
      fprintf(cons, "evsave: %d kB free in %s\n", free, dname);
      fflush(cons);
      }
    if (free<=freesize)
      {
      ok=0;
      if (debug)
        {
        fprintf(cons, "evsave: no space on fs\n");
        fflush(cons);
        }
      }
    istime=0;
    alarm(60);
    }
  if ((msize>0) && (filesize>=msize))
    {
    ok=0;
    if (debug)
      {
      fprintf(cons, "evsave: file too big\n");
      fflush(cons);
      }
    }
  if (!ok) return;
  if ((event=getevent(in))==0)
    {
    if (debug)
      {
      fprintf(cons, "got no event: %s, last event: %d\n", grund, eventnum);
      fflush(cons);
      }
    return;
    }
  eventnum=event[1];
  if (fwrite(event, 4, event[0]+1, out)==0)
    {
    perror("fwrite");
    return;
    }
  filesize+=event[0]+1;
  }
}

/*****************************************************************************/

void sighand(int sig)
{
istime=1;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
if (readargs(argc, argv)!=0) exit(0);
if (debug)
  {
  cons=fopen("/dev/console", "a");
/*cons=fopen("/dev/tty", "a");*/
  if (cons==0) {perror("open console"); exit(0);}
  fprintf(cons, "start evsave: verbose=%d, gzippath=>%s<, "
  "maxsize=%d, freesize=%d, name=>%s<\n",
  verbose, gzippath, maxsize, freesize, name);
  fflush(cons);
  }

if (freesize>0)
  {
  int free;
  dname=make_dname(name);
  if ((free=df(dname))==-1) exit(0);
  if (debug)
    {
    fprintf(cons, "evsave: %d kB free on %s\n", free, dname);
    fflush(cons);
    }
  }

if (user!=0)
  {
  if (setuser(user)!=0) exit(0);
  }

if ((xname=indexedname(name))==0) exit(0);
if (debug)
  {
  fprintf(cons, "evsave: xname=>%s<\n", xname);
  fflush(cons);
  }

if ((outfile=fopen(xname, "w"))==0)
  {
  perror("fopen outfile");
  exit(0);
  }

if (freesize>0)
  {
  struct sigaction sig;

  sig.sa_handler=sighand;
  sigemptyset(&sig.sa_mask);
  sig.sa_flags=0;
  sigaction(SIGALRM, &sig, (struct sigaction*)0);
  istime=0;
  alarm(60);
  }

do_events(0, outfile);

fclose(outfile);
if (debug)
  {
  fprintf(cons, "stop evsave\n");
  fflush(cons);
  fclose(cons);
  }
}

/*****************************************************************************/
/*****************************************************************************/
