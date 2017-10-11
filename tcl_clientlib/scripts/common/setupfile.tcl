#$Id: setupfile.tcl,v 1.5 2011/11/26 01:55:06 wuestner Exp $
# 
# global_setupfile(default)     default name; derived from display
# global_setupfile(restored)    file used for restore
# global_setupfile(save)        file to be used for save
# global_setupfile(use_default) use default file for save
# static_setupfile(save)
# static_setupfile(use_default)
# static_setupfile(win)
# win_pos(setupfile)
#

#-----------------------------------------------------------------------------#

proc display_name {} {
  global env

# do we use a display?
    if {[string length [info commands .]]==0} {
        # no, we do not have a window
        return {}
    }

# guess SSH tunnel
    # we should check whether it is used or not
    if {[info exists env(SSH_CLIENT)]} {
        set ssh_client [lindex $env(SSH_CLIENT) 0]
        return $ssh_client
    }

# no SSH tunnel; use conventional display
  set display [. cget -screen]
  if {$display==""} {
    set display $env(DISPLAY)
  }
if {0} {
  regsub {\.[^:]*} $display {} display
  if [string match *:? $display] {
    set display $display.0
  }
}
  return $display
}

#-----------------------------------------------------------------------------#
if {0} {
    proc setupfile_name {key} {
      regsub -all { } "~/.[winfo name .]rc_$key" {} setup_file
      return $setup_file
    }
} else {
    proc setupfile_name {key} {
        global argv0

        set myname [file tail $argv0]
        set setup_file ~/.${myname}rc
        if {[string length $key]>0} {
            append setup_file _$key
        }
        return $setup_file
    }
}
#-----------------------------------------------------------------------------#

proc find_setupfile {name key default} {
# teste $name:
  puts "search for $name"
  set file [glob -nocomplain -- $name]
  if {[llength $file]>0} {
    return [lindex $file 0]
  }
# teste $name ohne key:
  regsub -- _$key $name {} n
  puts "not found; search for $n"
  set file [glob -nocomplain -- $n]
  if {[llength $file]>0} {
    return [lindex $file 0]
  }
# # teste $name ohne key und ohne ':.+\..+'
#   regsub -- $key $name {} n
#   regsub {:.+\..+} $n {} n
#   puts "not found; search for $n"
#   set file [glob -nocomplain -- $n]
#   if {[llength $file]>0} {
#     return [lindex $file 0]
#   }
# return default
  puts "not found; will try $default"
  return "$default"
}

#-----------------------------------------------------------------------------#

proc save_setup_default {} {
  global global_setupfile

  if {$global_setupfile(use_default)} {
    save_setup $global_setupfile(default)
  } else {
    save_setup $global_setupfile(save)
  }
}

#-----------------------------------------------------------------------------#

proc restore_setup_default {} {
  global global_setupfile
puts "global_setupfile(use_default): $global_setupfile(use_default)"
  if {$global_setupfile(use_default)} {
    restore_setup $global_setupfile(restored)
  } else {
    restore_setup $global_setupfile(save)
  }
}

#-----------------------------------------------------------------------------#

proc setupfile_reset {} {
  global static_setupfile global_setupfile

  set static_setupfile(save)        $global_setupfile(save)
  set static_setupfile(use_default) $global_setupfile(use_default)
}

#-----------------------------------------------------------------------------#

proc setupfile_apply {} {
  global static_setupfile global_setupfile

  set global_setupfile(save)        $static_setupfile(save)
  set global_setupfile(use_default) $static_setupfile(use_default)
}

#-----------------------------------------------------------------------------#

proc setupfile_ok {} {
  global static_setupfile

  setupfile_apply
  wm withdraw $static_setupfile(win)
}

#-----------------------------------------------------------------------------#

proc setupfile_cancel {} {
  global static_setupfile

  wm withdraw $static_setupfile(win)
}

#-----------------------------------------------------------------------------#

proc setupfile_win_open {} {
  global win_pos static_setupfile

  if {![winfo exists .xh_setupfile]} {setupfile_win_create}
  if $win_pos(setupfile) then {wm positionfrom $static_setupfile(win) user}
  setupfile_reset
  wm deiconify $static_setupfile(win)
  set win_pos(setupfile) 1
  }

#-----------------------------------------------------------------------------#

proc setupfile_win_create {} {
  global global_setupfile static_setupfile win_pos

  set win_pos(setupfile) 0
  set self .xh_setupfile
  set static_setupfile(win) $self
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] setup file"
  wm maxsize $self 10000 10000

  frame $self.from -relief raised -borderwidth 1
  frame $self.gridframe -relief raised -borderwidth 1
  frame $self.b_frame -relief raised -borderwidth 1

# fromframe
  set static_setupfile(from) "setup read from: $global_setupfile(restored)"
  entry $self.from.f -textvariable static_setupfile(from)\
      -state disabled -relief flat -width 0
  pack $self.from.f -fill x -expand 1

# gridframe
  set static_setupfile(to) "save setup to:"
  entry $self.gridframe.t -textvariable static_setupfile(to)\
      -state disabled -relief flat -width 0
  radiobutton $self.gridframe.r1 -variable static_setupfile(use_default)\
      -value 1
  radiobutton $self.gridframe.r2 -variable static_setupfile(use_default)\
      -value 0
  entry $self.gridframe.e1 -textvariable global_setupfile(default) -width 0\
      -state disabled -relief flat
  entry $self.gridframe.e2 -textvariable static_setupfile(save)
  grid $self.gridframe.t -row 0 -column 0 -columnspan 2 -sticky w
  grid $self.gridframe.r1 -row 1 -column 0 -sticky w
  grid $self.gridframe.e1 -row 1 -column 1 -sticky we
  grid $self.gridframe.r2 -row 2 -column 0 -sticky w
  grid $self.gridframe.e2 -row 2 -column 1 -sticky we
  
# buttonframe
  button $self.b_frame.ok -text OK -command "setupfile_ok"
  button $self.b_frame.a -text Apply -command "setupfile_apply"
  button $self.b_frame.r -text Reset -command "setupfile_reset"
  button $self.b_frame.c -text Cancel -command "setupfile_cancel"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
      -side left -fill both -padx 2m -pady 2m -expand 1

  pack $self.from -side top -fill x -expand 1
  pack $self.b_frame -side bottom -fill both -expand 1
  pack $self.gridframe -side left -fill x -expand 1
}
