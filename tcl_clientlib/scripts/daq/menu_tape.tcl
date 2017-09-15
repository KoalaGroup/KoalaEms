# $ZEL: menu_tape.tcl,v 1.7 2003/02/04 19:27:54 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# Diese Prozedur erzeugt das Tapemenu im Hauptmenu
#

proc create_menu_tape {parent} {
  set self [string trimright $parent .].tape
  menu $self -title "DAQ tape"

#   $self add command -label "wind" -underline 0 \
#       -command tape_wind
# 
  $self add command -label "disable" -underline 0 \
      -command tape_disable

  $self add command -label "enable" -underline 0 \
      -command tape_enable

  $self add command -label "wind to end of data" -underline 0 \
      -command tape_wind_eod

  $self add command -label "write filemark" -underline 0 \
      -command tape_mark

  $self add command -label "rewind" -underline 0 \
      -command tape_rewind

  $self add command -label "load" -underline 0 \
      -command tape_load 

  $self add command -label "unload" -underline 0 \
      -command tape_unload

  $self add command -label "erase" -underline 0 \
      -command tape_erase

  return $self
  }
