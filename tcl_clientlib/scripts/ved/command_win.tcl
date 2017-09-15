#
# global vars in this file:
#
# array global_${ved}    ???
#
#

proc comm_ok {ved} {
  global global_${ved}
}

proc comm_apply {ved} {
  global global_${ved}
  set win [set global_${ved}(command_text)]
  set i 1
  set line [$win get $i.0 [expr $i+1].0]
  set line [string trim $line]
  while {"$line"!=""} {
    set sp [string first { } $line]
    if {$sp==-1} {
      lappend proclist $line
      lappend proclist {}
    } else {
      lappend proclist [string range $line 0 [expr $sp-1]]
      lappend proclist [string range $line [expr $sp+1] end]
    }
    incr i
    set line [$win get $i.0 [expr $i+1].0]
    set line [string trim $line]
  }
  set erg [$ved command $proclist]
  set n 0
  foreach i $erg {
    if {$n==0} {
      output $i
      incr n
    } else {
      output_append $i
    }
  }
}

proc comm_reset {ved} {
  global global_${ved}
}

proc comm_cancel {ved} {
  global global_${ved}
  wm withdraw [set global_${ved}(command_win)]
}

proc comm_select {ved} {
  global global_${ved}
  set line [selection get]
  regsub {.*\ +} $line {} nline
  set nline "$nline "
  set win [set global_${ved}(command_text)]
  $win insert end $nline
}

proc comm_fill_capbox {win ved} {
  if [catch {
    $ved version_separator .
    set erg [$ved caplist]
    } mist] {
    bgerror $mist
  } else {
    foreach i $erg {
      $win insert end $i
    }
  }
}

proc docommand_win_create {self ved} {
  global global_${ved} global_setup

  set global_${ved}(command_win) $self
  toplevel $self
  wm withdraw $self
  wm group $self .

  frame $self.commframe -relief raised -borderwidth 1
  frame $self.capframe -relief raised -borderwidth 1

  frame $self.b_frame -relief raised -borderwidth 1

# commframe
  frame $self.commframe.lb -relief flat -borderwidth 0
  label $self.commframe.lb.l -text "Commandlist:" -anchor w
  button $self.commframe.lb.b -text "Clear"\
      -command "$self.commframe.t delete 1.0 end"
  text $self.commframe.t -width 0 -height 4\
      -yscrollcommand "$self.commframe.s set"
  set global_${ved}(command_text) $self.commframe.t
  scrollbar $self.commframe.s -command "$self.commframe.t yview"
  pack $self.commframe.lb.l -side left
  pack $self.commframe.lb.b -side right
  pack $self.commframe.lb -side top -padx 1m -pady 1m -fill x -expand 0
  pack $self.commframe.t -side left -fill both -expand 1
  pack $self.commframe.s -side right -fill y

# capframe
  label $self.capframe.l -text "Available Procedures:" -anchor w
  listbox $self.capframe.t -width 0 -font $global_setup(list_font)
  comm_fill_capbox $self.capframe.t $ved
  bind $self.capframe.t <ButtonRelease-1> "comm_select $ved"
  bind $self.capframe.t <KeyPress-Return> "comm_select $ved"
  bind $self.capframe.t <KeyPress-KP_Enter> "comm_select $ved"
  pack $self.capframe.l -side top -padx 1m -pady 1m -fill x -expand 0
  pack $self.capframe.t -side left -fill both -expand 1
  if {[$self.capframe.t size]<=10} {
    $self.capframe.t config -height 0
  } else {
    $self.capframe.t config -height 10 -yscrollcommand "$self.capframe.s set"
    scrollbar $self.capframe.s -command "$self.capframe.t yview"
    pack $self.capframe.s -side right -fill y
  }

# b_frame
  button $self.b_frame.ok -text Ok\
      -command "comm_ok $ved"
  button $self.b_frame.apply -text Apply\
      -command "comm_apply $ved"
  button $self.b_frame.reset -text Reset\
      -command "comm_reset $ved"
  button $self.b_frame.cancel -text Cancel\
      -command "comm_cancel $ved"
  pack $self.b_frame.ok $self.b_frame.apply $self.b_frame.reset\
      $self.b_frame.cancel -side left -fill both -padx 1m -pady 1m -expand 1

  pack $self.commframe -side top -fill both -expand 1
  pack $self.capframe -side top -fill both -expand 0
  pack $self.b_frame -side bottom -fill x -expand 0
}

proc docommand_win_open {ved} {
# global global_${ved}
  set self .${ved}_comm
  if [winfo exists $self] {
    wm positionfrom $self user
  } else {
    docommand_win_create $self $ved
  }
  wm deiconify $self
}
