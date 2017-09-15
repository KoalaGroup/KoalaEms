/*************************************************************
**      Treiber zum Laden und Starten von DSP-Programmen    **
**      fuer die USE                                        **
**      copyright 1993 by G.Heinrichs, H.Rongen  ZEL        **
*************************************************************/

#include <stdio.h>
#include <time.h>


/**----------------------  Typen Deklarationen  ----------------------**/

#define   byte      unsigned char
#define   word      unsigned short int
#define   longint   unsigned long int

#define   TIMEOUT   100

typedef   enum {FALSE=0, TRUE=1}  boolean;


/**------------------  Register-Adressen des DSP32C  -----------------**/

word *par[4]  = {(word*) 0xff100000, (word*) 0xff200000, (word*) 0xff300000,
				 (word*) 0xff400000};
word *pdr[4]  = {(word*) 0xff100004, (word*) 0xff200004, (word*) 0xff300004,
				 (word*) 0xff400004};
word *pdr2[4] = {(word*) 0xff100018, (word*) 0xff200018, (word*) 0xff300018,
				 (word*) 0xff400018};
word *emr[4]  = {(word*) 0xff100008, (word*) 0xff200008, (word*) 0xff300008,
				 (word*) 0xff400008};
word *pcr[4]  = {(word*) 0xff10000e, (word*) 0xff20000e, (word*) 0xff30000e,
				 (word*) 0xff40000e};
word *pir[4]  = {(word*) 0xff100010, (word*) 0xff200010, (word*) 0xff300010,
				 (word*) 0xff400010};
word *pcrH[4] = {(word*) 0xff100014, (word*) 0xff200014, (word*) 0xff300014,
				 (word*) 0xff400014};
word *parE[4] = {(word*) 0xff100016, (word*) 0xff200016, (word*) 0xff300016,
				 (word*) 0xff400016};


/**--- Adressen des Sequenzers, der LED-Anzeige und des OUT-Portes ---**/

#define   MEMB		0x600000
#define   DATA_SEQ  0x200000
#define   ADR_SEQ   0x200004
#define   LED  		0x200014
#define   OUT  		0x20000c

word led_out[10] = {3, 159, 37, 13, 153, 73, 65, 31, 1, 9};


/**---------  Routinen zur Steuerung von DSP32C-Programmen  ----------**/

dspError (text)
char *text;
{
	printf ("dsp_drv: %s \n\n",text);
	exit();
}


stop_dsp (dsp_nr)
byte dsp_nr;
{
   *pcr[dsp_nr]  = 0x00;    /* Reset DSP                                 */
   *pcr[dsp_nr]  = 0x02;    /* enter Register Map 2                      */
   *pcrH[dsp_nr] = 0x02;    /* enter Register Map 3 --> full 16 bit Mode */
   *pcr[dsp_nr]  = 0x021a;  /* Auto DMA and Increment enable             */
   *parE[dsp_nr] = 0;       /* DMA Adress Pointer 0x000000               */
   *par[dsp_nr]  = 0;
}


init_dsp_adr (dsp_nr, adr)
byte dsp_nr;
longint adr;
{
   *pcr[dsp_nr]  = *pcr[dsp_nr] | 0x18; /* be sure DMA and Increment are enabled */
   *parE[dsp_nr] = (word) (adr >> 16);  /* die oberen 8 bit der 24 Bit Adresse   */
   *par[dsp_nr]  = (word) adr;          /* die unteren 16 bit der Adresse        */
}


run_dsp (dsp_nr)
byte dsp_nr;
{
   *pcr[dsp_nr]  = 0x00;
   *pcr[dsp_nr]  = 0x01;
   *pcr[dsp_nr]  = 0x1f;
   *pcrH[dsp_nr] = 0x02;
}


/**------ Routinen zum Lesen und Schreiben von Daten des DSP's  ------**/

