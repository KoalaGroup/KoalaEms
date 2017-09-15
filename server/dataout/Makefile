# Generated automatically from Makefile.in by configure.
# dataout/Makefile.in
# $ZEL: Makefile.in,v 1.3 2002/11/06 15:36:29 wuestner Exp $

CONF = ../conf

srcdir = .


include $(CONF)/rules.make
include $(CONF)/config.make

lib = libdo.a

all: $(lib)

ifdef dataout
subdir = $(dataout)
endif

$(lib): $(subdir) dataout.stamp
	$(RANLIB) $@

$(subdir): FORCE
	$(MAKE) -C $@ all

handler:
	$(MAKE) -C $(subdir) handler

depend:
	$(MAKE) -C $(subdir) depend ; \

.PHONY: FORCE depend

###############################################################

