// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	qdcs.c -- 16 channel slow QDC */

#include <stddef.h>		// offsetof()
#include <dos.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <errno.h>
#include "pci.h"
#include "daq.h"
#include "seq.h"

#define QDCS
#define _XF		1
#define _NS		12.5
#define _NSF	1
#define _MV		0.5
#define _FG		3

extern SQDC_BC		*inp_bc;
extern SQDC_RG		*inp_unit;

#include "qdc.c"
