# Generated automatically from Makefile.in by configure.
# conf/Makefile.in
# $ZEL: Makefile.in,v 1.8 2005/02/11 17:55:28 wuestner Exp $

CONF = .

srcdir = .


CPP = gcc -E

include rules.make

makevars = objects domains invocations procs dataout lowlevel \
	commu trigger dummyprocs profilelib

all: config.make sconf.c sconf.h

config.make: thisconfig.kein.c
	$(CPP) -P -I$(srcdir) $< | grep ".*=.*" >$@
	$(MAKE) stamps1

sconf.c: thisconfig.kein.c
	echo "/* automatically generated */" >$@
	echo  "char buildname[]=\"$(z)\";" >>$@
	echo  "char builddate[]=\"`date`\";" >>$@
	echo "char configuration[]=\"\\" >>$@
	$(CPP) -C -P -I$(srcdir) $< | grep -v '^$$' | sed -f $(srcdir)/mksconf.sed >>$@
	echo "\";" >>$@

#	sed -f $(srcdir)/config.sed $< >$@
#	sed -f $(srcdir)/mksconf.sed $< >>$@

sconf.h: thisconfig.kein.c #sconf.h.prefix
	echo "/* automatically generated */" >$@
	echo "#ifndef _sconf_h_" >>$@
	echo "#define _sconf_h_" >>$@
	echo "#include <config.h>" >>$@
	cat $(srcdir)/sconf.h.prefix >>$@
	$(CPP) -P -I$(srcdir) $< | grep "^%" | tr "%" "#" >>$@
	echo "#endif" >>$@
	$(MAKE) stamps2

thisconfig.kein.c: $(z).config
	cp $< $@

stamps1: config.make.alt
	for i in $(makevars) ; do \
		$(MAKE) -f $(srcdir)/Makefile.stamps all var=$$i ; \
	done
	cp config.make config.make.alt

stamps2: sconf.h.alt
	$(CPP) -P -I. $(srcdir)/definealt.kein.c | tr "%" "#" >altdefines
	$(CPP) -P -I. $(srcdir)/cmpdefines.kein.c >configstamps
	sh configstamps
	cp sconf.h sconf.h.alt

config.make.alt sconf.h.alt:
	touch $@

.PHONY: stamps1 stamps2

clean:
	-$(RM) sconf.h sconf.c
	-$(RM) *.alt *.stamp
	-$(RM) config.defines altdefines configstamps buildname builddate thisconfig.kein.c
