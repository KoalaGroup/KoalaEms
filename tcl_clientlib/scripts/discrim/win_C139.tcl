# $Id: win_C139.tcl,v 1.4 2005/04/06 19:28:49 wuestner Exp $
#

proc create_C193_win {slot} {
  global global_moduls global_ved

  set space mod_C193_$slot
  lappend global_moduls $space
  namespace eval $space {
  }
  set ${space}::ved $global_ved
  set ${space}::slot $slot
  set ${space}::type C193
  
  set self [frame .frame_C193_$slot -border 2 -relief ridge]
  frame $self.head -border 2 -relief ridge
  frame $self.channels -border 2 -relief ridge
  frame $self.common -border 2 -relief ridge
  frame $self.cbuttons
  frame $self.buttons

  label $self.head.type -text C193 -anchor w
  label $self.head.slot_l -text "Slot"
  set ${space}::realslot [$global_ved command1 nAFslot $slot]
  entry $self.head.slot_e -state readonly -textvariable ${space}::realslot \
      -relief flat -width 2
  set efont [lindex [$self.head.slot_e configure -font] 4]
  pack $self.head.type -side top -fill x
  pack $self.head.slot_l -side left -fill x
  pack $self.head.slot_e -side left -fill x

  global chan_C193_$slot
  set chan_C193_${slot}(all) 0
  label $self.common.l_all -font $efont -text all -anchor e
  entry $self.common.e_all -textvariable chan_C193_${slot}(all)\
        -justify right -width 4
  set ${space}::chan_ent(all) $self.common.e_all
  grid $self.common.l_all $self.common.e_all -sticky ew
  button $self.cbuttons.write -text {Write all} -command "modul_C193_write_all\
    $space"
  pack $self.cbuttons.write -side top -fill x -expand 1
  
  for {set i 0} {$i<32} {incr i} {
    label $self.channels.l_$i -font $efont -text $i -anchor e
    entry $self.channels.e_$i -textvariable chan_C193_${slot}($i)\
        -justify right -width 4
    set ${space}::chan_ent($i) $self.channels.e_$i
    grid $self.channels.l_$i $self.channels.e_$i -sticky ew
  }

  button $self.buttons.reread -text Reread -command "modul_C193_init $space"
  button $self.buttons.write -text Write -command "modul_C193_write $space"
  pack $self.buttons.write $self.buttons.reread -side top -fill x -expand 1

  pack $self.head -side top -fill x -expand 0
  pack $self.common -side top -fill x -expand 0
  pack $self.cbuttons -side top -fill x -expand 0
  pack $self.channels -side top -fill x -expand 0
  pack $self.buttons -side bottom -fill x -expand 0
  pack $self -side left -fill both -expand 1
}

proc modul_C193_init {space} {
  set ved [set ${space}::ved]
  set slot [set ${space}::slot]
  set slot1 [expr $slot+1]
  
  global chan_C193_$slot
  for {set i 0} {$i<16} {incr i} {
    set res [$ved command1 nAFread $slot $i 0]
    set chan_C193_${slot}($i) [expr $res&0xff]
  }
  for {set i 0} {$i<16} {incr i} {
    set res [$ved command1 nAFread $slot1 $i 0]
    set chan_C193_${slot}([expr $i+16]) [expr $res&0xff]
  }
  for {set i 0} {$i<32} {incr i} {
    modul_C193_color $space $i #d9d9d9
  }
}

proc modul_C193_color {space chan color} {
  [set ${space}::chan_ent($chan)] config -bg $color
  update idletasks
}

proc modul_C193_write_all {space} {
  set ved [set ${space}::ved]
  set slot [set ${space}::slot]
  global chan_C193_$slot
  modul_C193_color $space all yellow
  set val [set chan_C193_${slot}(all)]
  if {$val<0} {set val 0}
  if {$val>255} {set val 255}
  set res [$ved command1 nAFwrite_q $slot 1 17 $val]
  set count 0
  while {($count<100) && (([$ved command1 nAFread $slot 0 0]&0x80000000)==0)} {
    after 100
    incr count
  }
  modul_C193_color $space all #d9d9d9
  modul_C193_init $space
}

proc modul_C193_write {space} {
    global global_setup
  set ved [set ${space}::ved]
  set slot [set ${space}::slot]
  set slot1 [expr $slot+1]

  global chan_C193_$slot

  for {set i 0} {$i<32} {incr i} {
    modul_C193_color $space $i yellow
  }

  $ved command1 CCCI $global_setup(crate) 0
  for {set i 0} {$i<32} {incr i} {
    set val [set chan_C193_${slot}($i)]
    if {$val<0} {set val 0}
    if {$val>255} {set val 255}
    if {$i<16} {
      set s $slot
      set c $i
    } else {
      set s [expr $slot+1]
      set c [expr $i-16]
    }
    set oldval [expr [$ved command1 nAFread $s $c 0]&0xff]
    if {$val!=$oldval} {
      set res [$ved command1 nAFwrite_q $s $c 16 $val]
      set count 0
      while {($count<100) && (([$ved command1 nAFread $s $c 0]&0x80000000)==0)} {
        after 100
        incr count
      }
      set res [$ved command1 nAFread $s $c 0]
      if {($res&0xff)!=$val} {
        puts "chan $i: $val-->[format {%08x} $res]"
        modul_C193_color $space $i red
      } else {
        modul_C193_color $space $i green3
      }
    } else {
      modul_C193_color $space $i green
    }
  }
  dump_thresholds
}
