# Generated automatically from Makefile.in by configure.
# lowlevel/scsicompat/linux/Makefile.in
# $ZEL: Makefile.in,v 1.4 2007/09/17 00:35:51 wuestner Exp $

CONF = ../../../conf

srcdir = .


include $(CONF)/rules.make
include $(CONF)/config.make

CFLAGS := $(CFLAGS) -DSCSICOMPAT_linux
cfiles = scsicompat_linux.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../libscsicompat.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

######################################################