float read_float (dsp_nr)
byte dsp_nr;
{
	longint sgn, exp, mant;
	union  {
			 float   f;
			 longint i;
			 word    dsp[2];
		   } data;
			
	data.dsp[1] = *pdr[dsp_nr];
	data.dsp[0] = *pdr[dsp_nr];
	
	sgn  = data.i & 0x80000000;						/* sign     */
	exp  = ( (data.i & 0x000000ff) << 24 );			/* exponent */
	mant = data.i & 0xffffff00;						/* mantisse */
	mant = (mant >> 8) & 0x007fffff;
	if (sgn)
		mant = (-mant) & 0x007fffff;		/* two's complement */
	if (exp && (!sgn || (sgn && mant)))
		exp = exp - 0x00000001;
	exp = (exp >> 1) & 0x7f800000;
	if (exp == 0x00000000)
		return (0.0);
	data.i = ( sgn | mant | exp );
	return ( data.f );
}


word read_word (dsp_nr)
byte dsp_nr;
{
	word data;
	
	data = *pdr[dsp_nr];
	return (data);
}


longint read_dword (dsp_nr)
byte dsp_nr;
{
	word d1, d2;
	longint daten;
	
	d1 = *pdr[dsp_nr];
	d2 = *pdr[dsp_nr];
	daten = ( d2 << 16 ) | d1;
	return (daten);
}


write_float (dsp_nr, daten)
byte dsp_nr;
float daten;
{
	longint sgn, exp, mant;
	longint dsp_data;
	union  {
			 float	 f;
			 longint i;
			 word    dsp[2];
		   } data;
			
	data.f = daten;
	sgn  = data.i & 0x80000000;				/* sign     */
	exp  = (data.i >> 23) & 0x000000ff;		/* exponent */
	mant = (data.i << 8) & 0x7fffffff;		/* mantisse */
	
	if ( (!sgn && exp) || (sgn && mant) )
		exp = exp + 0x00000001;
	if ( sgn )
		mant = -mant;				/* two's complement */
	data.i = ( sgn | mant | exp );		/* DSP32C float */
	
	*pdr[dsp_nr] = data.dsp[1];
	*pdr[dsp_nr] = data.dsp[0];
}	


write_word (dsp_nr, daten)
byte dsp_nr;
word daten;
{
	*pdr[dsp_nr] = daten;
}


write_dword (dsp_nr, daten)
byte dsp_nr;
longint daten;
{
	*pcrH[dsp_nr] = 0x03;
	*pdr2[dsp_nr] = (word) (daten & 0xffff);
	*pdr[dsp_nr]  = (word) (daten >> 16);
	*pcrH[dsp_nr] = 0x02;
}


/**-----------  Routinen zum Laden von DSP32C-Programmen  -----------**/

reset_dsp (dsp_nr)
byte dsp_nr;
{
	long dummy;
	
	stop_dsp( dsp_nr );						/*  DSP anhalten		*/
  	init_dsp_adr( dsp_nr, 0x200008 );		/*  Reset Sequenzer		*/
	write_word( dsp_nr, 0x0 );
	init_dsp_adr( dsp_nr, 0x200008 );
	dummy = (long) read_word( dsp_nr );
	for( dummy = 0; dummy < 0x1ffff; dummy++ )
	{}
	init_dsp_adr( dsp_nr, OUT );			/*  Reset I/O-Port		*/
	write_word( dsp_nr, 0x0 );
	init_dsp_adr( dsp_nr, LED );			/*  LED auf 4 setzen	*/
	write_word( dsp_nr, led_out[4] );
}


write_blk_word (dsp_nr, daten, count)
byte dsp_nr;
word *daten;
int  count;
{
int i;
word d1,d2;
	for (i=0; i<count; i++)
	{
	  d1 = *daten++;
	  d2 = (d1 >> 8) | (d1 << 8);
	  *pdr[dsp_nr] = d2;
	}		
}


