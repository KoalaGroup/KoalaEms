#include <instcom.h>
/*#include "/n2/eth/ethos9.h"*/
#define  P_BYTE 0
#define  P_LONG 1
#define  P_NULL 2
#define  MAX_BUF 1460
#define  TOF_LOG (ptr_instcom->control_log & 0x00000008)
#define  INBYTES VPB_SIZE<<2
/***************************** Static Variables ***************************/
long              my_lln=0;
pparam            prot;
extern instcom    *ptr_instcom, *link_instcom() ;
unsigned char     *b_b, b_buf[MAX_BUF], eth_buf[MAX_BUF] ;
unsigned char     sequ ;
typedef struct    {short count; short type;} _item;
typedef struct    {short count; _item item[1];} _pack;
_pack             *p_pack;
_item             *p_item;
long              max_buf;
/********************************* init_x **********************************/
init_x()
{
   long r;
   ptr_instcom = link_instcom() ;
   my_lln=2 ;
   r=initialize_link(my_lln,ptr_instcom->host_node,0,INBYTES,-1) ;
   if (r != TR_SUCC ) printf("initialize err %d\n",r);
   return r;
 }
/********************************* reset_x **********************************/
reset_x()
{
   b_b = b_buf ;
   sequ=1;
   p_pack=(_pack *)eth_buf ;
   p_pack->count=0;
   max_buf=MAX_BUF;
 }
/********************************* close_x **********************************/
close_x()
{
   long r ;
   r=close_link(my_lln) ;
   my_lln = 0 ;
   return r;
}
/********************************* get_instcom ****************************/
get_instcom()
{
long    r;
prot.p_elem = VPB_SIZE ;
prot.p_fmt = FMT_I4 ;
prot.p_tim = 0 ;
if ( (r=read_link(my_lln,ptr_instcom->_vpb._l,&prot)) != TR_SUCC )
   printf("Cannot read VAX common, err %d\n",r);
return r;
}
/********************************* put_instcom ****************************/
put_instcom()
{
return (transfer(ptr_instcom->_orb._l , ORB_SIZE, FMT_I4));
}

/********************************* put_data ****************************/
put_data(buf,count)
unsigned long     *buf;
long     count ;
{
unsigned long     *top , *lbuf;
_item l_item ;
long r;

if (buf){
   if (!ptr_instcom->pack_data) { /* No data packing */
      l_item.count=count<<2 ;    /* in bytes */
      l_item.type =P_LONG ;
      return ( pack(buf,&l_item) ) ;
      }
   top = buf+count ;
   while (buf < top) { /* scan data buffer for type */
      l_item.count=0 ;
      if (*buf == 0) {            /* Zero data */
         lbuf=buf ;
	 for (lbuf=buf; (lbuf < top ) && (*lbuf == 0); lbuf++)l_item.count++;         /*  NULL descriptor */
         l_item.type=P_NULL ;
          /*  BYTE  or LONG descriptor */
         if (l_item.count <= 8 ) {
           if (p_pack->count && lbuf < top) { /* not at border */
              if ( p_item->type == P_BYTE && *lbuf < 256) l_item.type=P_BYTE;
              else if ( ( p_item->type == P_BYTE || *lbuf < 256) &&
                (l_item.count <= 4 ) ) l_item.type=P_BYTE;
              else if ( ( p_item->type == P_LONG && *lbuf >=256) &&
                (l_item.count <= 2 ) ) l_item.type=P_LONG;
              }
           else if (lbuf >= top) { /* last item in buffer */
              if ( p_item->type == P_BYTE && l_item.count < 4 )
                 l_item.type=P_BYTE;
              else if ( p_item->type == P_LONG && l_item.count < 2)
                 l_item.type=P_LONG;
              }
           else if (!p_pack->count) { /* first item in buffer */
              if (*lbuf < 256 && l_item.count < 4 ) l_item.type=P_BYTE;
              else if (*lbuf >= 256 && l_item.count < 2) l_item.type=P_LONG;
              }
           }
         }
      else if (*buf < 256) {            /* Byte data */
         lbuf=buf ;
	 for (lbuf=buf; (lbuf < top ) && (*lbuf && *lbuf < 256); lbuf++)
         l_item.count++;
         /*  BYTE descriptor */
         l_item.type=P_BYTE ;
         /*  BYTE  or LONG descriptor */
         if (l_item.count < 3 ) {
           if (p_pack->count && lbuf < top) { /* not at border */
              if ( p_item->type == P_LONG || *lbuf >= 256) l_item.type=P_LONG;
              }
           else if (lbuf >= top) { /* last item in buffer */
              if ( p_item->type == P_LONG && l_item.count == 1)
                l_item.type=P_LONG;
              }
           else if (!p_pack->count) { /* first item in buffer */
              if (*lbuf >= 256 && l_item.count == 1 ) l_item.type=P_LONG;
              }
           }
         }
      else{                  /* Long word data */
         lbuf=buf ;
	 for (lbuf=buf; (lbuf < top ) && (*lbuf >= 256); lbuf++)l_item.count++;
         /*  LONG descriptor */
         l_item.type=P_LONG ;
         }
      if (l_item.type == P_LONG)  l_item.count <<= 2 ;
      if ( (r=pack(buf, &l_item)) != TR_SUCC) return r;
      buf=lbuf ;
      }
   return r ;  
   }
return transfer_packed();
}


