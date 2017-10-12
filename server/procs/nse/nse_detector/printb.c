#include <config.h>
#include <debug.h>
#include <types.h>

/***************************************************/
/*		print buffer                       */
/***************************************************/
printb(typ,buf,count)
unsigned          char  typ;
register unsigned char  *buf;
register          long  count;
{
register unsigned long  j,line ;
line=0;
while(count)
	{
	if (typ == 'c')                 /* print characters */
		for (j = count > 64 ? 64 : count, count=count-j ; j ; j--)
		printf("%c",*buf++);
	else if (typ == 'b')             /* print binary */
		for (j = count > 32 ? 32 : count, count=count-j ; j ; j--)
		printf("%2x",*buf++);
	else if (typ == 's')             /* print binary */
		for (j = count > 16 ? 16 : count,count=count-j ; j ; j--) 
		printf("%4x ",*((unsigned short *)buf)++);
	else if (typ == 'l')             /* print binary */
		for (j = count >  8 ?  8 : count, count=count-j ; j ; j--)
		printf("%8x ",*((unsigned long *)buf)++);
	else if (typ == 'd')             /* print decimal */
		{printf("%4d: ",line) ; line += 8 ;
		for (j = count >  8 ?  8 : count, count=count-j ; j ; j--)
		printf("%d ",*((unsigned long *)buf)++);}
	else {j=count ; printf(" no type %c",typ);}
	printf("\n");
	}
}		
