# Generated automatically from Makefile.in by configure.
# procs/Makefile.in
# $ZEL: Makefile.in,v 1.4 2002/11/06 15:37:13 wuestner Exp $

CONF = ../conf

srcdir = .


include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = commands.c proclist.c listprocs.c
ofiles = $(cfiles:.c=.o)
depfiles = commands.c proclist.c
hfiles = procs.h

procconfs = $(patsubst %, %/procedures.config, $(procs))

lib = libprocs.a
subdirs = $(procs)

ifdef procs
all: $(lib)
else
all:
endif

$(lib): $(hfiles) $(lib)($(ofiles)) $(subdirs) procs.stamp
	$(RANLIB) $@

$(subdirs): FORCE
	$(MAKE) -C $@ all

listprocs.c: procedures

procs.h: procedures

procedures: procs.stamp $(procconfs)
	-$(RM) $@
	for p in $(procs) ; do \
	cat $(srcdir)/$$p/procedures.config >>$@ ; \
	done

clean:
	-$(RM) listprocs.c
	-$(RM) procedures

depend: xdepend $(hfiles)
	for i in $(subdirs) ; do \
	  echo checking directory $$i ; \
	  $(MAKE) -C $$i depend ; \
	done

.PHONY: FORCE depend

#####################################################################

