# $ZEL: scaler_option_win.tcl,v 1.4 2011/01/13 20:14:35 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#

proc options_reset {} {
  global global_setup static_options

  set static_options(panes)             $global_setup(num_panes)
  set static_options(display_repeat)    $global_setup(display_repeat)
  set static_options(show_index)        $global_setup(show_index)
  set static_options(show_name)         $global_setup(show_name)
  set static_options(show_content)      $global_setup(show_content)
  set static_options(show_rate)         $global_setup(show_rate)
  set static_options(show_max_rate)     $global_setup(show_max_rate)
  set static_options(view_deadtime)     $global_setup(view_deadtime)
}

proc options_apply {} {
  global global_setup static_options

  set global_setup(num_panes)         $static_options(panes)
  set global_setup(display_repeat)    $static_options(display_repeat)
  set global_setup(show_index)        $static_options(show_index)
  set global_setup(show_name)         $static_options(show_name)
  set global_setup(show_content)      $static_options(show_content)
  set global_setup(show_rate)         $static_options(show_rate)
  set global_setup(show_max_rate)     $static_options(show_max_rate)
  set global_setup(view_deadtime)     $static_options(view_deadtime)
  change_display
}

proc options_ok {window} {
  options_apply
  wm withdraw $window
}

proc options_cancel {window} {
  options_reset
  wm withdraw $window
}

proc option_win_open {} {
  global win_pos static_options

  if {![winfo exists .scaler_options]} {option_win_create}
  if $win_pos(options) then {wm positionfrom $static_options(win) user}
  options_reset
  wm deiconify $static_options(win)
  set win_pos(options) 1
}

proc option_win_create {} {
  global win_pos static_options

  set win_pos(options) 0
  set self .scaler_options
  set static_options(win) $self
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] options"
  wm maxsize $self 10000 10000

  frame $self.numberframe -relief raised -borderwidth 1
  frame $self.numberframe.f -borderwidth 0

  label $self.numberframe.f.l_panes -text "# of panes:" -anchor w
  entry $self.numberframe.f.e_panes -textvariable static_options(panes)\
      -width 10
  label $self.numberframe.f.l_repeat -text "display interval (s):" -anchor w
  entry $self.numberframe.f.e_repeat -textvariable static_options(display_repeat)\
      -width 10
#   label $self.numberframe.f.l_channels -text "# of channels:" -anchor w
#   entry $self.numberframe.f.e_channels -textvariable static_options(channels)\
#       -width 10
#  pack $self.numberframe.f.l_panes $self.numberframe.f.e_panes -side left
  grid $self.numberframe.f.l_panes $self.numberframe.f.e_panes -sticky w
  grid $self.numberframe.f.l_repeat $self.numberframe.f.e_repeat -sticky w
  pack $self.numberframe.f -side left
  frame $self.checkframe -relief raised -borderwidth 1

  checkbutton $self.checkframe.index -text "show Index"\
      -underline 5 \
      -variable static_options(show_index)
  checkbutton $self.checkframe.name -text "show Name"\
      -underline 5 \
      -variable static_options(show_name)
  checkbutton $self.checkframe.content -text "show Content"\
      -underline 5 \
      -variable static_options(show_content)
  checkbutton $self.checkframe.rate -text "show Rate"\
      -underline 5 \
      -variable static_options(show_rate)
  checkbutton $self.checkframe.max -text "show Maximum Rate"\
      -underline 7 \
      -variable static_options(show_max_rate)
  checkbutton $self.checkframe.dead -text "show DeadTime"\
      -underline 5 \
      -variable static_options(view_deadtime)
  pack $self.checkframe.index $self.checkframe.name $self.checkframe.content\
      $self.checkframe.rate $self.checkframe.max $self.checkframe.dead\
      -anchor w

  frame $self.b_frame -relief raised -borderwidth 1

  button $self.b_frame.ok -text OK -command "options_ok $self"
  button $self.b_frame.a -text Apply -command "options_apply"
  button $self.b_frame.r -text Reset -command "options_reset"
  button $self.b_frame.c -text Cancel -command "options_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
      -side left -fill both -padx 2m -pady 2m -expand 1

  pack $self.b_frame -side bottom -fill both -expand 1
  pack $self.numberframe $self.checkframe -side top -fill x -anchor w
}
