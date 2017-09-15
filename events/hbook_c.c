/* routines operating on HBOOK (PAW) files */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "hbook_c.h"
  
int pawc_[HLIMIT]; /* stupid fortran COMMON/PAWC/ */

/***************************************************************************/

void initHbook()
{
int limit = HLIMIT; 
hlimit_(&limit);
return;
}
  
/***************************************************************************/
void Hbook1(int id, char* title, int nx, float xmin, float xmax,  float vmx)
{  
hbook1_(&id, title, &nx, &xmin, &xmax, &vmx, strlen(title));
return;
}

  
/***************************************************************************/
void Hbook2(int id, char* title, int nx, float xmin, float xmax,  
                                int ny, float ymin, float ymax,  float vmx)
{
hbook2_(&id,title,&nx,&xmin,&xmax,&ny,&ymin,&ymax,&vmx,strlen(title));
return; 
}

  
/***************************************************************************/
void Hrget(int id, char* fname, char* chopt)
{  
hrget_(&id,fname,chopt,strlen(fname),sizeof(chopt)); 
return;
}
  
/***************************************************************************/
void Hrput(int id, char* fname, char* chopt)
{  
hrput_(&id,fname,chopt,strlen(fname),sizeof(chopt)); 
  
{ /* print time information */
  struct tm *t;
  time_t ttt;
    
  time(&ttt);
  t=localtime(&ttt);

  fprintf(stderr,"         Hbook saved to  file=\"%s\" at %02d:%02d:%02d \n",
    fname, t->tm_hour, t->tm_min ,t->tm_sec); 
  }
return;
}
  
/***************************************************************************/
void Hfill(int id, float x, float y, float w)
{
hfill_(&id,&x,&y,&w); 
return;
}


/***************************************************************************/
void Hreset(int id, char* fname)
{  
hreset_(&id,fname,strlen(fname)); 
return;
}
 
/***************************************************************************/
void Hnoent(int id, int *noent)
{
hnoent_(&id,noent);
return;
}

/************************************************************************************************************************/
void Hgive(int id, char* chtitl, int *nx, float *xmi, float *xma, int *ny,
    float *ymi, float *yma, int *nwt, int *loc)
{
int i;
hgive_(&id,chtitl,nx,xmi,xma,ny,ymi,yma,nwt,loc,HLen-1);
chtitl[HLen-1]=0;
for (i=HLen-2; (i>=0) && (chtitl[i]==' '); i--) chtitl[i]=0;
return;
}

/***************************************************************************/
float Hi(int id, int i)
{
return hi_(&id,&i);
}

/***************************************************************************/
float Hij(int id, int i, int j)
{
return hij_(&id,&i,&j);
}

/***************************************************************************/
float Hstati(int id, int icase, char* choice, int num)
{
return hstati_(&id,&icase,choice,&num,strlen(choice));
}

/********************************************************************/
void Hfithn(int id, char* chfun, char* chopt, int np, float par[],
    float step[], float pmin[], float pmax[], float sigpar[], float *chi2)
{
hfithn_(&id, chfun, chopt, &np, par, step, pmin, pmax, sigpar, chi2,
    strlen(chfun),strlen(chopt));
return;
}

/********************************************************************/

/***************************************************************************/
/* made by Adam */
void Hmdir(char* name, char* opt)
{  
hmdir_(name, opt, strlen(name), strlen(opt));
return;
}

/***************************************************************************/
/* made by Adam */
/*
 * void Hcdir(char* name, char* opt)
 * {  
 *   char  fortName[f77NameLen+1], fortOpt[f77NameLen+1];
 * 
 *   strncpy(fortName,name,f77NameLen);
 *   fortName[f77NameLen]=0;  
 * 
 *   strncpy(fortOpt,opt,f77NameLen); 
 *   fortOpt[f77NameLen]=0;  
 * 
 *   hcdir_(fortName, fortOpt, strlen(fortName), strlen(fortOpt));
 * 
 *   if(hbtr1)fprintf(stderr," hcdir, dir:%s option:%s\n",fortName, fortOpt);
 *   return;
 * }
 */

/***************************************************************************/
void Hopera(int id1, char* oper, int id2, int id3, float f1, float f2)
/* made by Adam */
{  
hopera_(&id1, oper, &id2, &id3, &f1, &f2, strlen(oper));
return;
}

/***************************************************************************/
/* made by Peter */

void Hropen(int lun, char* top, char* file, char* opt, int* lrec, int* istat)
{
hropen_(&lun, top, file, opt, lrec, istat, strlen(top), strlen(file),
    strlen(opt));
}

/***************************************************************************/

void Hrend(char* top)
{
hrend_(top, strlen(top));
}

/***************************************************************************/

void Hrin(int id, int icycle, int iofset)
{
hrin_(&id, &icycle, &iofset);
}

/***************************************************************************/

void Hrout(int id, int* icycle, char* opt)
{
hrout_(&id, icycle, opt, strlen(opt));
}

/***************************************************************************/

void Hidopt(int id, char* opt)
{
hidopt_(&id, opt, strlen(opt));
}

/***************************************************************************/

void Hid1(int *idvect, int* n)
{
hid1_(idvect, n);
}

/***************************************************************************/

void Hid2(int *idvect, int* n)
{
hid2_(idvect, n);
}

/***************************************************************************/

void Hcdir(char* chpath, char* chopt)
{
int i;
for (i=strlen(chpath); i<HLen; i++) chpath[i]=' ';
hcdir_(chpath, chopt, HLen-1, strlen(chopt));
chpath[HLen-1]=0;
for (i=HLen-2; (i>=0) && (chpath[i]==' '); i--) chpath[i]=0;
}

/***************************************************************************/

void Hlnext(int* idh, char* chtype, char* chtitl, char* chopt)
{
int i;
hlnext_(idh, chtype, chtitl, chopt, HLen-1, HLen-1, strlen(chopt));
chtype[HLen-1]=0;
for (i=HLen-2; (i>=0) && (chtype[i]==' '); i--) chtype[i]=0;
chtitl[HLen-1]=0;
for (i=HLen-2; (i>=0) && (chtitl[i]==' '); i--) chtitl[i]=0;
}

/***************************************************************************/

void Hdelet(int id)
{
hdelet_(&id);
}
/***************************************************************************/
/***************************************************************************/
