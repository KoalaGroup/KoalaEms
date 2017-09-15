/******************************************************************************
*                                                                             *
* kernel.c                                                                    *
*                                                                             *
* last changed: 23.09.94                                                      *
*                                                                             *
******************************************************************************/
/*
  C BASED FORTH-83 MULTI-TASKING KERNEL

  Copyright (C) 1988-1990 by Mikael R.K. Patel

  Computer Aided Design Laboratory (CADLAB)
  Department of Computer and Information Science
  Linkoping University
  S-581 83 LINKOPING
  SWEDEN

  Email: mip@ida.liu.se

  Started on: 30 June 1988

  Dependencies:
        (cc) kernel.h, error.h, memory.h, io.c, compiler.v,
             locals.v, string.v, float.v, memory.v, queues.v,
             multi-tasking.v, exceptions.v

  Description:
       Virtual Forth machine and kernel code supporting multi-tasking of
       light weight processes. A pure 32-bit Forth-83 Standard implementation.

       Extended with floating point numbers, argument binding and local
       variables, exception handling, queue data management, multi-tasking,
       symbol hiding and casting, forwarding, null terminated string,
       memory allocation, file search paths, and source library module
       loading.

  Note:
       The kernel does not implement the block word set. All code is
       stored as text files.

  Copying:
       This program is free software; you can redistribute it and/or modify
       it under the terms of the GNU General Public License as published by
       the Free Software Foundation; either version 1, or (at your option)
       any later version.

       This program is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
       GNU General Public License for more details.

       You should have received a copy of the GNU General Public License
       along with this program; see the file COPYING.  If not, write to
       the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <errno.h>
#include "kernel.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "in.h"
#include "out.h"

/* EXTERNAL DECLARATIONS */

extern VOID io_dispatch();


/* INTERNAL FORWARD DECLARATIONS */

extern code_entry qnumber;
extern code_entry terminate;
extern code_entry abort_entry;
extern entry toexception;
extern entry span;
extern entry state;
extern code_entry vocabulary;


/* VOCABULARY LISTING PARAMETERS */

#define COLUMNWIDTH 15
#define LINEWIDTH 75


/* CONTROL STRUCTURE MARKERS */

#define ELSE 1
#define THEN 2
#define AGAIN 4
#define UNTIL 8
#define WHILE 16
#define REPEAT 32
#define LOOP 64
#define PLUSLOOP 128
#define OF 256
#define ENDOF 512
#define ENDCASE 1024
#define SEMICOLON 2048


/* MULTI-TASKING MACHINE REGISTERS */

INT32 verbose;                  /* Application or programming mode */
INT32 quited;                   /* Interpreter toploop state */
INT32 running;                  /* Task switch flag */
INT32 tasking;                  /* Multi-tasking flag */

TASK tp;                        /* Task pointer */
TASK foreground;                /* Foreground task pointer */


/* FORTH MACHINE REGISTERS */

PTR sp;                         /* Parameter stack pointer */
PTR s0;                         /* Bottom of parameter stack pointer */

PTR32 ip;                       /* Instruction pointer */
PTR32 rp;                       /* Return stack pointer */
PTR32 r0;                       /* Bottom of return stack pointer */

PTR32 fp;                       /* Argument frame pointer */
PTR32 ep;                       /* Exception frame pointer */


/* VOCABULARY SEARCH LISTS */

#define CONTEXTSIZE 64

static VOCABULARY_ENTRY current = &forth;
static VOCABULARY_ENTRY context[CONTEXTSIZE] = {&forth};


/* ENTRY LOOKUP CACHE, SIZE AND HASH FUNCTION */

#define CACHESIZE 256
#define hash(s) ((s[0] + (s[1] << 4)) & (CACHESIZE - 1))

static ENTRY cache[CACHESIZE];

/* DICTIONARY AREA FOR THREADED CODE AND DATA */

PTR32 dictionary;
PTR32 dp;


/* INTERNAL STRUCTURE AND SIZES */

static INT32 hld;
static ENTRY thelast = NIL;

#define PADSIZE 84
static CHAR thepad[PADSIZE];

#define TIBSIZE 256
static CHAR thetib[TIBSIZE];

/* CASTING IN INTERPRET TOP-LOOP */

#define CASTING

/* INNER MULTI-TASKING FORTH VIRTUAL MACHINE */

VOID doinner()
{
INT32 e;

/* Exception marking and handler */
if (e = setjmp(restart))
  {
  spush(e, INT32);
  doraise();
  }

/* Run virtual machine until task switch */
running = TRUE;
while (running)
  {
  /* Fetch next thread to execute */
  register ENTRY p = (ENTRY) *ip++;

  /* Select on type of entry */
  switch (p -> code)
    {
    case CODE:
      ((SUBR) (p -> parameter))();
      break;
    case COLON:
      rpush(ip);
      fjump(p -> parameter);
      break;
    case VARIABLE:
      spush(&(p -> parameter), PTR32);
      break;
    case CONSTANT:
      spush(p -> parameter, INT32);
      break;
    case VOCABULARY:
      doappend((VOCABULARY_ENTRY) p);
      break;
    case CREATE:
      spush(p -> parameter, INT32);
      break;
    case USER:
      spush(((INT32) tp) + p -> parameter, INT32);
      break;
    case LOCAL:
      spush(*((PTR32) (INT32) fp - p -> parameter), INT32);
      break;
    case FORWARD:
      if (p -> parameter)
        docall((ENTRY) p -> parameter);
      else
        {
        sprintf(s_err, "%s%s: unresolved forward entry\n", input_info,
            p -> name);
        emit_err();
        doabort();
        }
      break;
    case EXCEPTION:
      spush(p, ENTRY);
      break;
    case FIELD:
      unary(p -> parameter +, INT32);
      break;
    default: /* DOES: FORTH LEVEL INTERPRETATION */
      rpush(ip);
      spush(p -> parameter, INT32);
      fjump(p -> code);
      break;
    }
  }
}

VOID docommand()
{
INT32 e;

/* Exception marking and handler */
if (e = setjmp(restart))
  {
  spush(e, INT32);
  doraise();
  return;
  }

/* Execute command on top of stack */
doexecute();

/* Check if this affects the virtual machine */
if (rp != r0)
  {
  tasking = TRUE;

  /* Run the virtual machine and allow user extension */
  while (tasking)
    {
    doinner();
    io_dispatch();
    }
  }
}

VOID docall(ENTRY p)
{
/* Select on type of entry */
switch (p -> code)
  {
  case CODE:
    ((SUBR) (p -> parameter))();
    return;
  case COLON:
    rpush(ip);
    fjump(p -> parameter);
    return;
  case VARIABLE:
    spush(&(p -> parameter), PTR32);
    return;
  case CONSTANT:
    spush(p -> parameter, INT32);
    return;
  case VOCABULARY:
    doappend((VOCABULARY_ENTRY) p);
    return;
  case CREATE:
    spush(p -> parameter, INT32);
    return;
  case USER:
    spush(((INT32) tp) + p -> parameter, INT32);
    return;
  case LOCAL:
    spush(*((PTR32) (INT32) fp - p -> parameter), INT32);
    return;
  case FORWARD:
    if (p -> parameter)
      docall((ENTRY) p -> parameter);
    else
      {
      sprintf(s_err, "%s%s: unresolved forward entry\n", input_info(),
          p -> name);
      emit_err();
      doabort();
      }
    return;
  case EXCEPTION:
    spush(p, ENTRY);
    return;
  case FIELD:
    unary(p -> parameter +, INT32);
    return;
  default: /* DOES: FORTH LEVEL INTERPRETATION */
    rpush(ip);
    spush(p -> parameter, INT32);
    fjump(p -> code);
    return;
  }
}

