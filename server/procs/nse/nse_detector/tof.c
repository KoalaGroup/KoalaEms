#include <stdio.h>

#include <errorcodes.h>
#include <modultypes.h>
#include <config.h>
#include <debug.h>
#include <types.h>

#include <signal.h>
#include "instcom.h"
#include "tof.h"
#include "./detector.h"

put_instcom(){};
put_data(){};
get_instcom(){};
init_x(){};
reset_x(){};
close_x(){};
short *mapmod(){};

/*POHL das wars*/

/* cards address   */
extern d_ba[];

/* acquisition parameters */
  extern p_res,p_div,p_del,p_bit,p_trg,pchwidt,
         w_length,n_module;                     

extern instcom *ptr_instcom ,*link_instcom() ;

extern 	 r, trigger_value ;
extern	 _tof_ev_id, mask, out ;
extern     spc_length ;
extern long     buf[B_S], raw[B_S], sums[B_S] ;        /* read spectra table       */
extern     xdet, ydet, xydet ;                    /* multi detector resolution*/

void help()
{ 
  printf("\n Syntax  : tof [<opt>] {<parameter>}\n");
  printf(" Function: control TOF unit\n") ;
  printf(" Options:\n") ;
  printf("         -d      disable acquisition\n");
  printf("         -e      enable acquisition\n") ;
  printf("         -g      read data (with grouping option)\n") ;
  printf("         -n      software reset\n") ;
  printf("         -p {c=} {f=} {o=} {w=} write parameters\n") ;
  printf("                     c= channel time resolution\n") ;
  printf("                     f= read trigger distance factor\n") ;
  printf("                     o= trigger delay\n") ;
  printf("                     w= w/base_clock=channel width\n") ;
  printf("         -r[=n]  read spectra n or all\n") ;
  printf("         -s[=n]  read sum spectra n or all\n") ;
  if (!MULTI_DET)
     printf("         -z[=n]  erase n or all spectra\n") ;
  else
     printf("         -z      erase memories\n") ;
  printf(" Default option: read all spectrum\n") ;
}

init()
{
 /* begin -- init -- */

register i, j ;

         if ((_tof_ev_id = _ev_link("tof_event")) == ERROR)
             _tof_ev_id = _ev_creat(0,1,-1,"tof_event") ;

printf("POHL_4:init\n");
/*
	 p_res = ptr_instcom->t_res ;
	 p_div = ptr_instcom->clock_divid ;
	 p_del = ptr_instcom->trig_delay ;
	 p_trg = ptr_instcom->trig_read_factor ;
*/
p_res=1; /* Bedeutet: Multidetector & not TOF */
p_bit=0 ; /* p_bit = 0, da p_res=1 folgt aus Orginalprogramm */

p_div=0;p_del=0;p_trg=0;

/* Twardo Initialisierung */
ptr_instcom->d_mask[0] = 1; /* nur in start- und stopaquitation; in main */
                            /* wird xdet, ydet damit bestimmt */

printf("POHL_5:p_ res= %d div= %d del= %d trg= %d\n",p_res,p_div,p_del,p_trg);

          n_module=CH_CARD_NB ;                     /* card channel number */
	 pchwidt  = DEF_CHWIDT ;   /* variable channel width(not use yet) */
	 w_length = DEF_W_LENGTH ;		   /* default 32 bits     */
	 return 0;
 /* end  -- init -- */
}

/**************************************************************************/

execut_cmd(ba,f_code,par_nb,p1,p2,p3,p4)

short *ba ;
int f_code ,par_nb ;
unsigned p1,p2,p3,p4 ;

