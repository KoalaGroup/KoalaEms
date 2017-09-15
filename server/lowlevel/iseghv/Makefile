# Generated automatically from Makefile.in by configure.
# lowlevel/can/Makefile.in
# $ZEL: Makefile.in,v 1.1 2006/04/20 13:48:50 peterka Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = iseghv.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

define MAKEIT
	$(MAKE) -C $(dir) depend

endef
MAKEDEP = $(foreach dir,$(can),$(MAKEIT))

depend: xdepend
	$(MAKEDEP)

.PHONY: FORCE depend