VOID doappend(VOCABULARY_ENTRY p)
{
INT32 v;

/* Flush the entry cache */
spush(FALSE, BOOL);
dorestore();

/* Check if the vocabulary is a member of the current search set */
for (v = 0; v < CONTEXTSIZE; v++)

  /* If a member then rotate the vocabulary first */
  if (p == context[v])
    {
    for (; v; v--) context[v] = context[v - 1];
    context[0] = p;
    return;
    }

/* If not a member, then insert first into the search set */
for (v = CONTEXTSIZE - 1; v > 0; v--) context[v] = context[v - 1];
context[0] = p;
}


/* VOCABULARY ROOT AND EXTERNAL VOCABULARIES */

vocabulary_entry forth = {NIL, "forth", NORMAL, VOCABULARY,
    (ENTRY) &vocabulary, (ENTRY) &qnumber};


/* COMPILER EXTENSIONS */

#include "compiler.v.c"

NORMAL_VOCABULARY(compiler, forth, "compiler", &backwardresolve, NIL)


/* LOCAL VARIABLES AND ARGUMENT BINDING */

#include "locals.v.c"

NORMAL_VOCABULARY(locals, compiler, "locals", &curlebracket, NIL)


/* NULL TERMINATED STRING */

#include "string.v.c"

NORMAL_VOCABULARY(string, locals, "string", &sprint, NIL)


/* FLOATING POINT */

#include "float.v.c"

NORMAL_VOCABULARY(float_entry, string, "float", &qfloat, &qfloat)


/* MEMORY MANAGEMENT */

#include "memory.v.c"

NORMAL_VOCABULARY(memory, float_entry, "memory", &free_entry, NIL)


/* DOUBLE LINKED LISTS */

#include "queues.v.c"

NORMAL_VOCABULARY(queues, memory, "queues", &dequeue, NIL)


/* MULTI-TASKING EXTENSIONS */

#include "multi_tasking.v.c"

NORMAL_VOCABULARY(multitasking, queues, "multi-tasking", &terminate, NIL)


/* SIGNAL AND EXCEPTION MANAGEMENT */

#include "exceptions.v.c"

NORMAL_VOCABULARY(exceptions, multitasking, "exceptions", &raise, NIL)

/* local procedures, requests, ... */

#include "xdrstring.v.c"

NORMAL_VOCABULARY(v_xdrstring, exceptions, "xdrstring", &xdrstring, NIL)

#include "procedures.v.c"

NORMAL_VOCABULARY(procedures, v_xdrstring, "v_server", &doterrcode, NIL)

/* LOGIC: FORTH-83 VOCABULARY */

NORMAL_CONSTANT(false, procedures, "false", FALSE)

NORMAL_CONSTANT(true, false, "true", TRUE)

VOID doboolean()
{
compare(!= 0, INT32);
}

NORMAL_CODE(boolean, true, "boolean", doboolean)

VOID donot()
{
unary(~, INT32);
}

NORMAL_CODE(not, boolean, "not", donot)

VOID doand()
{
binary(&, INT32);
}

NORMAL_CODE(and, not, "and", doand)

VOID door()
{
binary(|, INT32);
}

NORMAL_CODE(or, and, "or", door)

VOID doxor()
{
binary(^, INT32);
}

NORMAL_CODE(xor, or, "xor", doxor)

VOID doqwithin()
{
register INT32 value;
register INT32 upper;
register INT32 lower;

upper = spop(INT32);
lower = spop(INT32);
value = spop(INT32);

spush((value > upper) || (value < lower) ? FALSE : TRUE, BOOL);
}

NORMAL_CODE(qwithin, xor, "?within", doqwithin)


/* STACK MANIPULATION */

VOID dodepth()
{
register INT32 t;

t = s0-sp;
spush(t, INT32);
}

NORMAL_CODE(depth, qwithin, "depth", dodepth)

VOID dodrop()
{
sdrop();
}

NORMAL_CODE(drop, depth, "drop", dodrop)

VOID donip()
{
snip();
}

NORMAL_CODE(nip, drop, "nip", donip)

VOID doswap()
{
sswap();
}

NORMAL_CODE(_swap, nip, "swap", doswap)

VOID dorot()
{
srot();
}

NORMAL_CODE(rot, _swap, "rot", dorot)

VOID dodashrot()
{
sdashrot();
}

NORMAL_CODE(dashrot, rot, "-rot", dodashrot)

VOID doroll()
{
register UNIV e;
register PTR s;

/* Fetch roll parameters: number and element */
e = snth(sp->INT32);

/* Roll the stack */
for (s = sp + sp->INT32; s > sp; s--) *s = *(s - 1);
sp++;

/* And assign the new top of stack */
*sp = e;
}

NORMAL_CODE(roll, dashrot, "roll", doroll)

VOID doqdup()
{
if (sp->INT32) sdup();
}

NORMAL_CODE(qdup, roll, "?dup", doqdup)

VOID dodup()
{
sdup();
}

NORMAL_CODE(dup_entry, qdup, "dup", dodup)

VOID doover()
{
sover();
}

NORMAL_CODE(over, dup_entry, "over", doover)

VOID dotuck()
{
stuck();
}

NORMAL_CODE(tuck, over, "tuck", dotuck)

VOID dopick()
{
*sp = snth(sp->INT32);
}

NORMAL_CODE(pick, tuck, "pick", dopick)

VOID dotor()
{
rpush(spop(INT32));
}

COMPILATION_CODE(tor, pick, ">r", dotor)

VOID dofromr()
{
spush(rpop(), INT32);
}

COMPILATION_CODE(fromr, tor, "r>", dofromr)

VOID docopyr()
{
spush(*rp, INT32);
}

COMPILATION_CODE(copyr, fromr, "r@", docopyr)

VOID dotwotor()
{
rpush(spop(INT32));
rpush(spop(INT32));
}

COMPILATION_CODE(twotor, copyr, "2>r", dotwotor)

VOID dotwofromr()
{
spush(rpop(), INT32);
spush(rpop(), INT32);
}

COMPILATION_CODE(twofromr, twotor, "2r>", dotwofromr)

VOID dotwodrop()
{
sndrop(1);
}

NORMAL_CODE(twodrop, twofromr, "2drop", dotwodrop)

VOID dondrop()
{
sndrop(sp->INT32);
}

NORMAL_CODE(_ndrop, twodrop, "ndrop", dondrop)

VOID dotwoswap()
{
register UNIV t;

t = *sp;
*sp = snth(1);
snth(1) = t;

t = snth(0);
snth(0) = snth(2);
snth(2) = t;
}

NORMAL_CODE(twoswap, _ndrop, "2swap", dotwoswap)

VOID dotworot()
{
    register UNIV t;

    t = *sp;
    *sp = snth(3);
    snth(3) = snth(1);
    snth(1) = t;

    t = snth(0);
    snth(0) = snth(4);
    snth(4) = snth(2);
    snth(2) = t;
}

NORMAL_CODE(tworot, twoswap, "2rot", dotworot)

VOID dotwodup()
{
    spush(snth(1).INT32, INT32);
    spush(snth(1).INT32, INT32);
}

NORMAL_CODE(twodup, tworot, "2dup", dotwodup)

VOID dotwoover()
{
spush(snth(3).INT32, INT32);
spush(snth(3).INT32, INT32);
}

NORMAL_CODE(twoover, twodup, "2over", dotwoover)


/* COMPARISON */

VOID dolessthan()
{
relation(<, INT32);
}

NORMAL_CODE(lessthan, twoover, "<", dolessthan)

VOID doequals()
{
relation(==, INT32);
}

NORMAL_CODE(equals, lessthan, "=", doequals)

VOID donotequals()
{
relation(!=, INT32);
}

NORMAL_CODE(notequals, equals, "!=", donotequals)

VOID dogreaterthan()
{
relation(>, INT32);
}

NORMAL_CODE(greaterthan, equals, ">", dogreaterthan)

VOID dozeroless()
{
compare(< 0, INT32);
}

NORMAL_CODE(zeroless, greaterthan, "0<", dozeroless)

VOID dozeroequals()
{
compare(== 0, INT32);
}

