# $ZEL: commu_setup.tcl,v 1.7 2003/02/04 19:27:50 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# enum    global_setup(commu_transport) / {default, unix, tcp}
# string  global_setup(commu_host)
# string  global_setup(commu_socket)
# int     global_setup(commu_port)
#
# enum    static_commu(transport)
# string  static_commu(host)
# string  static_commu(socket)
# int     static_commu(port)
# boolean win_pos(commu)         / true: Window ist bereits positioniert
#

proc commu_reset {window} {
  global global_setup static_commu
  set static_commu(transport) $global_setup(commu_transport)
  set static_commu(host)      $global_setup(commu_host)
  set static_commu(socket)    $global_setup(commu_socket)
  set static_commu(port)      $global_setup(commu_port)
  commu_pack $window
}

proc commu_pack {window} {
  global static_commu
  pack forget $window.unixframe $window.tcpframe $window.buttonframe
  pack $window.radioframe -side top -fill x -expand 1
  switch $static_commu(transport) {
    unix {pack $window.unixframe -side top -fill x -expand 1}
    tcp  {pack $window.tcpframe -side top -fill x -expand 1}
  }
  pack $window.b_frame -side bottom -fill both -expand 1
}

proc commu_apply {} {
  global global_setup static_commu
  set global_setup(commu_transport) $static_commu(transport)
  set global_setup(commu_host)      $static_commu(host)
  set global_setup(commu_socket)    $static_commu(socket)
  set global_setup(commu_port)      $static_commu(port)
}

proc commu_ok {window} {
  commu_apply
  wm withdraw $window
}

proc commu_cancel {window} {
  wm withdraw $window
}

proc commu_win_open {} {
  global win_pos static_commu

  if {![winfo exists .commu]} {commu_win_create}
  bind Entry <Delete> {
         tkEntryBackspace %W
  }
  commu_reset $static_commu(win)
  if $win_pos(commu) then {wm positionfrom .commu user}
  update idletasks
  wm deiconify .commu
  set win_pos(commu) 1
}

proc commu_win_create {} {
  global win_pos global_setup static_commu

  set win_pos(commu) 0
  set self .commu
  set static_commu(win) $self
  toplevel $self
  wm group $self .
  wm title $self "[winfo name .] commu"
  wm maxsize $self 10000 10000

  frame $self.radioframe -relief raised -borderwidth 1
  frame $self.unixframe -relief raised -borderwidth 1
  frame $self.tcpframe -relief raised -borderwidth 1
  frame $self.b_frame -relief raised -borderwidth 1

# radioframe
  label $self.radioframe.l -text "Transport:" -anchor w
  radiobutton $self.radioframe.default\
    -relief raised -borderwidth 1\
    -variable static_commu(transport)\
    -value default\
    -anchor w\
    -text Default\
    -selectcolor black\
    -command "commu_pack $self"
  radiobutton $self.radioframe.unix\
    -relief raised -borderwidth 1\
    -variable static_commu(transport)\
    -value unix\
    -anchor w\
    -text Unix\
    -selectcolor black\
    -command "commu_pack $self"
  radiobutton $self.radioframe.tcp\
    -relief raised -borderwidth 1\
    -variable static_commu(transport)\
    -value tcp\
    -anchor w\
    -text TCP\
    -selectcolor black\
    -command "commu_pack $self"
  pack $self.radioframe.l -side top -padx 1m -fill x -expand 1
  pack $self.radioframe.default -side top -padx 1m -fill x -expand 1
  pack $self.radioframe.unix -side top -padx 1m -fill x -expand 1
  pack $self.radioframe.tcp -side top -padx 1m -fill x -expand 1

# unixframe
  label $self.unixframe.l -text "Socketname:" -anchor w
  entry $self.unixframe.e -textvariable static_commu(socket) -width 20
  pack $self.unixframe.l -side top -padx 1m -pady 1m -fill x -expand 1
  pack $self.unixframe.e -side top -padx 1m -pady 1m -fill x -expand 1

# tcpframe
  label $self.tcpframe.hl -text "Host:" -anchor w
  entry $self.tcpframe.he -textvariable static_commu(host) -width 20
  label $self.tcpframe.pl -text "Port:" -anchor w
  entry $self.tcpframe.pe -textvariable static_commu(port) -width 20
  pack $self.tcpframe.hl -side top -padx 1m -fill x -expand 1
  pack $self.tcpframe.he -side top -padx 1m -fill x -expand 1
  pack $self.tcpframe.pl -side top -padx 1m -fill x -expand 1
  pack $self.tcpframe.pe -side top -padx 1m -fill x -expand 1

# buttonframe
  button $self.b_frame.ok -text OK -command "commu_ok $self"
  button $self.b_frame.a -text Apply -command "commu_apply"
  button $self.b_frame.r -text Reset -command "commu_reset $self"
  button $self.b_frame.c -text Cancel -command "commu_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
      -side left -fill both -padx 1m -pady 1m -expand 1
}
