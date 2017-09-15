#define CAMACinc(a) a=CAMACincFunc(a)
#define CAMACreadX(a) CAMACread(a)
#define CAMACaddr(n,a,f) ( ((a)<<8) | ((n)<<16) | ((f)<<24) )

typedef int camadr_t;

static int BRANCH = 0;