NORMAL_CODE(zeroequals, zeroless, "0=", dozeroequals)

VOID dozerogreater()
{
compare(> 0, INT32);
}

NORMAL_CODE(zerogreater, zeroequals, "0>", dozerogreater)

VOID doulessthan()
{
relation(<, NUM32);
}

NORMAL_CODE(ulessthan, zerogreater, "u<", doulessthan)

/* CONSTANTS */

NORMAL_CONSTANT(nil, ulessthan, "nil", NIL)

NORMAL_CONSTANT(minusfour, nil, "-4", -4)

NORMAL_CONSTANT(minustwo, minusfour, "-2", -2)

NORMAL_CONSTANT(minusone, minustwo, "-1", -1)

NORMAL_CONSTANT(zero, minusone, "0", 0)

NORMAL_CONSTANT(one, zero, "1", 1)

NORMAL_CONSTANT(two, one, "2", 2)

NORMAL_CONSTANT(four, two, "4", 4)


/* ARITHMETRIC */

VOID doplus()
{
binary(+, INT32);
}

NORMAL_CODE(plus, four, "+", doplus)

VOID dominus()
{
binary(-, INT32);
}

NORMAL_CODE(minus, plus, "-", dominus)

VOID dooneplus()
{
unary(++, INT32);
}

NORMAL_CODE(oneplus, minus, "1+", dooneplus)

VOID dooneminus()
{
unary(--, INT32);
}

NORMAL_CODE(oneminus, oneplus, "1-", dooneminus)

VOID dotwoplus()
{
unary(2 +, INT32);
}

NORMAL_CODE(twoplus, oneminus, "2+", dotwoplus)

VOID dotwominus()
{
unary(-2 +, INT32);
}

NORMAL_CODE(twominus, twoplus, "2-", dotwominus)

VOID dotwotimes()
{
sp->INT32 <<= 1;
}

NORMAL_CODE(twotimes, twominus, "2*", dotwotimes)

VOID doleftshift()
{
binary(<<, INT32);
}

NORMAL_CODE(leftshift, twotimes, "<<", doleftshift)

VOID dotimes()
{
binary(*, INT32);
}

NORMAL_CODE(times_entry, leftshift, "*", dotimes)

VOID doumtimes()
{
binary(*, NUM32);
}

NORMAL_CODE(utimes_entry, times_entry, "um*", doumtimes)

VOID doumdividemod()
{
register NUM32 t;

t = snth(0).NUM32;
snth(0).NUM32 = t % sp->NUM32;
sp->NUM32 = t / sp->NUM32;
}

NORMAL_CODE(umdividemod, utimes_entry, "um/mod", doumdividemod)

VOID dotwodivide()
{
sp->INT32 >>= 1;
}

NORMAL_CODE(twodivide, umdividemod, "2/", dotwodivide)

VOID dorightshift()
{
binary(>>, INT32);
}

NORMAL_CODE(rightshift, twodivide, ">>", dorightshift)

VOID dodivide()
{
binary(/, INT32);
}

NORMAL_CODE(divide, rightshift, "/", dodivide)

VOID domod()
{
binary(%, INT32);
}

NORMAL_CODE(mod, divide, "mod", domod)

VOID dodividemod()
{
    register INT32 t;

    t = snth(0).INT32;
    snth(0).INT32 = t % sp->INT32;
    sp->INT32 = t / sp->INT32;
}

NORMAL_CODE(dividemod, mod, "/mod", dodividemod)

VOID dotimesdividemod()
{
    register INT32 t;

    t = spop(INT32);
    sp->INT32 = sp->INT32 * snth(0).INT32;
    snth(0).INT32 = sp->INT32 % t;
    sp->INT32 = sp->INT32 / t;
}

NORMAL_CODE(timesdividemod, dividemod, "*/mod", dotimesdividemod)

VOID dotimesdivide()
{
    register INT32 t;

    t = spop(INT32);
    binary(*, INT32);
    spush(t, INT32);
    binary(/, INT32);
}

NORMAL_CODE(timesdivide, timesdividemod, "*/", dotimesdivide)

VOID domin()
{
    register INT32 t;

    t = spop(INT32);
    sp->INT32 = (t < sp->INT32 ? t : sp->INT32);
}

NORMAL_CODE(min, timesdivide, "min", domin)

VOID domax()
{
    register INT32 t;

    t = spop(INT32);
    sp->INT32 = (t > sp->INT32 ? t : sp->INT32);
}

NORMAL_CODE(max, min, "max", domax)

VOID doabs()
{
    sp->INT32 = (sp->INT32 < 0 ? - sp->INT32 : sp->INT32);
}

NORMAL_CODE(abs_entry, max, "abs", doabs)

VOID donegate()
{
    unary(-, INT32);
}

NORMAL_CODE(negate, abs_entry, "negate", donegate)


/* MEMORY */

VOID dofetch()
{
    unary(*(PTR32), INT32);
}

NORMAL_CODE(fetch, negate, "@", dofetch)

VOID dostore()
{
    register PTR32 t;

    t = spop(PTR32);
    *t = spop(INT32);
}

NORMAL_CODE(store, fetch, "!", dostore)

VOID dowfetch()
{
    unary(*(NUM16 *), NUM32);
}

NORMAL_CODE(wfetch, store, "w@", dowfetch)

VOID dolesswfetch()
{
    unary(*(PTR16), INT32);
}

NORMAL_CODE(lesswfetch, wfetch, "<w@", dolesswfetch)

VOID dowstore()
{
    register PTR16 t;

    t = spop(PTR16);
    *t = spop(INT32);
}

NORMAL_CODE(wstore, lesswfetch, "w!", dowstore)

VOID docfetch()
{
    unary(*(NUM8 *), NUM32);
}

NORMAL_CODE(cfetch, wstore, "c@", docfetch)

VOID dolesscfetch()
{
    unary(*(PTR8), INT32);
}

NORMAL_CODE(lesscfetch, cfetch, "<c@", dolesscfetch)

VOID docstore()
{
    register PTR8 t;

    t = spop(PTR8);
    *t = spop(INT32);
}

NORMAL_CODE(cstore, lesscfetch, "c!", docstore)

VOID doffetch()
{
    register INT32 pos;
    register INT32 width;

    width = spop(INT32);
    pos = spop(INT32);
    if (width < sizeof(INT32) * 8)
        sp->INT32 = (sp->INT32 >> pos) & ~(-1 << width);
}

NORMAL_CODE(ffetch, cstore, "f@", doffetch)

VOID dolessffetch()
{
    register INT32 pos;
    register INT32 width;

    width = spop(INT32);
    pos = spop(INT32);
    if (width < sizeof(INT32) * 8) {
        sp->INT32 = (sp->INT32 >> pos) & ~(-1 << width);
        if ((1 << (width - 1)) & sp->INT32) {
            sp->INT32 = (sp->INT32) | (-1 << width);
        }
    }
}

NORMAL_CODE(lessffetch, ffetch, "<f@", dolessffetch)

VOID dofstore()
{
    register INT32 pos;
    register INT32 width;
    register INT32 value;

    width = spop(INT32);
    pos = spop(INT32);
    value = spop(INT32);
    sp->INT32 = ((sp->INT32 & ~(-1 << width)) << pos) | (value & ~((~(-1 << width)) << pos));
}

NORMAL_CODE(fstore, lessffetch, "f!", dofstore)

VOID dobfetch()
{
    register INT32 bit;

    bit = spop(INT32);
    sp->INT32 = (((sp->INT32 >> bit) & 1) ? TRUE : FALSE);
}

NORMAL_CODE(bfetch, fstore, "b@", dobfetch)

VOID dobstore()
{
    register INT32 bit;
    register INT32 value;

    bit = spop(INT32);
    value = spop(INT32);
    sp->INT32 = (sp->INT32 ? (value | (1 << bit)) : (value & ~(1 << bit)));
}

NORMAL_CODE(bstore, bfetch, "b!", dobstore)

