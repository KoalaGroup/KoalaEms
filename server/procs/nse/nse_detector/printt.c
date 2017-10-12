
#include <time.h>
char *month[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct"
    ,"Nov","Dec"} ;
char tim_buf[50] ;
char *printt(name)
char *name ;
{
struct sgtbuf t;
getime(&t) ;
sprintf(tim_buf,"%s -- %d-%s-%d %d:%d:%d ",name,
  t.t_day,month[t.t_month-1],t.t_year,t.t_hour,t.t_minute,t.t_second) ;
return tim_buf ;
}
