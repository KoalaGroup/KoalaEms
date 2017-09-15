#
# global vars in this file:
#
# global_${ved}
# global_${ved}(mlist_win)
# global_mtypes
#

proc mlist_down {ved} {
  global global_${ved}
  set win [set global_${ved}(mlist_text)]
  set mlist {}
  set i 1
  set oline [$win get $i.0 [expr $i+1].0]
  regsub {#.*} $oline {} line
  set line [string trim $line]
  while {"$line"!=""} {
    set sp [string first {:} $line]
    if {$sp==-1} {
      set sp [string first { } $line]
    }
    if {$sp==-1} {
      si_dialog .error {Error} "no separator in $oline" error
      return
    }
    set addr [string trim [string range $line 0 [expr $sp-1]]]
    set mod  [string trim [string range $line [expr $sp+1] end]]
    lappend pair $addr $mod
    lappend mlist $pair
    unset pair
    incr i
    set oline [$win get $i.0 [expr $i+1].0]
    regsub {#.*} $oline {} line
    set line [string trim $line]
  }
  if [catch {$ved modullist create $mlist} mist] {
    si_dialog .error {Error} $mist error
  }
}

proc mlist_up {ved} {
  global global_${ved}
  global global_mtypes
  if [catch {set erg [$ved modullist]} mist] {
    si_dialog .error {Error} $mist error
  } else {
    set win [set global_${ved}(mlist_text)]
    $win delete 1.0 end
    foreach i $erg {
      set addr [lindex $i 0]
      set mod [lindex $i 1]
      set addr "[string range {        } [string length $addr] 9]$addr"
      set name [modultypename $mod]
      if {$name!=""} {
        $win insert end "$addr: $mod  # $name\n"
      } else {
        $win insert end "$addr: $mod\n"
      }
    }
  }
}

proc mlist_delete {ved} {
  global global_${ved}
  if [catch {$ved modullist delete} mist] {
    si_dialog .error {Error} $mist error
  }
}

proc mlist_dismiss {ved} {
  global global_${ved}
  wm withdraw [set global_${ved}(mlist_win)]
}

proc mlist_reset {ved} {
  global global_${ved}
}

proc mlist_select {ved} {
  global global_${ved}
  set line [selection get]
  regsub {\ +} $line {  #} nline
  set win [set global_${ved}(mlist_text)]
  $win insert end $nline
}

proc modultypename {id} {
  global global_mtypes
  if {[info exists global_mtypes] == 0} {
    load_modultypes
  }
  foreach i [array names global_mtypes] {
    if {$i==$id} {
      return [set global_mtypes($i)]
    }
  }
  return {}
}

proc mlist_fill_modbox {win ved} {
  global global_mtypes
  if {[info exists global_mtypes] == 0} {
    load_modultypes
  }
  foreach i [array names global_mtypes] {
    set line "$i [set global_mtypes($i)]"
    $win insert end $line
  }
}

proc modullist_win_create {self ved} {
  global global_${ved} global_setup

  set global_${ved}(mlist_win) $self
  toplevel $self
  wm withdraw $self
  wm group $self .

  frame $self.listframe -relief raised -borderwidth 1
  frame $self.modframe -relief raised -borderwidth 1
  frame $self.b1_frame -relief raised -borderwidth 1
  frame $self.b2_frame -relief raised -borderwidth 1

# listframe
  frame $self.listframe.lb -relief flat -borderwidth 0
  label $self.listframe.lb.l -text "Modullist:" -anchor w
  button $self.listframe.lb.b -text "Clear"\
      -command "$self.listframe.t delete 1.0 end"
  text $self.listframe.t -width 0 -height 10\
      -font $global_setup(slist_font)\
      -yscrollcommand "$self.listframe.s set"\
      -wrap none
  set global_${ved}(mlist_text) $self.listframe.t
  scrollbar $self.listframe.s -command "$self.listframe.t yview"
  pack $self.listframe.lb.l -side left
  pack $self.listframe.lb.b -side right
  pack $self.listframe.lb -side top -padx 1m -pady 1m -fill x -expand 0
  pack $self.listframe.t -side left -fill both -expand 1
  pack $self.listframe.s -side right -fill y

# modframe
  label $self.modframe.l -text "Known Modules:" -anchor w
  listbox $self.modframe.t -width 0 -font $global_setup(list_font)
  mlist_fill_modbox $self.modframe.t $ved
  bind $self.modframe.t <ButtonRelease-1> "mlist_select $ved"
  bind $self.modframe.t <KeyPress-Return> "mlist_select $ved"
  bind $self.modframe.t <KeyPress-KP_Enter> "mlist_select $ved"
  pack $self.modframe.l -side top -padx 1m -pady 1m -fill x -expand 0
  pack $self.modframe.t -side left -fill both -expand 1
  if {[$self.modframe.t size]<=10} {
    $self.modframe.t config -height 0
  } else {
    $self.modframe.t config -height 10 -yscrollcommand "$self.modframe.s set"
    scrollbar $self.modframe.s -command "$self.modframe.t yview"
    pack $self.modframe.s -side right -fill y
  }

# b1_frame
  button $self.b1_frame.down -text Download\
      -command "mlist_down $ved"
  button $self.b1_frame.up -text Upload\
      -command "mlist_up $ved"
  button $self.b1_frame.delete -text Delete\
      -command "mlist_delete $ved"
  pack $self.b1_frame.down $self.b1_frame.up $self.b1_frame.delete\
      -side left -fill both -padx 1m -pady 1m -expand 1

# b2_frame
  button $self.b2_frame.dismiss -text Dismiss\
      -command "mlist_dismiss $ved"
  button $self.b2_frame.reset -text Reset\
      -command "mlist_reset $ved"
  pack $self.b2_frame.dismiss $self.b2_frame.reset\
      -side left -fill both -padx 1m -pady 1m -expand 1

  pack $self.listframe -side top -fill both -expand 1
  pack $self.modframe -side top -fill both -expand 0
  pack $self.b2_frame -side bottom -fill x -expand 0
  pack $self.b1_frame -side bottom -fill x -expand 0
}

proc modullist_win_open {ved} {
# global global_${ved}
  set self .${ved}_mlist
  if [winfo exists $self] {
    wm positionfrom $self user
  } else {
    modullist_win_create $self $ved
  }
  wm deiconify $self
}