VOID doplusstore()
{
    register PTR32 t;

    t = spop(PTR32);
    *t += spop(INT32);
}

NORMAL_CODE(plusstore, bstore, "+!", doplusstore)

VOID dotwofetch()
{
    register PTR32 t;

    t = sp->PTR32;
    spush(*t++, INT32);
    snth(0).INT32 = *t;
}

NORMAL_CODE(twofetch, plusstore, "2@", dotwofetch)

VOID dotwostore()
{
    register PTR32 t;

    t = spop(PTR32);
    *t++ = spop(INT32);
    *t = spop(INT32);
}

NORMAL_CODE(twostore, twofetch, "2!", dotwostore)


/* STRINGS */

VOID docmove()
{
    register INT32 n;
    register CSTR to;
    register CSTR from;

    n = spop(INT32);
    to = spop(CSTR);
    from = spop(CSTR);

    while (--n != -1) *to++ = *from++;
}

NORMAL_CODE(cmove, twostore, "cmove", docmove)

VOID docmoveup()
{
    register INT32 n;
    register CSTR to;
    register CSTR from;

    n = spop(INT32);
    to = spop(CSTR);
    from = spop(CSTR);

    to += n;
    from += n;
    while (--n != -1) *--to = *--from;
}

NORMAL_CODE(cmoveup, cmove, "cmove>", docmoveup)

VOID dofill()
{
    register INT32 with;
    register INT32 n;
    register CSTR from;

    with = spop(INT32);
    n = spop(INT32);
    from = spop(CSTR);

    while (--n != -1) *from++ = with;
}

NORMAL_CODE(fill, cmoveup, "fill", dofill)

VOID docount()
{
    register CSTR t;

    t = spop(CSTR);
    spush(*t++, INT32);
    spush(t, CSTR);
}

NORMAL_CODE(count, fill, "count", docount)

VOID dobounds()
{
    register CSTR n;

    n = snth(0).CSTR;
    snth(0).CSTR = snth(0).CSTR + sp->INT32;
    sp->CSTR = n;
}

NORMAL_CODE(bounds, count, "bounds", dobounds)

VOID dodashtrailing()
{
    register CSTR p;

    p = snth(0).CSTR + sp->INT32;
    sp->INT32 += 1;
    while (--(sp->INT32) && (*--p == ' '));
}

NORMAL_CODE(dashtrailing, bounds, "-trailing", dodashtrailing)

VOID dodashmatch()
{
    register INT32 n;
    register CSTR s;
    register CSTR t;

    n = spop(INT32);
    s = spop(CSTR);
    t = spop(CSTR);

    if (n) {
        while ((n) && (*s++ == *t++)) n--;
        spush(n ? TRUE : FALSE, BOOL);
    }
    else {
        spush(TRUE, BOOL);
    }
}

NORMAL_CODE(dashmatch, dashtrailing, "-match", dodashmatch)


/* NUMERICAL CONVERSION */

NORMAL_VARIABLE(base, dashmatch, "base", 10)

VOID dobinary()
{
    base.parameter = 2;
}

NORMAL_CODE(binary_entry, base, "binary", dobinary)

VOID dooctal()
{
    base.parameter = 8;
}

NORMAL_CODE(octal, binary_entry, "octal", dooctal)

VOID dodecimal()
{
    base.parameter = 10;
}

NORMAL_CODE(decimal, octal, "decimal", dodecimal)

VOID dohex()
{
    base.parameter = 16;
}

NORMAL_CODE(hex, decimal, "hex", dohex)

VOID doconvert()
{
register CHAR c;
register INT32 b;
register INT32 n;

b = base.parameter;
n = snth(0).INT32;

for (;;)
  {
  c = *(sp->CSTR);
  if (c < '0' || c > 'z' || (c > '9' && c < 'a'))
    {
    snth(0).INT32 = n;
    return;
    }
  else
    {
    if (c > '9') c = c - 'a' + ':';
    c = c - '0';
    if (c < 0 || c >= b)
      {
      snth(0).INT32 = n;
      return;
      }
    n = (n * b) + c;
    sp->INT32 += 1;
    }
  }
}

NORMAL_CODE(convert, hex, "convert", doconvert)

VOID dolesssharp()
{
    hld = (INT32) thepad + PADSIZE;
}

NORMAL_CODE(lesssharp, convert, "<#", dolesssharp)

VOID dosharp()
{
    register NUM32 n;

    n = sp->NUM32;
    sp->NUM32 = n / (NUM32) base.parameter;
    n = n % (NUM32) base.parameter;
    *(CSTR) --hld = n + ((n > 9) ? 'a' - 10 : '0');
}

NORMAL_CODE(sharp, lesssharp, "#", dosharp)

VOID dosharps()
{
    do { dosharp(); } while (sp->INT32);
}

NORMAL_CODE(sharps, sharp, "#s", dosharps)

VOID dohold()
{
    *(CSTR) --hld = spop(INT32);
}

NORMAL_CODE(hold, sharps, "hold", dohold)

VOID dosign()
{
    INT32 flag;

    flag = spop(INT32);
    if (flag < 0) *(CSTR) --hld = '-';
}

NORMAL_CODE(sign, hold, "sign", dosign)

VOID dosharpgreater()
{
    sp->INT32 = hld;
    spush((INT32) thepad + PADSIZE - hld, INT32);
}

NORMAL_CODE(sharpgreater, sign, "#>", dosharpgreater)

VOID doqnumber()
{
    register CSTR s0;
    register CSTR s1;

    s0 = spop(CSTR);
    spush(0, INT32);
    if (*s0 == '-') {
        spush(s0 + 1, CSTR);
    }
    else {
        spush(s0, CSTR);
    }
    doconvert();
    s1 = spop(CSTR);
    if (*s1 == '\0') {
        if (*s0 == '-') unary(-, INT32);
        spush(TRUE, BOOL);
    }
    else {
        sp->CSTR = s0;
        spush(FALSE, BOOL);
    }
}

NORMAL_CODE(qnumber, sharpgreater, "?number", doqnumber)


/* CONTROL STRUCTURES */

INT32 docheck(this)
INT this;
{
ENTRY last;
INT32 follow = spop(INT32);

/* Check if the symbol is in the follow set */
if (this & follow)
  {

  /* Return true is so */
  return TRUE;
  }
else
  {

  /* Else report a control structure error */
  dolast();
  last = spop(ENTRY);
  sprintf(s_err, "%s%s: illegal control structure\n", input_info(),
      last -> name);
  emit_err();
  doabort();

  return FALSE;
  }
}

VOID dodo()
{
    spush(&parendo, CODE_ENTRY);
    dothread();
    doforwardmark();
    dobackwardmark();
    spush(LOOP+PLUSLOOP, INT32);
}

COMPILATION_IMMEDIATE_CODE(do_entry, qnumber, "do", dodo)

VOID doqdo()
{
    spush(&parenqdo, CODE_ENTRY);
    dothread();
    doforwardmark();
    dobackwardmark();
    spush(LOOP+PLUSLOOP, INT32);
}

COMPILATION_IMMEDIATE_CODE(qdo_entry, do_entry, "?do", doqdo)

VOID doloop()
{
    if (docheck(LOOP)) {
        spush(&parenloop, CODE_ENTRY);
        dothread();
        dobackwardresolve();
        doforwardresolve();
    }
}

COMPILATION_IMMEDIATE_CODE(loop, qdo_entry, "loop", doloop)

VOID doplusloop()
{
    if (docheck(PLUSLOOP)) {
        spush(&parenplusloop, CODE_ENTRY);
        dothread();
        dobackwardresolve();
        doforwardresolve();
    }
}

COMPILATION_IMMEDIATE_CODE(plusloop, loop, "+loop", doplusloop)

VOID doleave()
{
rndrop(2);
fjump(rpop());
fbranch(*ip);
}

COMPILATION_CODE(leave, plusloop, "leave", doleave)

