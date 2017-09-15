# Generated automatically from Makefile.in by configure.
# lowlevel/caenet/Makefile.in
# $ZEL: Makefile.in,v 1.1 2007/03/20 20:40:31 wuestner Exp $
# 
# created: 2007-Mar-20 PW

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = caenet.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

subdirs := $(caenet)

CAENTYPES = $(shell echo $(caenet)|tr '[:lower:]' '[:upper:]')
CAENFLAGS=$(foreach typ,$(CAENTYPES),-DCAEN_$(typ) )
CFLAGS += $(CAENFLAGS)
export CAENFLAGS

lib = ../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles)) $(subdirs)
	$(RANLIB) $@

$(subdirs): FORCE
	$(MAKE) -C $@ all

define MAKEIT
	$(MAKE) -C $(dir) depend

endef
MAKEDEP = $(foreach dir,$(caenet),$(MAKEIT))

depend: xdepend
	$(MAKEDEP)

.PHONY: FORCE depend
