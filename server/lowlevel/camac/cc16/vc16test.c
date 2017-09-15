/*  W-Ie-Ne-R CC16/VC16 Test */
/*  Version 1.1, revised 2/95 */

#include <stdio.h>
#include "cam_vc16.c"

int main()
{
   int   n1,f1,a1;
   int   q,x;
   long int   data;
   char  inbuf [130];
   cam_controller_ini(0x800000);
   n1=1;
   printf("\n            W-IE-NE-R CC16/VC16 DEMONSTRATION  \n");
   printf("\n   ------------------------------------------------- \n \n");
   printf("\n NAF code operation demonstration (F=0 to stop) \n");
   printf("\n copyright Plein & Baus GmbH, Burscheid/Germany - VI/94 \n \n");

/* demonstration of clear, inhibit and inizialise */

   cam_c();
   cam_i();
   cam_z();
   cam_ci();
   set_delay(0x0);

/* demonstration of NAF read and write operations */

cam_irq();

   while (n1 != 0)
   {
     printf("\n   -------------------------------------------------  \n");
     printf("\n CAMAC station number   N=  \t");
     gets(inbuf);
     sscanf(inbuf, "\t%d", &n1);
     printf("\n CAMAC function number  F= \t");
     gets(inbuf);
     sscanf(inbuf, "\t%d", &f1);
     printf("\n CAMAC subaddress       A= \t");
     gets(inbuf);
     sscanf(inbuf, "\t%d", &a1);
     printf("\n");
     if (f1>8)
     {
     printf(" data to write : \t");
     gets(inbuf);
     sscanf(inbuf, "\t%li", &data);
     cam_nfaqx_write24(n1,f1,a1,data,&q,&x);
     printf("\t\t\t  Q= %i    X= %i",q,x);
     }
     else
     {
     data = cam_nfaqx_read24(n1,f1,a1,&q,&x);
     printf(" data= %li",data);
     printf("\t Q= %i    X= %i \n",q,x);
     }
   }
}