VOID doi()
{
spush(rnth(1), INT32);
}

COMPILATION_CODE(i_entry, leave,"i", doi)

VOID doj()
{
spush(rnth(4), INT32);
}

COMPILATION_CODE(j_entry, i_entry, "j", doj)

VOID doif()
{
spush(&parenqbranch, CODE_ENTRY);
dothread();
doforwardmark();
spush(ELSE+THEN, INT32);
}

COMPILATION_IMMEDIATE_CODE(if_entry, j_entry, "if", doif)

VOID doelse()
{
if (docheck(ELSE))
  {
  spush(&parenbranch, CODE_ENTRY);
  dothread();
  doforwardmark();
  doswap();
  doforwardresolve();
  spush(THEN, INT32);
  }
}

COMPILATION_IMMEDIATE_CODE(else_entry, if_entry, "else", doelse)

VOID dothen()
{
if (docheck(THEN))
  {
  doforwardresolve();
  }
}

COMPILATION_IMMEDIATE_CODE(then_entry, else_entry, "then", dothen)

VOID docase()
{
spush(0, INT32);
spush(OF+ENDCASE, INT32);
}

COMPILATION_IMMEDIATE_CODE(case_entry, then_entry, "case", docase)

VOID doof()
{
    if (docheck(OF)) {
        spush(&over, CODE_ENTRY);
        dothread();
        spush(&equals, CODE_ENTRY);
        dothread();
        spush(&parenqbranch, CODE_ENTRY);
        dothread();
        doforwardmark();
        spush(&drop, CODE_ENTRY);
        dothread();
        spush(ENDOF, INT32);
    }
}

COMPILATION_IMMEDIATE_CODE(of_entry, case_entry, "of", doof)

VOID doendof()
{
    if (docheck(ENDOF)) {
        spush(&parenbranch, CODE_ENTRY);
        dothread();
        doforwardmark();
        doswap();
        doforwardresolve();
        spush(OF+ENDCASE, INT32);
    }
}

COMPILATION_IMMEDIATE_CODE(endof, of_entry, "endof", doendof)

VOID doendcase()
{
    if (docheck(ENDCASE)) {
        spush(&drop, CODE_ENTRY);
        dothread();
        while (sp->INT32) doforwardresolve();
        dodrop();
    }
}

COMPILATION_IMMEDIATE_CODE(endcase, endof, "endcase", doendcase)

VOID dobegin()
{
    dobackwardmark();
    spush(AGAIN+UNTIL+WHILE, INT32);
}

COMPILATION_IMMEDIATE_CODE(begin, endcase, "begin", dobegin)

VOID dountil()
{
    if (docheck(UNTIL)) {
        spush(&parenqbranch, CODE_ENTRY);
        dothread();
        dobackwardresolve();
    }
}

COMPILATION_IMMEDIATE_CODE(until, begin, "until", dountil)

VOID dowhile()
{
    if (docheck(WHILE)) {
        spush(&parenqbranch, CODE_ENTRY);
        dothread();
        doforwardmark();
        spush(REPEAT, INT32);
    }
}

COMPILATION_IMMEDIATE_CODE(while_entry, until, "while", dowhile)

VOID dorepeat()
{
    if (docheck(REPEAT)) {
        spush(&parenbranch, CODE_ENTRY);
        dothread();
        doswap();
        dobackwardresolve();
        doforwardresolve();
    }
}

COMPILATION_IMMEDIATE_CODE(repeat, while_entry, "repeat", dorepeat)

VOID doagain()
{
    if (docheck(AGAIN)) {
        spush(&parenbranch, CODE_ENTRY);
        dothread();
        dobackwardresolve();
    }
}

COMPILATION_IMMEDIATE_CODE(again, repeat, "again", doagain)

VOID dorecurse()
{
    dolast();
    dothread();
}

COMPILATION_IMMEDIATE_CODE(recurse, again, "recurse", dorecurse)

VOID dotailrecurse()
{
    if (theframed) {
        spush(&parenunlink, CODE_ENTRY);
        dothread();
    }
    dolast();
    dotobody();
    spush(&parenbranch, CODE_ENTRY);
    dothread();
    dobackwardresolve();
}

COMPILATION_IMMEDIATE_CODE(tailrecurse, recurse, "tail-recurse", dotailrecurse)

VOID doexit()
{
    fsemicolon();
}

COMPILATION_CODE(exit_entry, tailrecurse, "exit", doexit)

VOID doexecute()
{
ENTRY t;

t = spop(ENTRY);
docall(t);
}

NORMAL_CODE(execute, exit_entry, "execute", doexecute)

VOID dobye()
{
    quited = FALSE;
}

NORMAL_CODE(bye, execute, "bye", dobye)


/* TERMINAL INPUT-OUTPUT */

VOID dodot()
{
if (sp->INT32 < 0)
  {
  emit('-');
  unary(-, INT32);
  }
doudot();
}

NORMAL_CODE(dot, bye, ".", dodot)

VOID dodotr()
{
    INT32 s, t;

    t = spop(INT32);
    s = sp->INT32;
    doabs();
    dolesssharp();
    dosharps();
    spush(s, INT32);
    dosign();
    dosharpgreater();
    spush(t, INT32);
    sover();
    dominus();
    dospaces();
    dotype();
}

NORMAL_CODE(dotr, dot, ".r", dodotr)

VOID doudot()
{
    dolesssharp();
    dosharps();
    dosharpgreater();
    dotype();
    dospace();
}

NORMAL_CODE(udot, dotr, "u.", doudot)

VOID doudotr()
{
    INT32 t;

    t = spop(INT32);
    dolesssharp();
    dosharps();
    dosharpgreater();
    spush(t, INT32);
    sover();
    dominus();
    dospaces();
    dotype();
}

NORMAL_CODE(udotr, udot, "u.r", doudotr)

VOID doascii()
{
spush(' ', INT32);
doword();
docfetch();
doliteral();
}

IMMEDIATE_CODE(ascii, udotr, "ascii", doascii)

VOID dodotquote()
{
input_scan(thetib, '"');
spush(thetib, CSTR);
dosdup();
snip();
spush(&parendotquote, CODE_ENTRY);
dothread();
docomma();
}

COMPILATION_IMMEDIATE_CODE(dotquote, ascii, ".\"", dodotquote)

VOID dodotparen()
{
input_scan(thetib, ')');
spush(thetib, CSTR);
dosprint();
}

IMMEDIATE_CODE(dotparen, dotquote, ".(", dodotparen)

VOID dodots()
{
int num, i;

/* Print the stack depth */
num=s0-sp;
sprintf(s_out, "[%d] ", num);
emit_();
for (i=num-1; i>=0; i--)
  {
  INT32 val;
  emit_s(" \\");
  val=sp[i].INT32;
  spush(val, INT32);
  dodot();
  }
}

NORMAL_CODE(dots, dotparen, ".s", dodots)

VOID docr()
{
emit('\n');
}

NORMAL_CODE(cr, dots, "cr", docr)

VOID doemit()
{
CHAR c;

c = (CHAR) spop(INT32);
emit(c);
}

NORMAL_CODE(do_emit, cr, "emit", doemit)

VOID dotype()
{
register INT32 n;
register CSTR s;

n = spop(INT32);
s = spop(CSTR);
while (n--) emit(*s++);
}

NORMAL_CODE(type, do_emit, "type", dotype)

VOID dospace()
{
emit(' ');
}

NORMAL_CODE(space, type, "space", dospace)

VOID dospaces()
{
register INT32 n;

n = spop(INT32);
while (n-- > 0) emit(' ');
}

NORMAL_CODE(spaces, space, "spaces", dospaces)

VOID dokey()
{
spush(key(), INT32);
}

NORMAL_CODE(_key, spaces, "key", dokey)

VOID doexpect()
{
char* dest;
int n;

/* Pop buffer pointer and size */
n = spop(INT32);
dest = spop(CSTR);

span.parameter=input_counted_scan(dest, '\n', n);
}

