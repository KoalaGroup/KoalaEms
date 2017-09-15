# Generated automatically from Makefile.in by configure.
# lowlevel/canbus/Makefile.in
# $ZEL: Makefile.in,v 1.1 2007/03/02 22:52:47 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = canbus.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

subdirs := $(canbus)

CANTYPES = $(shell echo $(canbus)|tr '[:lower:]' '[:upper:]')
CANFLAGS=$(foreach typ,$(CANTYPES),-DCAN_$(typ) )
CFLAGS += $(CANFLAGS)
export CANFLAGS

lib = ../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles)) $(subdirs)
	$(RANLIB) $@

$(subdirs): FORCE
	$(MAKE) -C $@ all

define MAKEIT
	$(MAKE) -C $(dir) depend

endef
MAKEDEP = $(foreach dir,$(canbus),$(MAKEIT))

depend: xdepend
	$(MAKEDEP)

.PHONY: FORCE depend
