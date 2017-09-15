# Generated automatically from Makefile.in by configure.
# procs/dataout/Makefile.in
# $ZEL: Makefile.in,v 1.4 2009/03/12 13:25:14 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = dataoutfile.c dataoutlog.c dataoutloginfo.c flush_data.c \
    dataoutlinkdir.c decodinghints.c fadvise.c clustproc.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

###################################################
