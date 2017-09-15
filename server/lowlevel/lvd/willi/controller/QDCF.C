// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	qdcf.c -- 16 channel fast QDC */

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

#define QDCF
#define _XF		2
#define _NS		6.25
#define _NSF	2
#define _MV		0.2
#define _FG		2

extern FQDC_BC		*inp_bc;
extern FQDC_RG		*inp_unit;

#include "qdc.c"
