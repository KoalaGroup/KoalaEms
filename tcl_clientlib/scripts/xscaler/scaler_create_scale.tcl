# $ZEL: scaler_create_scale.tcl,v 1.2 2011/11/26 01:59:16 wuestner Exp $
# copyright 1998-2011
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#

proc scale_command {d} {
  global global_setup
  
  puts "scale: $d"
  puts "interval: $global_setup(readout_repeat)"
}

proc create_scale {frame} {
  global global_setup global_scale

  set self [string trimright $frame .].scale
  set global_scale(win) $self
  scale $self -orient horizontal\
      -from 100\
      -to 60000\
      -label {Interval/ms:}\
      -resolution 100\
      -showvalue 1\
      -variable global_setup(readout_repeat)\
      -font -*-courier-medium-r-*-*-12-*-*-*-*-*-*-*\
      -length 10c
      
  pack $self -fill x -expand 1
}
