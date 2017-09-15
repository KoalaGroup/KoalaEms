#
# static_autostart(text)
# global_autostart()
# static_autostart(autostart)
# global_setup(autostart)
# 
# 
#
#-----------------------------------------------------------------------------#

proc autostart_reset {} {
  global global_setup global_autostart static_autostart

  $static_autostart(text) delete 1.0 end
  if {[info exists global_autostart]} {
    foreach i [array names global_autostart] {
      $static_autostart(text) insert end "$global_autostart($i)\n"
    }
  }
  set static_autostart(autostart) $global_setup(autostart)
}

#-----------------------------------------------------------------------------#

proc autostart_apply {} {
  global global_setup global_autostart static_autostart

  ladd global_setup_extra global_autostart

  if {[info exists global_autostart]} {unset global_autostart}
  set idx 1
  set line [$static_autostart(text) get $idx.0 "$idx.0 lineend"]
  while {"$line"!=""} {
    set line [string trimright $line { &}]
    set line [string trimleft $line]
    set global_autostart($idx) $line
    incr idx
    set line [$static_autostart(text) get $idx.0 "$idx.0 lineend"]
  }
  set  global_setup(autostart) $static_autostart(autostart)
}

#-----------------------------------------------------------------------------#

proc autostart_ok {window} {
  autostart_apply
  wm withdraw $window
}

#-----------------------------------------------------------------------------#

proc autostart_exec {} {
  global static_autostart global_setup global_exec

  set idx 1
  set line [$static_autostart(text) get $idx.0 "$idx.0 lineend"]
  while {"$line"!=""} {
    set line [string trimright $line { &}]
    set line [string trimleft $line]
      if {[string index $line 0]!="#"} {
        regsub {%port} $line $global_setup(port) line
        regsub {%host} $line [exec hostname] line
        if [catch {set pid [eval "exec $line >&/dev/console </dev/null &"]} mist] {
          bgerror $mist
        } else {
          foreach id $pid {lappend global_exec $id}
        }
      }
    incr idx
    set line [$static_autostart(text) get $idx.0 "$idx.0 lineend"]
  }
}

#-----------------------------------------------------------------------------#

proc autostart_ok {window} {
  autostart_apply
  wm withdraw $window
}

#-----------------------------------------------------------------------------#

proc autostart_cancel {window} {
  autostart_reset
  wm withdraw $window
}

#-----------------------------------------------------------------------------#

proc autostart_win_open {} {
  global win_pos

  if {![winfo exists .autostart]} {create_autostart_win}
  if $win_pos(autostart) then {wm positionfrom .autostart user}
  autostart_reset
  wm deiconify .autostart
  set win_pos(autostart) 1
  }

#-----------------------------------------------------------------------------#

proc create_autostart_win {} {
  global global_setup win_pos static_autostart
   
  set win_pos(autostart) 0
  set self .autostart
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] autostart"
  wm maxsize $self 10000 10000

  frame $self.titleframe -relief raised -borderwidth 1
  frame $self.commandframe -relief raised -borderwidth 1
  frame $self.autoframe -relief raised -borderwidth 1
   
# titleframe
  label $self.titleframe.l -text "Commands:" -anchor w
  pack $self.titleframe.l -side top -padx 1m -fill x -expand 1

# commandframe
  frame $self.commandframe.o -borderwidth 0
  frame $self.commandframe.u -borderwidth 0
  set static_autostart(text) [text $self.commandframe.o.t -width 0 -height 5\
      -font $global_setup(list_font)\
      -xscrollcommand "$self.commandframe.u.s set"\
      -yscrollcommand "$self.commandframe.o.s set"\
      -wrap none]
  scrollbar $self.commandframe.o.s -command "$self.commandframe.o.t yview"
  pack $self.commandframe.o.t -side left -fill both -expand 1
  pack $self.commandframe.o.s -side right -fill y
  set width [expr [$self.commandframe.o.s cget -width]+\
      2*[$self.commandframe.o.s cget -borderwidth]\
      +2*[$self.commandframe.o.s cget -highlightthickness]]
  frame $self.commandframe.u.dummy -relief flat -borderwidth 0 \
      -width $width -height $width
  scrollbar $self.commandframe.u.s -command "$self.commandframe.o.t xview"\
      -orient horizontal
  pack $self.commandframe.u.dummy -side right
  pack $self.commandframe.u.s -side left -fill both -expand 1
  pack $self.commandframe.o -side top -fill both -expand 1
  pack $self.commandframe.u -side top -fill x -expand 1

# autoframe
  checkbutton $self.autoframe.c\
      -text "Enable Automatic Execution"\
      -variable static_autostart(autostart)\
      -selectcolor black
  pack $self.autoframe.c -side left -padx 1m -pady 1m -fill x

# startframe
  frame $self.startframe -relief raised -borderwidth 1
  button $self.startframe.exec -text "Execute now"\
      -command "autostart_exec"
  pack $self.startframe.exec\
      -side left -fill both -padx 2m -pady 2m -expand 1

# buttonframe
  frame $self.b_frame -relief raised -borderwidth 1
  button $self.b_frame.ok -text OK -command "autostart_ok $self"
  button $self.b_frame.a -text Apply -command "autostart_apply"
  button $self.b_frame.r -text Reset -command "autostart_reset"
  button $self.b_frame.c -text Cancel -command "autostart_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
      -side left -fill both -padx 2m -pady 2m -expand 1

  pack $self.titleframe $self.commandframe -side top -fill both
  pack $self.autoframe -side top -fill both -expand 1
  pack $self.startframe -side top -fill both -expand 1
  pack $self.b_frame -side bottom -fill both -expand 1
}

#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
