#ifndef __hbook_h
#define __hbook_h

/* used in xd2paw   - limit 512*1024      */
#define  PAWCSIZE   524288    
/* used in xd2hbook - unknown limitations */
#define  HLIMIT    10*1024*1024
#define HLen 81

__BEGIN_DECLS
extern void   hbook1_(int *id, char *name, int *nx, float *xmin, float *xmax, float *v, int namelen);
extern void   hbook2_(int *id, char *name, int *nx, float *xmin, float *xmax, 	       
	                            int *ny, float *ymin, float *ymax, float *v, int namelen);

extern void   hdelet_(int* id);
extern int    hexist_(int* id);

extern void   hfill_(int* id, float* , float* , float* );

extern void   hf1_(int* , float* , float* );
extern void   hf2_(int* , float* , float* , float*);

extern float  hi_  (int*, int*);
extern float  hij_ (int*, int*, int*);

extern void   hlimap_(int *, char *,int);
extern void   hlimit_(int *);

extern void   hpak_  (int*, float* );
extern void   hrput_ (int*, char*, char*,int,int);
extern void   hrget_ (int*, char*, char*,int,int);

extern void   hreset_ (int*, char*,int);

/* made by Adam */
extern void   hmdir_ (char *dir, char *opt, int, int);
extern void   hcdir_ (char *dir, char *opt, int, int);
extern void   hopera_(int *id1,char*oper,int *id2,int *id3,float *f1,float *f2, int);
/* made by Peter */
extern void hropen_(int* lun, char* top, char* file, char* opt, int* lrec,
    int* istat, int, int, int);
extern void hrend_(char* top, int);
extern void hrin_(int* id, int* icycle, int* iofset);
extern void hrout_(int* id, int* icycle, char* opt, int);
extern void hidopt_(int* id, char* opt, int);
extern void hid1_(int *idvect, int* n);
extern void hid2_(int *idvect, int* n);
extern void hlnext_(int* idh, char* chtype, char* chtitl, char* chopt, int,
    int, int);
/* -----------------------------------------------------------------------*/
/* new functions defined from 'c' */

extern void initHbook();

extern void Hbook1(int id, char* title, int nx, float xmin, float xmax,  float vmx);

extern void Hbook2(int id, char* title, int nx, float xmin, float xmax,  
                                int ny, float ymin, float ymax,  float vmx);

extern void Hrput(int id, char* fname, char* chopt);
extern void Hrget(int id, char* fname, char* chopt);

extern void Hfill(int id, float x, float y, float w);

extern void Hreset(int id, char* title);

extern void Hnoent(int id, int *noent);

void Hgive(int id, char* chtitl, int *nx, float *xmi, float *xma, int *ny, float *ymi, float *yma,int *nwt, int *loc);

float Hi(int id, int i);

float Hij(int id, int i, int j);
 
float Hstati(int id, int icase, char* choice, int num);

void Hfithn(int id,char* chfun,char* chopt,int np,float par[],float step[],float pmin[],float pmax[],float sigpar[],float *chi2);

/* made by Adam */
extern void Hmdir(char* dir, char *opt);
extern void Hcdir(char* dir, char *opt);
extern void Hopera(int id1, char *oper, int id2, int id3, float f1, float f2);
/* made by Peter */
extern void Hropen(int lun, char* top, char* file, char* opt, int* lrec,
    int* istat);
extern void Hrend(char* top);
extern void Hrin(int id, int icycle, int iofset);
extern void Hrout(int id, int* icycle, char* opt);
extern void Hidopt(int id, char* opt);
extern void Hid1(int *idvect, int* n);
extern void Hid2(int *idvect, int* n);
extern void Hlnext(int* idh, char* chtype, char* chtitl, char* chopt);
extern void Hdelet(int id);
__END_DECLS

#endif
