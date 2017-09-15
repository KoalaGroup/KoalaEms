# $Id: win_4413.tcl,v 1.3 2005/04/06 19:28:48 wuestner Exp $
#

proc create_4413_win {slot} {
  global global_moduls global_ved

  set space mod_4413_$slot
  lappend global_moduls $space
  namespace eval $space {
    
  }
  set ${space}::ved $global_ved
  set ${space}::slot $slot
  set ${space}::type 4413

  set self [frame .frame_4413_$slot -border 2 -relief ridge]
  frame $self.head -border 2 -relief ridge
  frame $self.common -border 2 -relief ridge
  frame $self.channels -border 2 -relief ridge
  frame $self.buttons

  label $self.head.type -text 4413 -anchor w
  label $self.head.slot_l -text "Slot"
  set ${space}::realslot [$global_ved command1 nAFslot $slot]
  entry $self.head.slot_e -state readonly -textvariable ${space}::realslot \
      -relief flat -width 2
  set efont [lindex [$self.head.slot_e configure -font] 4]
  pack $self.head.type -side top -fill x
  pack $self.head.slot_l -side left -fill x
  pack $self.head.slot_e -side left -fill x

  label $self.common.l_all -font $efont -text thresh. -anchor e
  entry $self.common.e_all -textvariable chan_4413_${slot}(thresh)\
        -justify right -width 5
  grid $self.common.l_all $self.common.e_all -sticky ew

  for {set i 0} {$i<16} {incr i} {
    label $self.channels.l_$i -font $efont -text $i -anchor e
    checkbutton $self.channels.e_$i -variable chan_4413_${slot}($i)\
        -selectcolor black
    grid $self.channels.l_$i $self.channels.e_$i -sticky ew
  }

  button $self.buttons.reread -text Reread -command "modul_4413_init $space"
  button $self.buttons.write -text Write -command "modul_4413_write $space"
  pack $self.buttons.reread $self.buttons.write -side bottom -fill x -expand 1

  pack $self.head -side top -fill x -expand 0
  pack $self.common -side top -fill x -expand 0
  pack $self.channels -side top -fill x -expand 0
  pack $self.buttons -side bottom -fill x -expand 0
  pack $self -side left -fill both -expand 1
}

proc modul_4413_init {space} {
  set ved [set ${space}::ved]
  set slot [set ${space}::slot]
  
  global chan_4413_$slot
  set mask [$ved command1 nAFread $slot 0 0]
  set chan_4413_${slot}(thresh) [expr [$ved command1 nAFread $slot 0 1]&0x3ff]
  for {set i 0} {$i<16} {incr i} {
    if {$mask&(1<<$i)} {
      set chan_4413_${slot}($i) 1
    } else {
      set chan_4413_${slot}($i) 0
    }
  }
}

proc modul_4413_write {space} {
    global global_setup
  set ved [set ${space}::ved]
  set slot [set ${space}::slot]

  $ved command1 CCCI $global_setup(crate) 0
  global chan_4413_$slot
  $ved command1 nAFcntl $slot 0 26
  set thresh [set chan_4413_${slot}(thresh)]
  $ved command1 nAFwrite $slot 0 17 [expr $thresh&0x3ff]
  set mask 0
  for {set i 0} {$i<16} {incr i} {
    set chan [set chan_4413_${slot}($i)]
    if {$chan} {set mask [expr $mask|(1<<$i)]}
  }
  $ved command1 nAFwrite $slot 0 16 $mask
  modul_4413_init $space
  dump_thresholds
}
