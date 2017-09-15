# $Id: modul_init.tcl,v 1.2 2005/04/06 19:28:47 wuestner Exp $
#

proc modul_init {space} {
puts "init $space [set ${space}::type]"
  switch [set ${space}::type] {
    C193 {modul_C193_init $space}
    4413 {modul_4413_init $space}
  }
}
