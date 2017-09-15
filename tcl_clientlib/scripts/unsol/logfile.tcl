#$Id: logfile.tcl,v 1.1 1998/05/18 12:22:56 wuestner Exp $
#
# global_setup(logfilename)
# global_setup(auto_log)
# global_flog               pathdescriptor
# static_logfile(win)
# static_logfile(name)
# win_pos(logfile)
#

#-----------------------------------------------------------------------------#
proc logfile_reset {} {
  global static_logfile global_setup

  set static_logfile(name) $global_setup(logfilename)
}
#-----------------------------------------------------------------------------#
proc logfile_apply {} {
  global static_logfile global_setup global_flog

  if {$global_flog!=""} {
    timestamp 1
    output_append "logfile \"$global_setup(logfilename)\" closed"
    close $global_flog
  }
  set global_setup(logfilename) $static_logfile(name)
  set global_flog ""
  if [catch {set global_flog [open $global_setup(logfilename) \
              {RDWR CREAT} 0640]} mist] {
    output $mist
    set global_flog ""
  } else {
    timestamp 1
    output_append "logfile \"$global_setup(logfilename)\" open"
  }
}
#-----------------------------------------------------------------------------#
proc logfile_ok {} {
  global static_logfile

  logfile_apply
  wm withdraw $static_logfile(win)
}
#-----------------------------------------------------------------------------#
proc logfile_cancel {} {
  global static_logfile

  wm withdraw $static_logfile(win)
}
#-----------------------------------------------------------------------------#
proc logfile_win_open {} {
  global win_pos static_logfile

  if {![winfo exists .unsol_logfile]} {logfile_win_create}
  if $win_pos(logfile) then {wm positionfrom $static_logfile(win) user}
  logfile_reset
  wm deiconify $static_logfile(win)
  set win_pos(logfile) 1
  }
#-----------------------------------------------------------------------------#
proc logfile_win_create {} {
  global static_logfile win_pos

  set win_pos(logfile) 0
  set self .unsol_logfile
  set static_logfile(win) $self
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] logfile file"
  wm maxsize $self 10000 10000

  frame $self.nameframe -relief raised -borderwidth 1
  frame $self.b_frame -relief raised -borderwidth 1

# nameframe
  set static_logfile(to) "name of logfile:"
  label $self.nameframe.t -text "name of logfile:" -relief flat -width 0
  entry $self.nameframe.e -textvariable static_logfile(name)
  pack $self.nameframe.t -side top -fill x
  pack $self.nameframe.e -side top -fill x

# buttonframe
  button $self.b_frame.ok -text OK -command "logfile_ok"
  button $self.b_frame.a -text Apply -command "logfile_apply"
  button $self.b_frame.r -text Reset -command "logfile_reset"
  button $self.b_frame.c -text Cancel -command "logfile_cancel"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
      -side left -fill both -padx 2m -pady 2m -expand 1

  pack $self.b_frame -side bottom -fill both -expand 1
  pack $self.nameframe -side top -fill x -expand 1
}
