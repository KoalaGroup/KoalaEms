# Generated automatically from Makefile.in by configure.
# main/Makefile.in
# $ZEL: Makefile.in,v 1.6 2009/08/27 01:26:42 wuestner Exp $

CONF = ../conf
COMMONLIB = /home/huagen/ems/common

srcdir = .
LIBS = 

include $(CONF)/rules.make

include $(CONF)/config.make

M4FLAGS = -DEMSEXCOMMON=$(EXCOMMON) -DEMSCOMMON=$(COMMON)

cfiles = scheduler.c requests.c server.c signals_unix.c timers_unix.c  \
	childhndlr.c
ofiles = $(cfiles:.c=.o)
depfiles = scheduler.c server.c signals_unix.c timers_unix.c  \
	childhndlr.c

libs1 = ../commu/libcommu.a ../objects/libobj.a
ifdef procs
libs2 = ../procs/libprocs.a
endif
libs3 = ../dummyprocs/libdummy.a
ifdef trigger
libs4 = ../trigger/libtrig.a
endif
ifdef dataout
libs5 = ../dataout/libdo.a
endif
ifdef lowlevel
libs6 = ../lowlevel/libll.a
endif
libs = $(libs2) $(libs3) $(libs4) $(libs5) $(libs6) $(libs1)

all: ../server.$(z)
../server.$(z): $(ofiles) $(libs) $(COMMONLIB)/libcommon.a
	$(CC) $(extralflags) $^  $(LIBS) $(extralibs) -o $@

clean:
	-$(RM) requests.c

depend: xdepend

.PHONY: depend

#########################################################

requests.c: $(COMMON)/requesttypes.inc
requests.o: requests.h requesttypes.h errorcodes.h