NORMAL_CODE(_expect, _key, "expect", doexpect)

NORMAL_VARIABLE(span, _expect, "span", 0)

VOID doline()
{
spush(input_line(), INT32);
}

NORMAL_CODE(line, span, "line", doline)

VOID dosource()
{
spush(input_source(), CSTR);
}

NORMAL_CODE(source, line, "source", dosource)


/* PROGRAM BEGINNING AND TERMINATION */

VOID doforth83()
{
}

NORMAL_CODE(forth83, source, "forth-83", doforth83)

VOID dointerpret()
{
INT32 flag;                 /* Flag value returned by for words */

#ifdef CASTING
INT32 cast;                 /* Casting operation flag */
#endif

quited = TRUE;              /* Iterate until bye or end of input */

while (quited)
  {

  /* Check stack underflow */
  if (s0<sp)
    {
    sprintf(s_err, "%sinterpret: stack underflow\n", input_info());
    emit_err();
    doabort();
    }

  /* Scan for the next symbol */
  spush(' ', INT32);
  doword();

  /* Exit top loop if end of input stream */
  if (input_eof())
    {
    sdrop();
    return;
    }

  /* Search for the symbol in the current vocabulary search set*/
  dofind();
  flag = spop(INT32);

#ifdef CASTING
  /* Check for vocabulary casting prefix */
  for (cast = flag; !cast;)
    {
    CSTR s = sp->CSTR;
    INT32 l = strlen(s) - 1;

    /* Assume casting prefix */
    cast = TRUE;

    /* Check casting syntax, vocabulary name within parethesis */
    if ((s[0] == '(') && (s[l] == ')'))
      {

      /* Remove the parenthesis from the input string */
      s[l] = 0;
      unary(++, INT32);

      /* Search for the symbol again */
      dofind();
      flag = spop(INT32);

      /* If found check that its a vocabulary */
      if (flag)
        {
        ENTRY v = spop(ENTRY);

        /* Check that the symbol is really a vocabulary */
        if (v -> code == VOCABULARY)
          {

          /* Scan for a new symbol */
          spush(' ', INT32);
          doword();

          /* Exit top loop if end of input stream */
          if (input_eof())
            {
            sdrop();
            return;
            }

          /* And look for it in the given vocabulary */
          spush(v, ENTRY);
          dolookup();
          flag = spop(INT32);
          cast = flag;
          }
        }
      else
        {
        /* Restore string after vocabulary name test */
        s[l] = ')';
        unary(--, INT32);
        }
      }
    }
#endif

  /* If found then execute or thread the symbol */
  if (flag)
    {
    if (state.parameter == flag)
      {
      dothread();
      }
    else
      {
      docommand();
      }
    }
  else
    {
    /* Else check if it is a literal */
    dorecognize();
    flag = spop(INT32);
    if (flag)
      {
      doliteral();
      }
    else
      {
      sprintf(s_err, "%s%s ??\n", input_info(), sp->CSTR);
      emit_err();
/*    doabort();*/
      }
    }
  }
quited = TRUE;
}

NORMAL_CODE(interpret, forth83, "interpret", dointerpret)

VOID doquit()
{
rinit();
doleftbracket();
dointerpret();
}

NORMAL_CODE(quit, interpret, "quit", doquit)

VOID doabort()
{
/* Check if it is the foreground task */
if (tp == foreground)
  {
  sinit();
  doleftbracket();
  input_flush();
  }

/* Terminate aborted tasks */
doterminate();
}

NORMAL_CODE(abort_entry, quit, "abort", doabort)

VOID doabortquote()
{
spush('"', INT32);
doword();
dosdup();
snip();
spush(&parenabortquote, CODE_ENTRY);
dothread();
docomma();
}

COMPILATION_IMMEDIATE_CODE(abortquote, abort_entry, "abort\"", doabortquote)


/* DICTIONARY ADDRESSES */

VOID dohere()
{
    spush(dp, PTR32);
}

NORMAL_CODE(here, abortquote, "here", dohere)

NORMAL_CONSTANT(pad, here, "pad", (INT32) thepad)

NORMAL_CONSTANT(tib, pad, "tib", (INT32) thetib)

VOID dotobody()
{
    sp->INT32 = sp->ENTRY -> parameter;
}

NORMAL_CODE(tobody, tib, ">body", dotobody)

VOID dodotname()
{
ENTRY e = spop(ENTRY);

emit_s(e -> name);
}

NORMAL_CODE(dotname, tobody, ".name", dodotname)

NORMAL_CONSTANT(cell, dotname, "cell", 4)

VOID docells()
{
    sp->INT32 <<= 2;
}

NORMAL_CODE(cells, cell, "cells", docells)

VOID docellplus()
{
    sp->INT32 += 4;
}

NORMAL_CODE(cellplus, cells, "cell+", docellplus)


/* COMPILER AND INTERPRETER WORDS */

VOID dosharpif()
{
INT32 symbol;
BOOL flag;

flag = spop(BOOL);

if (!flag)
  {
  do
    {
    spush(' ', INT32);
    doword();
    symbol = spop(INT32);
    if (STREQ(symbol, "#if"))
      {
      dosharpelse();
      spush(' ', INT32);
      doword();
      symbol = spop(INT32);
      }
    }
  while (!((STREQ(symbol, "#else") || STREQ(symbol, "#then"))));
  }
}

IMMEDIATE_CODE(sharpif, cellplus, "#if", dosharpif)

VOID dosharpelse()
{
INT32 symbol;

do
  {
  spush(' ', INT32);
  doword();
  symbol = spop(INT32);
  if (STREQ(symbol, "#if"))
    {
    dosharpelse();
    spush(' ', INT32);
    doword();
    symbol = spop(INT32);
    }
  }
while (!STREQ(symbol, "#then"));
}

IMMEDIATE_CODE(sharpelse, sharpif, "#else", dosharpelse)

VOID dosharpthen()
{
}

IMMEDIATE_CODE(sharpthen, sharpelse, "#then", dosharpthen)

VOID dosharpifdef()
{
spush(' ', INT32);
doword();
dofind();
doswap();
dodrop();
dosharpif();
}

IMMEDIATE_CODE(sharpifdef, sharpthen, "#ifdef", dosharpifdef)

VOID dosharpifundef()
{
spush(' ', INT32);
doword();
dofind();
doswap();
dodrop();
dozeroequals();
dosharpif();
}

IMMEDIATE_CODE(sharpifundef, sharpifdef, "#ifundef", dosharpifundef)

VOID dosharpinclude()
{
INT32 flag;
CSTR  fname;
int res;

spush(' ', INT32);
doword();
fname = spop(CSTR);
if ((res=include_file(fname))!=0)
  {
  sprintf(s_err, "%scannot open file \"%s\": %s\n", input_info(), fname,
      strerror(res));
  emit_err();
  }
}

NORMAL_CODE(sharpinclude, sharpifundef, "#include", dosharpinclude)
/*
VOID dosharppath()
{
INT32 flag;

spush(' ', INT32);
doword();
if (flag = io_path(sp->CSTR, IO_PATH_FIRST) == IO_UNKNOWN_PATH)
  {
  sprintf(s_err, "%s%s: unknown environment variable\n", input_unfo(),
      sp->CSTR);
  emit_err();
  }
else
  {
  if (flag == IO_TOO_MANY_PATHS)
    {
    sprintf(s_err, "%s%s: too many paths defined\n", input_unfo(), sp->CSTR);
    emit_err();
    }
  }
dodrop();
}

NORMAL_CODE(sharppath, sharpinclude, "#path", dosharppath)
*/

VOID doparen()
{
int c;

while (1)
  {
  c = key();
  if (c==-1)
    {
    sprintf(s_err, "%skernel: end of file during comment\n", input_info());
    emit_err();
    return;
    }
  if (c == ')')
    return;
  else if (c == '(')
    {
    sprintf(s_err, "%skernel: warning nested comment\n", input_info());
    emit_err();
    doparen();
    }
  }
}

