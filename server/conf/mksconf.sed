# make text file ready for inclusion into C string
# (strip empty lines, escape quoten & newlines)
# $Id: mksconf.sed,v 1.1 1996/01/17 10:01:31 drochner Exp $

/^$/ d
s!"!\\"!g
s!$!\\n\\!
