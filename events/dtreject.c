int* get_event()
{
static int event[10000];
int res;

res=read(0, (char*)event, 4);
if (res!=4)
  {
  perror("read head"); return 0;
  }
res=read(0, (char*)(event+1), 4*event[0]);
if (res!=4*event[0])
  {
  perror("read event"); return 0;
  }
return event;
}

int* get_subevent(int* event, int isid)
{
if (event[3]!=1) {printf("%d subevents\n", event[3]); return 0;}
if (event[4]!=isid) {printf("subevent is %d\n", event[4]); return 0;}
return event+5;
}

int get_data(int* sev, int* tdt, int* rej)
{
if (sev[0]!=4) {printf("subeventsize=%d\n", sev[0]); return -1;}
*tdt=sev[3]; *rej=sev[4];
return 0;
}

long sum[1000];
long num[1000];

add_rej(int tdt, int rej)
{
if (rej<1000)
  {
  sum[rej]+=tdt;
  num[rej]++;
  }
}

main()
{
int* event;
int* subevent;
int tdt, rej;
int i, nn=0;

for (i=0; i<1000; i++)
  {
  sum[i]=0;
  num[i]=0;
  }

while (1)
  {
  event=get_event();
  if (!event) goto raus;
  subevent=get_subevent(event, 1000);
  if (!subevent) goto raus;
  if (get_data(subevent, &tdt, &rej)<0) goto raus;
  nn++;
  if (nn%10000==0) printf("%d\n", nn);
  if (rej>0)
    {
    add_rej(tdt, rej);
    /*printf("tdt=%d, rej=%d\n", tdt, rej);*/
    }
  }

raus:
for (i=0; i<1000; i++)
  {
  if (num[i]>0) printf("%4d: %16d %f\n", i, num[i], (float)sum[i]/(float)num[i]);
  }
}