{/* -- begin execute commande -- */

   int funct_accept, funct_done, offset, j ;
   register i ;
   register short *p_proto ;

D(D_REQ,printf("execut_cmd TOF_LOG %d\n",TOF_LOG);)
   if(TOF_LOG)
{
printf("\n exe ba= %x",ba);
printf("\n exe f_code= %x",f_code);
printf("\n exe par_nb= %x",par_nb);
printf("\n exe p1=%x p2=%x p3=%x p4=%x",p1,p2,p3,p4);
}
   if (f_code<F_MIN || f_code>F_MAX) 
     { if (TOF_LOG) printf("%s execut_cmd return code %d\n",
		    printt("TOF"),TOF_CMD_BAD) ;
       return TOF_CMD_BAD ;
     }
   p_proto=ba ;

   /*tof_wait();*/

   offset=PRO_OF_CHA/2 ;	    /* protocol offset channel card  */
   if (ba == (short *)T_BA)         /* time generator with dual port */
     { offset=0x80 ;		    /* protocol offset time generator*/
       *(p_proto+DPWR_O)=PG_COM ;   /* operation in word             */
     }
   switch (par_nb)
     { case 4:	*(p_proto+offset+6)=(short)p4 ;
       case 3:	*(p_proto+offset+5)=(short)p3 ;
       case 2:	*(p_proto+offset+4)=(short)p2 ;
       case 1:	*(p_proto+offset+3)=(short)p1 ;
       case 0:	break ;
       default:	tof_signal() ;
		if (TOF_LOG) printf("%s execut_cmd return code %d\n",
			     printt("TOF"),TOF_BADARG) ;
		return TOF_BADARG ;
     }
/* Hier wird nach Vorbereitungen das Commandword auf die Karte gegeben. */
    *(p_proto+offset+1)=*(p_proto+offset+1)&0x3FFF;
    *(p_proto+offset+2)=0 ;
    *(p_proto+offset+0)=(short)f_code ;

    if (ba==(short *)T_BA)
       *p_proto = 1 ;				 /* declench IT/flexipm      */
   funct_accept=FALSE ;
   funct_done  =FALSE ;
   j=100*WAIT_MAX ;				 /* wait time to 0.01 sec    */
   for ( i=0; i<j ;i++)
     { tsleep(2) ;				 /* wait 0.01s		     */
        if (*(p_proto+offset+1)&0x8000) funct_accept=TRUE ; 	/* accepted  */
        if (!(*(p_proto+offset)&0x00FF)) 
	  { funct_done =TRUE ;		 	/* executed  */
	    break ;
	  }
     }
   if(TOF_LOG)
      printf("\n end f. %x status %x",*(p_proto+offset),*(p_proto+offset+1));
   if (funct_done)
	    r = TOF_SUCC ;
   else
     { if (funct_accept)
	    r = TOF_TIM_OUT ;
       else
	    r = TOF_CMD_NACC ;
     }
   /*tof_signal() ;*/
   if (TOF_LOG) printf("%s execut_cmd return code %d\n",printt("TOF"),r) ;
   return r ;
}/* -- end execute commande -- */

/***************************************************************************/

start_acq(no)				/* start acq. on channel card only */
int no ;

{/* -- begin start acquisition -- */
D(D_REQ,printf("start_acq(%d)\n",no);)

        if (ptr_instcom->d_mask[no] == 0xFFFFFFFF) return 0 ;
	r = execut_cmd(d_ba[no],F_START_ACQ,0) ;
	return r ;

}/* -- end start acquisition -- */

/**************************************************************************/

stop_acq(no)			 	/* stop acq. on channel card only */
int no ;

{/* -- begin stop acquisition -- */
D(D_REQ,printf("stop_acq(%d)\n",no);)
        if (ptr_instcom->d_mask[no] == 0xFFFFFFFF) return 0 ;
	r = execut_cmd(d_ba[no],F_STOP_ACQ,0) ;
	return r ;

}/* -- end stop acquisistion -- */

/**************************************************************************/

write_param(ba,p1,p2,p3,p4)	/* write channel and timer parameters     */
unsigned ba;
unsigned p1,p2,p3,p4 ;

{/* -- begin write parameters -- */

	r = execut_cmd(ba,F_WRITE_PAR,4,p1,p2,p3,p4) ;
	return r ;

}/* -- end write parameters -- */

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

erase(ba, m_d_mask)

unsigned ba ;				/* base address VME module	*/
unsigned m_d_mask ;			/* mask detector or monitor 	*/

{/* -- begin erase memories -- */
  register add_mem ;
  register count, i ;
  int  words_nb, first ;

  words_nb=spc_length*w_length ;  	/* words per spectra		*/
  if (!MULTI_DET)			/* not multi detectors		*/
    { for (i=0, first=TRUE, count=0 ; i<32 ; i++)
        { if( !((m_d_mask>>i) & 1))
            { if (first)
                { add_mem=DEB_INCR_O*2+(words_nb*2*i) ;/* first channel addr */
	          first=FALSE ;
                }
              count +=words_nb ;
            }
          else if (!first)
            { r=execut_cmd(ba,F_ERASE_MEM,4,(add_mem>>16),add_mem,
                          (count>>16),count) ;
              count=0 ;
              first=TRUE ;
            }
         }
     }
  else                                         /* multi detector           */
     { count=xydet*words_nb ;          /* words nb to erase        */
       first=FALSE ;
       add_mem=DEB_INCR_O*2 ;
     }
  if (!first)
     { r=execut_cmd(ba,F_ERASE_MEM,4,(add_mem>>16),add_mem,
                    (count>>16),count) ;
     }
  return r;
}
/**************************************************************************/

