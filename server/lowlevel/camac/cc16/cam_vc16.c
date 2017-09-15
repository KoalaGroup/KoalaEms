/*

cam_vc16.c
********************************************************************
               CC16 / VC 16 C routines
 ----------------------------------------------------------------
  Unit contains :
    cam_adr         	- set I/O address (BADR)
    getstatus     	- get CC16 status
    cam_cratecheck	- test crate available
    set_crate     	- set geographical crate address
    set_delay        - set value for cable delay (4 bits)
    cam_controller_ini - controller initialization
    cam_z         	- CAMAC Initialize (Z)
    cam_c         	- CAMAC Clear (C)
    cam_i         	- CAMAC Inhibit (I)
    cam_ci        	- CAMAC Clear + Inhibit (I) setzen
    cam_irq       	- enable CAMAC LAM-Interrupt
    cam_noirq     	- disable CAMAC LAM-Interrupt
    cam_q         	- get CAMAC Q-response
    cam_x        	- get CAMAC X-response
    cam_nfa       	- set N,F,A start CAMAC cycle
    cam_nfa_read  	- set N,F,A and read 2 byte
    cam_nfaqx_read  	- set N,F,A and read 2 byte with Q and X
    cam_nfaqx_read24 - set N,F,A and read 3 byte with Q and X
    cam_nfa_write 	- set N,F,A and write 2 byte
    cam_nfaqx_write 	- set N,F,A and write 2 byte with Q and X
    cam_nfaqx_write24- set N,F,A and write 2 byte with Q and X
    cam_lam       	- get LAM source
    cam_lamf      	- any LAM request

    badr := basic I/O address
    copyright:    A. Ruben, W-Ie-Ne-R, PleinBaus GmbH  20.03.95
********************************************************************

CPU dependent register access has to be defined in unit reg_acc.h

*/
 
#include "reg_acc.c"

unsigned int nfa;
unsigned long int badr;
unsigned long int badr2;
unsigned long int badr4;
unsigned long int badr6;
unsigned long int badr8;
unsigned long int badrA;
unsigned long int badrC;

void cam_adr(unsigned long int);
int  cam_cratecheck (void);
void set_crate (int);
void set_delay (int);
void cam_controller_ini (unsigned long int);
int  getstatus(void);
void cam_z(void);
void cam_c(void);
void cam_i(void);
void cam_ci(void);
void cam_0(void);
int  cam_q(void);
int  cam_x(void);
void cam_irq(void);
void cam_noirq(void);
void cam_nfa(int, int, int);
void cam_nfa_write(int, int, int, unsigned int);
void cam_nfaqx_write(int, int, int, unsigned int, int *, int *);
void cam_nfaqx_write24(int, int, int, long int, int *, int *);
unsigned int  cam_nfa_read(int, int, int);
unsigned int  cam_nfaqx_read(int, int, int, int *, int *);
long int cam_nfaqx_read24(int, int, int, int *, int *);

/* **************************************************************************} */

void cam_adr(unsigned long int bad)
{
  badr=bad;
  badr2=badr+2;
  badr4=badr+4;
  badr6=badr+6;
  badr8=badr+8;
  badrA=badr+10;
  badrC=badr+12;
};
/* **************************************************************************} */

int  cam_cratecheck (void)
{
  int j;
  put_reg(badr8,0);
  put_reg(badr2,7);
  put_reg(badr4,0);
  j=get_reg(badr6);
  j=(j & 4) - 4;
  if (j<0)
  {
   j=1;
  }
  return(j);
};
/* **************************************************************************} */

void  set_crate (int crate)
{
  put_reg(badr8,crate);
};
/* **************************************************************************} */

void set_delay (int delay_bits)
{
  unsigned int i;
  i=get_reg(badr);
  i=i & 0x0ff;
  i=i | (delay_bits << 8);
  put_reg(badr,i);
}
/* **************************************************************************} */

void cam_controller_ini (unsigned long int bad)
{
  vme_init();
  cam_adr(bad);
  set_crate(0);
  cam_z();
};
/* **************************************************************************} */

int getstatus(void)
{
  char j;
  j=0;
  return(j & 7);
};
/* ***********************************************************************} */

void cam_z(void)
{
  put_reg(badr2,11);
  put_reg(badr2,9);
  put_reg(badr2,6);
};
/* **************************************************************************} */

void cam_c(void)
{
  put_reg(badr2,11);
  put_reg(badr2,8);
  put_reg(badr2,6);
};
/* *************************************************************************} */