/*IMMEDIATE_CODE(paren, sharppath, "(", doparen);*/
IMMEDIATE_CODE(paren, sharpinclude, "(", doparen)

VOID dobackslash()
{
int c;
while (1)
  {
  c=key();
  if ((c=='\n') || (c==-1)) return;
  }
}

IMMEDIATE_CODE(backslash, paren, "\\", dobackslash)

VOID docomma()
{
*dp++ = spop(INT32);
}

NORMAL_CODE(comma, backslash, ",", docomma)

VOID doallot()
{
INT32 n;

n = spop(INT32);
dp = (PTR32) ((PTR8) dp + n);
}

NORMAL_CODE(allot, comma, "allot", doallot)

VOID doalign()
{
align(dp);
}

NORMAL_CODE(align_entry, allot, "align", doalign)

VOID dodoes()
{
    if (theframed != NIL) {
        spush(&parenunlinkdoes, CODE_ENTRY);
    }
    else {
        spush(&parendoes, CODE_ENTRY);
    }
    dothread();
    doremovelocals();
}

COMPILATION_IMMEDIATE_CODE(does, align_entry, "does>", dodoes)

VOID doimmediate()
{
    current -> last -> mode |= IMMEDIATE;
}

NORMAL_CODE(immediate, does, "immediate", doimmediate)

VOID doexecution()
{
    current -> last -> mode |= EXECUTION;
}

NORMAL_CODE(execution, immediate, "execution", doexecution)

VOID docompilation()
{
    current -> last -> mode |= COMPILATION;
}

NORMAL_CODE(compilation, execution, "compilation", docompilation)

VOID doprivate()
{
    current -> last -> mode |= PRIVATE;
}

NORMAL_CODE(private_entry, compilation, "private", doprivate)

VOID dorecognizer()
{
    current -> recognizer = current -> last;
}

NORMAL_CODE(recognizer, private_entry, "recognizer", dorecognizer)

VOID dobracketcompile()
{
    dotick();
    dothread();
}

COMPILATION_IMMEDIATE_CODE(bracketcompile, recognizer, "[compile]",
    dobracketcompile)

VOID docompile()
{
    spush(*ip++, INT32);
    dothread();
}

COMPILATION_CODE(compile, bracketcompile, "compile", docompile)

VOID doqcompile()
{
    if (state.parameter) docompile();
}

COMPILATION_CODE(qcompile, compile, "?compile", doqcompile)

NORMAL_VARIABLE(state, qcompile, "state", FALSE)

VOID docompiling()
{
    spush(state.parameter, INT32);
}

NORMAL_CODE(compiling, state, "compiling", docompiling)

VOID doliteral()
{
if (state.parameter)
  {
  spush(&parenliteral, CODE_ENTRY);
  dothread();
  docomma();
  }
}

COMPILATION_IMMEDIATE_CODE(literal, compiling, "literal", doliteral)

VOID doleftbracket()
{
state.parameter = FALSE;
}

IMMEDIATE_CODE(leftbracket, literal, "[", doleftbracket)

VOID dorightbracket()
{
state.parameter = TRUE;
}

NORMAL_CODE(rightbracket, leftbracket, "]", dorightbracket)

VOID doword()
{
CHAR brkchr;

brkchr = (CHAR) spop(INT32);
input_skipspace();
input_scan(thetib, brkchr);
spush(thetib, CSTR);
}

NORMAL_CODE(word_entry, rightbracket, "word", doword)


/* VOCABULARIES */

NORMAL_CONSTANT(context_entry, word_entry, "context", (INT32) context)

NORMAL_CONSTANT(current_entry, context_entry, "current", (INT32) &current)

VOID dolast()
{
spush((theframed ? theframed : current -> last), ENTRY);
}

NORMAL_CODE(last, current_entry, "last", dolast)

VOID dodefinitions()
{
current = context[0];
}


NORMAL_CODE(definitions, last, "definitions", dodefinitions)

VOID doonly()
{
    INT32 v;

    /* Flush the entry cache */
    spush(FALSE, BOOL);
    dorestore();

    /* Remove all vocabularies except the first */
    for (v = 1; v < CONTEXTSIZE; v++) context[v] = NIL;

    /* And make it definition vocabulary */
    current = context[0];
}

NORMAL_CODE(only, definitions, "only", doonly)

VOID doseal()
{
    INT32 v;

    /* Flush the entry cache */
    spush(FALSE, BOOL);
    dorestore();

    /* Remove the first vocabulary */
    for (v = 0; context[v] = context[v + 1]; v++);
}

NORMAL_CODE(seal, only, "seal", doseal)

VOID dorestore()
{
    register INT32 i;           /* Iteration variable */
    register ENTRY e;           /* Pointer to parameter entry */
    register ENTRY p;           /* Pointer to current entry */

    /* Access parameter and check if an entry */
    e = spop(ENTRY);
    if (e) {

        /* Flush all enties until the parameter symbol */
        for (p = current -> last; p && (p != e); p = p -> link)
            cache[hash(p -> name)] = NIL;

        /* If the entry was found remove all symbols until this entry */
        if (p == e) current -> last = e;
    }
    else {

        /* Flush the entry cache */
        for (i = 0; i < CACHESIZE; i++) cache[i] = NIL;
    }
}

NORMAL_CODE(restore, seal, "restore", dorestore)

VOID dotick()
{
BOOL flag;

spush(' ', INT32);
doword();
dofind();
flag = spop(BOOL);
if (!flag)
  {
  sprintf(s_err, "%s%s ??\n", input_info(), sp->CSTR);
  emit_err();
  doabort();
  }
}

NORMAL_CODE(tick, restore, "'", dotick)

VOID dobrackettick()
{
    dotick();
    doliteral();
}

COMPILATION_IMMEDIATE_CODE(brackettick, tick, "[']", dobrackettick)

VOID dolookup()
{
    VOCABULARY_ENTRY v;         /* Search vocabulary */
    register ENTRY e;           /* Search entry */
    register CSTR s;            /* And string */

    /* Fetch parameters and initate entry pointer */
    v = (VOCABULARY_ENTRY) spop(PTR32);
    s = sp->CSTR;

    /* Iterate over the linked list of entries */
    for (e = v -> last; e; e = e -> link)

        /* Compare the symbol and entry string */
        if (STREQ(s, e -> name)) {

            /* Check if the entry is currently visible */
            if (visible(e, v)) {
                /* Return the entry and compilation mode */
                sp->ENTRY = e;
                spush((e -> mode & IMMEDIATE ? 1 : -1), INT32);
                return;
            }
        }
    spush(FALSE, BOOL);
}

NORMAL_CODE(lookup, brackettick, "lookup", dolookup)

#ifdef PROFILE
VOID docollision()
{
    /* Add collision statistics to profile information */
}
#endif

VOID dofind()
{
ENTRY e;                    /* Entry in the entry cache */
CSTR  n;                    /* Name string of entry to be found */
INT32 v;                    /* Index into vocabulary set */

/* Access the string to be found */
n = sp->CSTR;

/* Check for cached entry */
if (e = cache[hash(n)])
  {

  /* Compare the string and the entry name */
  if (STREQ(sp->CSTR, e -> name))
    {

    /* Check if the entry is currently visible */
    if (!(((e -> mode & COMPILATION) && (!state.parameter)) ||
        ((e -> mode & EXECUTION) && (state.parameter))))
      {
      sp->ENTRY = e;
      spush((e -> mode & IMMEDIATE ? 1 : -1), INT32);
      return;
      }
    }
#ifdef PROFILE
  else
    {
    docollision();
    }
#endif
  }

/* For each vocabulary in the current search chain */
for (v = 0; context[v] && v < CONTEXTSIZE; v++)
  {
  spush(context[v], VOCABULARY_ENTRY);
  dolookup();
  if (sp->INT32)
    {
    cache[hash(n)] = snth(0).ENTRY;
    return;
    }
  else
    {
    sdrop();
    }
  }
  spush(FALSE, BOOL);
}

