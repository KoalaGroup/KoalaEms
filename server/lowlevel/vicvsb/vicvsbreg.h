struct viccsr{
    int reg,val;
};

#define READCSR _IOWR('V',2,struct viccsr)
#define WRITECSR _IOW('V',3,struct viccsr)
#define SENDINT _IOW('V',5,int)
#define GETVICSPACE _IOR('V',6,int)