/********************************* pack ********************************/
pack(buf,lp_item)
register unsigned char   *buf;
register _item *lp_item;
{
long               t_c , r ;

while (lp_item->count){
 if (!p_pack->count || p_item->type != lp_item->type ||
 (p_item->count+lp_item->count) >= 32768) {
   /* Add new packing item */
   if (max_buf < 8) { /* Transfer last block */  
     if ((r=transfer_packed()) != TR_SUCC)   return r;
     }
   add_new_pack(lp_item->type);
   }
 if (lp_item->type != P_NULL) {
   t_c =  lp_item->count > max_buf ?  max_buf&~3 : lp_item->count ;
   max_buf -= t_c;
   copy_packed (b_b,buf,t_c,lp_item->type);
   b_b += t_c ;
   lp_item->count -= t_c ;
   p_item->count += t_c ;
   if ( lp_item->type == P_BYTE ) buf += t_c<<2 ; else  buf += t_c ;
   }
 else {
   p_item->count += lp_item->count ;
   lp_item->count=0 ;   
   }
 if (max_buf < 5) { /* if byte buffer full: transfer */
   if ((r=transfer_packed()) != TR_SUCC) return r;
   }
 }
return TR_SUCC ;
}
/********************************* add_new_pack ****************************/
add_new_pack(type)
short type;
{
if (!p_pack->count) { /* initializs pack buffer */
   p_item = p_pack->item ;
   max_buf = MAX_BUF - 2 ;
   }
else p_item++ ;
p_pack->count++;
p_item->count=0;
p_item->type=type;
max_buf -= 4;
}
/********************************* add_new_pack ****************************/
copy_packed (to,from,count,type)
register unsigned char *to,*from;
register short         count,type;
{
if (type == P_BYTE)
   while (count--) {
                   from += 3;
                   *to++=*from++;
                   }
else if (type == P_LONG)
   while (count) {
                   *to++=*(from+3);
                   *to++=*(from+2);
                   *to++=*(from+1);
                   *to++=*from;
                   from += 4;
                   count -= 4;
                   }
}

/****************************** transfer_packed *************************/
transfer_packed()
{
short    i , t_count ;

i=TR_SUCC ;
if (i = p_pack->count) { /* Something to transfer */
   if (TOF_LOG) printf("Packed %d format elements\n",i);
   t_count=0 ;
   p_item=p_pack->item ;
   while (i--) {
      if ( p_item->type == P_BYTE) {
         t_count += p_item->count ;
/*         if (TOF_LOG) printf("   %d BYTES \n",p_item->count);*/
         }
      else if ( p_item->type == P_LONG) {
         t_count += p_item->count ;
         p_item->count >>= 2;
/*         if (TOF_LOG) printf("   %d LONGS \n",p_item->count);*/
         }
      else if ( p_item->type == P_NULL) {
/*         if (TOF_LOG) printf("   %d NULL \n",p_item->count);*/
         }
      /* Big ending integer for VAX */
      p_item->type = byteswap(p_item->type);
      p_item->count= byteswap(p_item->count);
      p_item++;
      }
   _strass(p_item,b_buf,t_count) ;
   t_count += 2 + 4*p_pack->count ;
   /* Big ending integer for VAX */
   p_pack->count=byteswap(p_pack->count);
   /* transfer descriptors and packed data */
   i=transfer(eth_buf,t_count,0);
   /* reset pointers */
   b_b = b_buf ;                /* packed data */
   p_pack->count=0;             /* number of descriptors */
   max_buf = MAX_BUF ;
   }
return i ;
}         
/********************************* transfer ****************************/
transfer(from,no,fmt)
unsigned long *from;
long          no;
unsigned char fmt;
{
long   r;

prot.p_elem = no ;
prot.p_fmt = fmt ;
prot.p_nmess = sequ++  % 128;

if (TOF_LOG) {
   printf("Transfer %d elements, format %d, sequence %d, addr %x, ",
           prot.p_elem,prot.p_fmt,prot.p_nmess,from); 
   if (!fmt)  printf("from %d to %d\n",*(unsigned char *)from,
                 *( (unsigned char *)from+prot.p_elem-1) );
   else  printf("from %d to %d\n",*from,*(from+prot.p_elem-1));
   }
if ( (r=write_link(my_lln,from,&prot)) != TR_SUCC ) 
   printf("write err %d,seq %d, size %d\n",r,prot.p_nmess,prot.p_elem);
return r;
}

/********************************* THE END ****************************/
