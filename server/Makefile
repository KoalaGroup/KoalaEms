# Generated automatically from Makefile.in by configure.
# server/Makefile.in
# $ZEL: Makefile.in,v 1.4 2010/12/01 13:28:15 wuestner Exp $

CONF = conf

COMMONLIB = /home/huagen/ems/common

srcdir = .


include $(CONF)/rules.make
include $(CONF)/config.make

subdirs = commu objects dummyprocs
ifdef procs
subdirs += procs
endif
ifdef trigger
subdirs += trigger
endif
ifdef dataout
subdirs += dataout
endif
ifdef lowlevel
subdirs += lowlevel
endif

.PHONY: FORCE conf all depend

#all: server.$(z) server
all: server

server: server.$(z)
	ln -sf server.$(z) server

server.$(z): FORCE $(subdirs) $(COMMONLIB)/libcommon.a
	$(MAKE) -C main ../$@

handler:
	$(MAKE) -C dataout handler

$(subdirs): FORCE conf
	$(MAKE) -C $@ all

$(CONF)/config.make: $(CONF)/$(z).config
	$(MAKE) -C $(CONF) all

conf:
	$(MAKE) -C $(CONF) all

distclean: clean
	-$(RM) config.status config.log config.cache Makefile

depend:
	for i in $(subdirs) main ; do \
	  echo checking directory $$i ; \
	  $(MAKE) -C $$i depend ; \
	done
