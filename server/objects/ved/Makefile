# Generated automatically from Makefile.in by configure.
CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = ved.c $(CONF)/sconf.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../libobj.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

##########################################################