long build_dword (adr)
byte *adr;
{
unsigned int i;
long e,h;
	e = 0;
	for (i=0; i<=3; i++)
	{
		h = *(adr+i);
		e = e + (h<<(i*8) );
	}
	return (e);
}


void build_name (adr, name)
char *adr, *name;
{
unsigned int i;
	*name = *"";
	for (i=0; i<8; i++)
	  if ( *(adr+i) > 0) strncat (name, adr+i, 1);
}


coff_load (dsp_nr, fn)
byte dsp_nr;
char *fn;
{
byte *buffer;
FILE *fp;
word  status;
byte  abbruch;
word  loops;
char  segname[10];
long  code_start, filezeiger, dsp_start_adr, segm_start, laenge;

	fp = fopen (fn, "r");
	if (fp==NULL) dspError ("coff_load: File not found");
	
    buffer = (byte*) malloc (4096);
    if (!buffer) dspError ("coff_load: Memory allocation failed");

    status = fread (buffer,1,20,fp);
    filezeiger = 20;
    loops      = 0;
    do
    {
    	loops++;
    	fseek (fp, filezeiger, 0);
    	fread (buffer,1,40,fp);
       	filezeiger = filezeiger + 40; 
    	build_name  (&buffer[0],segname);
    	dsp_start_adr = build_dword (&buffer[12]);
    	laenge        = build_dword (&buffer[16]);
    	segm_start    = build_dword (&buffer[20]);
        if (loops == 1) code_start = segm_start;
        abbruch = (filezeiger == code_start);	
     	if (laenge>0)
     	{
            printf ("Programm    %s \n",fn);  
	        printf ("Segment     %s \n",segname);
            printf ("DSP_start   %8lx h \n",dsp_start_adr);
            printf ("Laenge      %8lx h \n",laenge);
            printf ("Filestart   %8lx h \n",segm_start);
            printf ("Filezeiger  %8lx h \n",filezeiger);
            printf ("\n");
     		fseek (fp, segm_start,0);
     		init_dsp_adr (dsp_nr, dsp_start_adr);
     		while (laenge >= 4096)
     		{
     			fread (buffer,1,4096,fp);
     			write_blk_word (dsp_nr, buffer, 4096/2);
     			laenge = laenge - 4096;
     		}
     		if (laenge>0)
     		{
     			fread (buffer, 1, laenge, fp);
     			write_blk_word (dsp_nr, buffer, laenge/2);
            }
        } 
	}
	while (!abbruch);
    free (buffer);	
    fclose (fp);
}


/**----------  Routinen fuer das Handshake mit dem DSP32C  ----------**/

long lookup_dsp_label (fname, lname)
char *fname;
char *lname;
{
	FILE *fp;
	byte buffer[40];
	byte name[20];
	word count;
	long relocp, reloca, adr;
	int status;
	boolean found;
	
	adr = 0;
	fp = fopen (fname, "r");
	if (fp == NULL)
		dspError ("lookup_dsp_label: File not found");
	else
	{
		fread (buffer, 1, 20, fp);
		relocp = build_dword (&buffer[8]);	/* Tabellen start zeiger */
		reloca = build_dword (&buffer[12]); /* Anzahl der Eintraege  */
		status = fseek (fp, relocp, 0);
		if (status != 0)
			dspError ("lookup_dsp_label: SEEK error");
		else
		{
			count = 1;
			found = FALSE;
			do
			{
				status = fread (buffer, 1, 18, fp);
				if (status == 0)
					dspError ("lookup_dsp_label: FSCANF error");
				else
				{
 					build_name (&buffer[0], name);
					if (strcmp (lname, name) == 0)
					{
						adr = build_dword (&buffer[8]);
						found = TRUE;
					}
					count++;
				}
			}
			while ( !((count > reloca) || found) );
		}
		fclose (fp);
	}
	return (adr);
}