void cam_i(void)
{
  put_reg(badr2,10);
};
/* ************************************************************************} */

void cam_ci(void)
{
  put_reg(badr2,10);
  put_reg(badr2,8);
  put_reg(badr2,6);
};
/* *************************************************************************} */

int cam_q(void)
{
  char j;
  j=get_reg(badr);
  return ( j >>6) & 1;
};
/* ************************************************************************} */

int cam_x(void)
{
  char j;
  j=get_reg(badr);
  return ( j >> 5) & 1;
};
/* ************************************************************************} */

void cam_irq(void)
{
  put_reg(badr2,12);
};
/* *************************************************************************} */

void cam_noirq(void)
{
  put_reg(badr2,13);
};

/* *************************************************************************} */

void cam_nfa(int n, int f, int a)
{
  nfa= a << 5;
  nfa= (nfa | f) << 5;
  nfa= nfa | (n+0x8000);
  put_reg(badr2,0);
  put_reg(badr4,nfa);
};
/* *************************************************************************} */

void cam_nfa_write(int n, int f, int a, unsigned int data)
{
  nfa= a << 5;
  nfa= (nfa | f) << 5;
  nfa= nfa | n;
  put_reg(badr2,0);
  put_reg(badr4,nfa);
  put_reg(badr2,2);
  put_reg(badr4,data);
};
/* *************************************************************************} */

void cam_nfaqx_write(int n, int f, int a, unsigned int data,int *q, int *x)
{
  char j;
  nfa= a << 5;
  nfa= (nfa | f) << 5;
  nfa= nfa | n;
	put_reg(badr2,0);
	put_reg(badr4,nfa);
	put_reg(badr2,2);
	put_reg(badr4,data);
	j=get_reg(badr);
	*q=( j >>6 ) & 1;
	*x=( j >>5 ) & 1;
};
/* *************************************************************************} */

void cam_nfaqx_write24(int n, int f, int a, long int data,int *q, int *x)
{
  char j;
  int ih;
  int il;
  ih= (int) ((data >> 16) & 255);
  il= (int) data;
  nfa= a << 5;
  nfa= (nfa | f) << 5;
  nfa= nfa | n;
	put_reg(badr2,0);
	put_reg(badr4,nfa);
	put_reg(badr2,1);
	put_reg(badr4,ih);
	put_reg(badr2,2);
	put_reg(badr4,il);
	j=get_reg(badr);
	*q=( j >>6 ) & 1;
	*x=( j >>5 ) & 1;
};
/* *************************************************************************} */

unsigned int cam_nfa_read(int n, int f, int a)
{
  nfa= a << 5;
  nfa= (nfa | f) << 5;
  nfa= nfa | (n+0x8000);
	put_reg(badr2,0);
	put_reg(badr4,nfa);
	put_reg(badr2,3);
	return ((unsigned int) get_reg(badr6));
};
/* *************************************************************************} */

unsigned int cam_nfaqx_read(int n, int f, int a, int *q, int *x)
{
  char j;
  nfa= a << 5;
  nfa= (nfa | f) << 5;
  nfa= nfa | (n+0x8000);
	put_reg(badr2,0);
	put_reg(badr4,nfa);
	put_reg(badr2,3);
	nfa= (unsigned int) get_reg(badr6);
	put_reg(badr,3);
  nfa=(unsigned int) get_reg(badr4);
	j=get_reg(badr);
	*q=( j >>6 ) & 1;
	*x=( j >>5 ) & 1;
	return (nfa);
};
/* *************************************************************************} */

long int cam_nfaqx_read24(int n, int f, int a, int *q, int *x)
{
  char j;
  unsigned long lh;
  nfa= a << 5;
  nfa= (nfa | f) << 5;
  nfa= nfa | (n+0x8000);
	put_reg(badr2,0);
	put_reg(badr4,nfa);
	put_reg(badr2,4);
	lh=get_reg(badr6) & 255;
	lh=lh <<16;
	put_reg(badr2,3);
	lh += get_reg(badr6)&0xffff;
	j=get_reg(badr);
	*q=( j >>6 ) & 1;
	*x=( j >>5 ) & 1;
	return (lh);
};
/* *************************************************************************} */

int cam_lam(int station)
{
  cam_nfa(station,8,0);
  return (cam_q());
};
/* *************************************************************************} */

int cam_lamf(void)
{
  int j;
  return(j & 2);
};
