#ifndef _wincc_h_
#define _wincc_h_

#include <stdio.h>
#include <errorcodes.h>

typedef enum
  {
  WCC_TBIT   =1, /* int */
  WCC_TBYTE  =2, /* int */
  WCC_TCHAR  =3, /* string */
  WCC_TDOUBLE=4, /* double */
  WCC_TFLOAT =5, /* float */
  WCC_TWORD  =6, /* int */
  WCC_TDWORD =7, /* int */
  WCC_TRAW   =8, /* opaque */
  WCC_TSBYTE =9, /* int */
  WCC_TSWORD =10 /* int */
  } wincc_types;

typedef enum {WCC_REQ=1, WCC_RES=2} wincc_commands;
typedef enum
  {
  WCC_USER_ERR=-3,   /* private definition for ems_server */
  WCC_NO_PATH=-2,    /* private definition for ems_server */
  WCC_SYSTEM=-1,     /* private definition for ems_server */
  WCC_OK=0,
  WCC_PROT_ERR=0x11,
  WCC_NO_ACCESS=0x12
  } wincc_errors;
typedef enum {WCC_RD=1, WCC_WR=2} wincc_operations;
typedef enum
  {
  WCC_V_OK=0,
  WCC_V_ILLEGAL_OP=256,
  WCC_V_NOT_EXISTENT=2048,
  WCC_V_CHANNEL_ERROR=257
  } wincc_verrors;

typedef struct
  {
  wincc_types typ;
  union
    {
    int i;
    float f;
    double d;
    struct
      {
      int size;
      char* vals;
      } opaque;
    char* str;
    } val;
  } wincc_value;

typedef struct
  {
  char* name;
  wincc_operations operation;
  wincc_verrors error;
  wincc_value value;
  } wincc_var_spec;

errcode wincc_low_init(char*);
errcode wincc_low_done(void);
int wincc_low_printuse(FILE*);

plerrcode wincc_open(int* path, char* hostname, int port);
plerrcode wincc_close(int path);
wincc_errors wincc_var(int path, int num, wincc_var_spec* var_specs, int async);

#endif