boolean ready_dsp (dsp_nr, timeout)
byte dsp_nr;
word timeout;
{
	boolean done;
	time_t start, end;
	word pir_data;
	
	done = FALSE;
	time (&end);
	time (&start);
	while ( !done && (difftime (end, start) < (double) timeout) )
	{
		done = *pcr[dsp_nr] & 0x40;
		time (&end);
	}
	if ( difftime (end, start) >= (double) timeout )
		dspError ("ready_dsp:  PIR timeout");
	else
		pir_data = *pir[dsp_nr];
	return (done);
}


boolean write_pir (dsp_nr, daten)
byte dsp_nr;
word daten;
{
	*pir[dsp_nr] = daten;
}


/**----- Routine zur Ausgabe von Werten auf der LED-Anzeige -----**/

set_led (dsp_nr, index)
byte dsp_nr;
int index;
{
	init_dsp_adr( dsp_nr, LED );
	write_word( dsp_nr, led_out[index] );
}


/**------ Routine zur Ausgabe von Werten auf dem DSP-Port -------**/

out_dsp (dsp_nr, daten)
byte dsp_nr;
word daten;
{
	init_dsp_adr( dsp_nr, OUT );
	write_word( dsp_nr, daten );
}


/**------- Routinen zum Laden und Starten des Sequenzers --------**/

reset_seq (dsp_nr)
byte dsp_nr;
{
	word dummy;
	
	init_dsp_adr( dsp_nr, 0x200008 );
	write_word( dsp_nr, 0x0 );
	init_dsp_adr( dsp_nr, 0x200008 );
	dummy = read_word( dsp_nr );
}


dsp_ready (dsp_nr)
byte dsp_nr;
{
	init_dsp_adr( dsp_nr, 0x200010 );
	write_word( dsp_nr, 0x1 );
}


write_seq (dsp_nr, adr, data)
byte dsp_nr;
word adr;
long data;
{
	word d0, d1, d2, d3;
	
	/**----- Datenwort in je 6 bit Worte aufspalten -----**/
	
	d0 = (word) (( data		   & 0x3f) | 0x40);		/*  bit 0..5    */
	d1 = (word) (((data >>  6) & 0x3f) | 0x40);		/*  bit 6..11   */
	d2 = (word) (((data >> 12) & 0x3f) | 0x40);		/*  bit 12..17  */
	d3 = (word) (((data >> 18) & 0x3f) | 0x40);		/*  bit 18..23  */
	
	/**-------- Adresse des jeweiligen Registers --------**/
	
	init_dsp_adr( dsp_nr, ADR_SEQ );
	write_word( dsp_nr, adr );

	/**----- Laden der 4 Datenworte in den Sequenzer ----**/

	init_dsp_adr( dsp_nr, DATA_SEQ );
	write_word( dsp_nr, d0 );
	write_word( dsp_nr, d1 );
	init_dsp_adr( dsp_nr, DATA_SEQ );
	write_word( dsp_nr, d2 );
	write_word( dsp_nr, d3 );
}


set_mode (dsp_nr)
byte dsp_nr;
{
	/**---------- Adresse des Control-Registers ----------**/

	init_dsp_adr( dsp_nr, ADR_SEQ );
	write_word( dsp_nr, 0x34 );

	/**---------- Betriebs-Modus des Sequenzers ----------**/

	init_dsp_adr( dsp_nr, DATA_SEQ );
	write_word( dsp_nr, 0x01 );
}


load_seq (dsp_nr, laenge)
byte dsp_nr;
long laenge;
{
	write_seq( dsp_nr, 0x20, 0 );				/*  Startadresse		*/
	write_seq( dsp_nr, 0x24, laenge );			/*  Blockgroesse		*/
	write_seq( dsp_nr, 0x28, 1 );				/*  Anzahl der Bloecke	*/
	write_seq( dsp_nr, 0x2c, 0 );				/*  Block-Schrittweite	*/
	write_seq( dsp_nr, 0x30, 1 );				/*  Adress-Schrittweite	*/
	set_mode( dsp_nr );							/*  Betriebs-Modus		*/
}