NORMAL_CODE(find, lookup, "find", dofind)

VOID dorecognize()
{
INT32 v;                    /* Vocabulary index */
ENTRY r;                    /* Recognizer function */

for (v = 0; context[v] && v < CONTEXTSIZE; v++)
  {

  /* Check if a recognizer function is available */
  if (r = context[v] -> recognizer)
    {
    spush(r, ENTRY);
    docommand();
    if (sp->INT32)
      {
      return;
      }
    else
      {
      sdrop();
      }
    }
  }

/* The string was not a literal symbol */
spush(FALSE, BOOL);
}

NORMAL_CODE(recognize, find, "recognize", dorecognize)

VOID doforget()
{
dotick();
sp->ENTRY = sp->ENTRY -> link;
dorestore();
}

NORMAL_CODE(forget, recognize, "forget", doforget)

VOID dowords()
{
ENTRY e;                    /* Pointer to entries */
INT32 v;                    /* Index into vocabulary set */
INT32 l;                    /* String length */
INT32 s;                    /* Spaces between words */
INT32 c;                    /* Column counter */
INT32 i;                    /* Loop index */

/* Iterate over all vocabularies in the search set */
for (v = 0; v < CONTEXTSIZE && context[v]; v++)
  {

  /* Print vocabulary name */
  sprintf(s_out, "VOCABULARY %s", context[v] -> name);
  emit_();
  if (context[v] == current)
    {
    sprintf(s_out, " DEFINITIONS");
    emit_();
    }
  emit('\n');

  /* Access linked list of enties and initiate column counter */
  c = 0;

  /* Iterate over all entries in the vocabulary */
  for (e = context[v] -> last; e; e = e -> link)
    {

    /* Check if the entry is current visible */
    if (visible(e, context[v]))
      {
      /* Print the entry string. Check that space is available */
      l = strlen(e -> name);
      s = (c ? (COLUMNWIDTH - (c % COLUMNWIDTH)) : 0);
      c = c + s + l;
      if (c < LINEWIDTH)
        {
        for (i = 0; i < s; i++) emit(' ');
        }
      else
        {
        emit('\n');
        c = l;
        }
      emit_s(e -> name);
      }
    }

  /* End the list of words and separate the vocabularies */
  emit('\n');
  emit('\n');
  }
}

IMMEDIATE_CODE(words, forget, "words", dowords)


/* DEFINING NEW VOCABULARY ENTRIES */

ENTRY make_entry(name, code, mode, parameter)
    CSTR name;                  /* String for the new entry */
    INT32 code, mode, parameter; /* Entry parameters */
{
    /* Allocate space for the entry */
    ENTRY e;

    /* Check type of entry to allocate */
    if (code == VOCABULARY)
        e = (ENTRY) malloc(sizeof(vocabulary_entry));
    else
        e = (ENTRY) malloc(sizeof(entry));

    /* Insert into the current vocabulary and set parameters */
    e -> link = current -> last;
    current -> last = e;

    /* Set entry parameters */
    e -> name = (CSTR) strcpy(malloc((unsigned) strlen(name) + 1), name);
    e -> code = code;
    e -> mode = mode;
    e -> parameter = parameter;
    if (code == VOCABULARY)
        ((VOCABULARY_ENTRY) e) -> recognizer = NIL;

    /* Check for entry caching */
    if (current == context[0])
        cache[hash(name)] = e;
    else
        cache[hash(name)] = NIL;

    /* Return pointer to the new entry */
    return e;
}

VOID doentry()
{
INT32 flag;
CSTR  name;
INT32 code, mode, parameter;
ENTRY forward;

/* Try to find entry to check for forward declarations */
forward = NIL;
dodup();
dofind();
flag = spop(INT32);
if (flag)
  {
  forward = spop(ENTRY);
  }
else
  {
  sdrop();
  }

/* Access name, code, mode and parameter field parameters */
name = spop(CSTR);
code = spop(INT32);
mode = spop(INT32);
parameter = spop(INT32);

/* Create the new entry */
(VOID) make_entry(name, code, mode, parameter);

/* If found and forward the redirect parameter field of initial entry */
if (forward && forward -> code == FORWARD)
  {
  forward -> parameter = (INT32) current -> last;
  if (verbose)
    {
    sprintf(s_err, "%s%s: forward definition resolved\n", input_info(),
        forward -> name);
    emit_err();
    }
  }
}

NORMAL_CODE(entry_entry, words, "entry", doentry)

VOID doforward()
{
spush(0, INT32);
spush(NORMAL, INT32);
spush(FORWARD, INT32);
spush(' ', INT32);
doword();
doentry();
}

NORMAL_CODE(forward, entry_entry, "forward", doforward)

VOID docode()
{
spush(NORMAL, INT32);
spush(CODE, INT32);
spush(' ', INT32);
doword();
doentry();
}

NORMAL_CODE(code, forward, "code", docode)

VOID docolon()
{
align(dp);
dohere();
spush(HIDDEN, INT32);
spush(COLON, INT32);
spush(' ', INT32);
doword();
doentry();
dorightbracket();
thelast = current -> last;
}

NORMAL_CODE(colon, code, ":", docolon)

VOID dosemicolon()
{
if (theframed != NIL)
  {
  spush(&parenunlinksemicolon, CODE_ENTRY);
  }
else
  {
  spush(&parensemicolon, CODE_ENTRY);
  }
dothread();
doleftbracket();
doremovelocals();
if (thelast != NIL)
  {
  thelast -> mode = NORMAL;
  if (current == context[0]) cache[hash(thelast -> name)] = thelast;
  thelast = NIL;
  }
}

COMPILATION_IMMEDIATE_CODE(semicolon, colon, ";", dosemicolon)

VOID docreate()
{
    align(dp);
    dohere();
    spush(NORMAL, INT32);
    spush(CREATE, INT32);
    spush(' ', INT32);
    doword();
    doentry();
}

NORMAL_CODE(_create, semicolon, "create", docreate)

VOID dovariable()
{
    spush(0, INT32);
    spush(NORMAL, INT32);
    spush(VARIABLE, INT32);
    spush(' ', INT32);
    doword();
    doentry();
}

NORMAL_CODE(variable, _create, "variable", dovariable)

VOID doconstant()
{
spush(NORMAL, INT32);
spush(CONSTANT, INT32);
spush(' ', INT32);
doword();
doentry();
}

NORMAL_CODE(constant, variable, "constant", doconstant)

VOID dovocabulary()
{
spush(&forth, VOCABULARY_ENTRY);
spush(NORMAL, INT32);
spush(VOCABULARY, INT32);
spush(' ', INT32);
doword();
doentry();
}

NORMAL_CODE(vocabulary, constant, "vocabulary", dovocabulary)

VOID dofield()
{
spush(NORMAL, INT32);
spush(FIELD, INT32);
spush(' ', INT32);
doword();
doentry();
}

NORMAL_CODE(field, vocabulary, "field", dofield)

/* INITIALIZATION OF THE KERNEL */

VOID kernel_initiate(ENTRY last, ENTRY first, INT32 users, INT32 parameters,
    INT32 returns)
{
/* Link user symbols into vocabulary chain if given */
if (first && last)
  {
  forth.last = last;
  first -> link = (ENTRY) &field;
  }

/* Create the foreground task object */
foreground = make_task(users, parameters, returns, (INT32) NIL);

/* Assign task fields */
foreground -> status = RUNNING;
s0 = (PTR) foreground -> s0;
sp = (PTR) foreground -> sp;
r0 = foreground -> r0;
rp = foreground -> rp;
ip = foreground -> ip;
fp = foreground -> fp;
ep = foreground -> ep;

/* Make the foreground task the current task */
tp = foreground;
}

VOID kernel_finish()
{
/* Future clean up function for kernel */
}

/*****************************************************************************/
/*****************************************************************************/
