typedef int camadr_t;

camadr_t CAMACincFunc(camadr_t);

camadr_t CAMACaddr(int,int,int);
int CAMACread(camadr_t);
int CAMACreadX(camadr_t);
void CAMACcntl(camadr_t);
void CAMACwrite(camadr_t, int);
int CAMACstatus(void);
void CCCC(void);
void CCCZ(void);
void CCCI(int);
int CAMAClams(void);
#define CAMACinc(a) a=CAMACincFunc(a)

int * CAMACblread(int *, camadr_t, int);
int * CAMACblreadAddrScan(int *, camadr_t, int);
int * CAMACblreadQstop(int *, camadr_t, int);
int CAMACgotQ(int);
int * CAMACblwrite(camadr_t, int *, int);
int * CAMACblread16(int *, camadr_t, int);