read_data()
{
register long i,j,k,no;
short *grp ,*mapmod();
int  spc_nb ,bytes;

if (out==1)
{          /* initialize Link */
   if (init_x() != TR_SUCC) return -1;
   reset_x();
           /* get vax experiment parameter and grouping parameter */
   if (get_instcom() != TR_SUCC)
      { close_x() ;
        return -2 ;
      }
}
   for (i=0 ; i < DET_NB ; sums[i++]=0 );

   if ((out==1) && ptr_instcom->grp_spct)
      {         /*-------------------- Build grouped spectra */

        if (MULTI_DET && !ptr_instcom->d_mask[0]) return TOF_BADARG;
        if ((grp=mapmod("grouping",&bytes)) == (short *)ERROR || !grp)
           { close_x();
             return -3;
           }
        spc_nb=ptr_instcom->grp_spct ;
        for (j = 0 ; j < (ptr_instcom->grp_spct) ; j++)
           {
             if (TOF_LOG) printf("*** grouped spectrum %d\n",j+1); 
             for (i = 0 ; i < p_res ; buf[i++]=0 );
             for (i = 0 ; i < DET_NB ; i++ )
                {                    /* add grouped spectra */
                  if (grp[i] == j+1)
                    {    /* find matching detector spectra */
                      get_spct (i,raw) ;
                      add_spct (raw,buf,p_res) ;
                      if (!sums[i])
                          sums[i]=sum_spct (raw,p_res) ;
                    }
                }  
             if ( mask & R_DATA )
                { if (put_data(buf,p_res) != TR_SUCC) 
                     { close_x();
                       return -4;
                     }
                }
           }
        } 
   else
        {              /* --------------------------- Build raw spectra */   
          for (i=0 ; i<n_module ; i++)
             { spc_nb =ptr_instcom->n_det ;
               if (MULTI_DET)
                 { k=xydet ;
                   spc_nb=xydet ;
                   if (TOF_LOG) printf(" x=%d,y=%d,xy=%d,length=%d,w=%d\n"
                                       ,xdet,ydet,xydet,spc_length,w_length);
                 }
               else k=32 ;
              for (j = 0 ; j<k ; j++)
                { if (MULTI_DET) no=j ;
                  else no=i*32+j ;
                  get_spct(no,buf) ;
                  sums[no]=sum_spct (buf,spc_length) ;
                  if (TOF_LOG || !out)
                    { if (mask & R_DATA)
                        { printf("\n **** Raw spectrum %d sum %d\n",
                                 no+1,sums[no]);
                          if (sums[no]) printb('l',buf,spc_length);
                        }
                    }
                  if (out)
                    { if (mask & R_DATA)
                        { if (put_data(buf,spc_length) != TR_SUCC)
                            { close_x();
                              return -5;
                            }
                        }
                    }
                }
            }
        }
if ((mask | SUM) && !out)                   /* print sums   */
      printb('l',sums,spc_nb) ;
/* data read was successful: */
/*   read_trigger(T_BA); */
   if (out)   
      { ptr_instcom->trigger=trigger_value ;

        /* flush byte buffer and  transfer detector sums 
           flush byte buffer and transfer os9 results */

           if (put_data (0,0) == TR_SUCC)
        /* if (put_data(sums,spc_nb)  == TR_SUCC) no transmit */
           if (put_data (0,0) == TR_SUCC)   put_instcom();
           close_x();
      } 
   return 0;
}
/************************** get_spct ************************************/
get_spct(no, lbuf) 
register long *lbuf ;
int no ;
{
short *ptr_mem ,*ba;
int mod_no=0 ,spc_no ;
register int k ;
union d_channel{ short tab[2]; long info[1] }ch ;

spc_no=no;
ba=(short *)d_ba[mod_no];
ptr_mem=ba+DEB_INCR_O+(spc_length*w_length*spc_no); /* first channel @ */
if (!MULTI_DET && (ptr_instcom->d_mask[mod_no]>>spc_no & 1))
   { for (k=0; k<p_res; k++, *lbuf++=0) ;
   }
else
   { for (k=0; k<spc_length; k++)
     { if (w_length==1)
          ch.tab[0]=0;			/* channel on 16bits, high=0 */
       else                             /*    "       32  "          */
          ch.tab[0]=*ptr_mem++ ;        /* high                      */
                                        /* for 16 & 32 bits          */
          ch.tab[1]=*ptr_mem++ ; 	/* low                       */
          *lbuf++=ch.info[0];
     }
   }
return 0 ;
}
/************************** sum_spct ************************************/
sum_spct (lbuf,size) 
register unsigned long *lbuf,size;
{
register unsigned long lsum;
lsum=0 ;
while(size--) lsum += *lbuf++;
return lsum ;
}
/************************** add_spct ************************************/
add_spct (src,dest,size) 
register unsigned long *src,*dest,size;
{
while(size--) *dest++ += *src++ ;
return 0 ;
}
toto()
{return -1;
}
