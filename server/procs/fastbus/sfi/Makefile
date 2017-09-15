# Generated automatically from Makefile.in by configure.
# procs/fastbus/sfi/Makefile.in
# $ZEL: Makefile.in,v 1.6 2003/09/30 11:56:13 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = \
        sfi_inout.c \
        sfi_raw.c \
#        sfi_blockwrite.c
#        sfi_blockread.c
#        sfi_fbblock_alter.c
#        sfi_timeout.c

ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

FASTBUSTYPES = $(shell echo $(fastbus)|tr '[:lower:]' '[:upper:]')
FASTBUSFLAGS=$(foreach typ,$(FASTBUSTYPES),-DFASTBUS_$(typ) )
CFLAGS += $(FASTBUSFLAGS)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#######################################
