COMMON = /home/huagen/ems/common
EXCOMMON = /home/huagen/ems/common
CC = gcc
CFLAGS = -Wcomment -Wformat -Wimplicit -Wmissing-declarations -Wmissing-prototypes -Wparentheses -Wpointer-arith -Wreturn-type -Wstrict-prototypes -Wunused -Werror
AR = ar
RANLIB = ranlib

% ::	%.m4
	m4 $(M4FLAGS) <$< >$@

(%.o): %.o
	$(AR) cr $@ $%
	$(RANLIB) $@

CPPFLAGS = -I$(srcdir) -I$(CONF) -I$(EXCOMMON) -I$(COMMON) -I.  -DHAVE_CONFIG_H

vpath %.h $(EXCOMMON):$(COMMON):$(CONF)
vpath %.stamp $(CONF)

.PHONY: clean tiefenrein zuerst xdepend FORCE
zuerst: all

%.stamp:
	touch $(CONF)/$@

config.make:
	touch $(CONF)/$@

tiefenrein:
	$(foreach m,$(wildcard */Makefile),(cd $(patsubst %/,%,$(dir $m));$(MAKE) clean);)
	-$(RM) *.o *.a

clean: tiefenrein

xdepend:
	cp Makefile Makefile.bak
	sed -e '/^# DO NOT DELETE THIS LINE/,$$d' < Makefile.bak > Makefile
	echo '# DO NOT DELETE THIS LINE' >> Makefile
	echo ' ' >> Makefile
	for i in $(depfiles) ; do \
	  echo checking $$i ; \
	  $(CC) -M $(CPPFLAGS) $(srcdir)/$$i >> Makefile ; \
	done

