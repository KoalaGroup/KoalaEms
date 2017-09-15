#
# global vars in this file:
#
# boolean global_setup(offline)
# int     global_${ved}(var_idx)
# int     global_${ved}(var_size)
# window  global_${ved}(var_win)
#

proc var_list_update {win ved} {
  $win.exframe.t delete 0 end
  set n [$ved namelist var]
  foreach i $n {
    $win.exframe.t insert end $i
  }
  if {[$win.exframe.t size]<=10} {
    $win.exframe.t config -height 0
    pack forget $win.exframe.s
  } else {
    $win.exframe.t config -height 10
    pack $win.exframe.s -side right -fill y
  }
}

proc var_create {ved} {
  global global_${ved}

  set idx [set global_${ved}(var_idx)]
  set size [set global_${ved}(var_size)]
  if [catch {$ved var create $idx $size} mist] {
    si_dialog .error {Error} $mist error
  } else {
    var_list_update [set global_${ved}(var_win)] $ved
  }
}

proc var_delete {ved} {
  global global_${ved}

  set idx [set global_${ved}(var_idx)]
  if [catch {$ved var delete $idx} mist] {
    si_dialog .error {Error} $mist error
  } else {
    var_list_update [set global_${ved}(var_win)] $ved
  }
}

proc var_read {ved} {
  global global_${ved}

  set idx [set global_${ved}(var_idx)]
  set size [set global_${ved}(var_size)]
  if [catch {set val [$ved var read $idx]} mist] {
    si_dialog .error {Error} $mist error
  } else {
    set win [set global_${ved}(var_win)].valframe.t
    $win delete 1.0 end
    set k 0
    foreach i $val {
      $win insert end "\[$k\]   $i\n"
      incr k
    }
  }
}

proc var_write {ved} {
  global global_${ved}

  set idx [set global_${ved}(var_idx)]
  set win [set global_${ved}(var_win)].valframe.t
  set i 1
  set list {}
  set line [$win get $i.0 [expr $i+1].0]
  set line [string trimright $line \n]
  set x [regsub {\[.*\]\ *} $line {} nline]
  while {"$line"!=""} {
    set list [lappend list $nline]
    incr i
    set line [$win get $i.0 [expr $i+1].0]
    set line [string trim $line "\n "]
    set x [regsub {\[.*\]\ *} $line {} nline]
  }
  incr i -1
  set global_${ved}(var_size) $i
  if [catch {$ved var write $idx $list} mist] {
    si_dialog .error {Error} $mist error
  }
}

proc var_select {ved} {
  global global_${ved}
  set line [selection get]
  set global_${ved}(var_idx) $line
  set global_${ved}(var_size) [$ved var size $line]
}

proc var_win_create {self ved} {
  global global_${ved}

  set global_${ved}(var_win) $self
  toplevel $self
  wm withdraw $self
  wm group $self .

  frame $self.idf -relief flat -borderwidth 0
  frame $self.idf.idframe -relief raised -borderwidth 1
  frame $self.idf.sizeframe -relief raised -borderwidth 1
  frame $self.valframe -relief raised -borderwidth 1
  frame $self.exframe -relief raised -borderwidth 1
  frame $self.b1_frame -relief raised -borderwidth 1
  frame $self.b2_frame -relief raised -borderwidth 1

# idf
  set global_${ved}(var_idx) 0
  set global_${ved}(var_size) 1
  label $self.idf.idframe.l -text "Index" -anchor w
  entry $self.idf.idframe.e -textvariable global_${ved}(var_idx)
  label $self.idf.sizeframe.l -text "Size" -anchor w
  entry $self.idf.sizeframe.e -textvariable global_${ved}(var_size)
  pack $self.idf.idframe.l -side top -padx 1m -fill x -expand 1
  pack $self.idf.idframe.e -side top -padx 1m -fill x -expand 1
  pack $self.idf.sizeframe.l -side top -padx 1m -fill x -expand 1
  pack $self.idf.sizeframe.e -side top -padx 1m -fill x -expand 1
  pack $self.idf.idframe -side left -fill x -expand 1
  pack $self.idf.sizeframe -side left -fill x -expand 1

# valframe
  label $self.valframe.l -text "Value" -anchor w
  text $self.valframe.t -width 0 -height 0\
      -yscrollcommand "$self.valframe.s set"
  scrollbar $self.valframe.s -command "$self.valframe.t yview"
  pack $self.valframe.l -side top -padx 1m -pady 1m -fill x -expand 0
  pack $self.valframe.t -side left -fill both -expand 1
  pack $self.valframe.s -side right -fill y

# exframe
  label $self.exframe.l -text "Existing Variables:" -anchor w
  listbox $self.exframe.t -width 0 -height 0\
      -yscrollcommand "$self.exframe.s set"
  scrollbar $self.exframe.s -command "$self.exframe.t yview"
  bind $self.exframe.t <ButtonRelease-1> "var_select $ved"
  bind $self.exframe.t <KeyPress-Return> "var_select $ved"
  bind $self.exframe.t <KeyPress-KP_Enter> "var_select $ved"
  pack $self.exframe.l -side top -padx 1m -pady 1m -fill x -expand 0
  pack $self.exframe.t -side left -fill both -expand 1
  pack $self.exframe.s -side right -fill y

# b1_frame
  button $self.b1_frame.create -text Create\
      -command "var_create $ved"
  button $self.b1_frame.delete -text Delete\
      -command "var_delete $ved"
  button $self.b1_frame.read -text Read\
      -command "var_read $ved"
  button $self.b1_frame.write -text Write\
      -command "var_write $ved"
  pack $self.b1_frame.create $self.b1_frame.delete $self.b1_frame.read\
      $self.b1_frame.write -side left -fill both -padx 1m -pady 1m -expand 1

# b2_frame
  button $self.b2_frame.dismiss -text Dismiss -command "wm withdraw $self"
  pack $self.b2_frame.dismiss -side left -fill both -padx 1m -pady 1m -expand 1

  pack $self.idf -side top -fill x -expand 0
  pack $self.valframe -side top -fill both -expand 1
  pack $self.exframe -side top -fill both -expand 0
  pack $self.b2_frame -side bottom -fill x -expand 0
  pack $self.b1_frame -side bottom -fill x -expand 0
}

proc var_win_open {ved} {
  set self .${ved}_var
  if [winfo exists $self] {
    wm positionfrom $self user
  } else {
    var_win_create $self $ved
  }
  var_list_update $self $ved
  wm deiconify $self
}
