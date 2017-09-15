#
# global vars in this file:
#
# enum    global_setup(xh_transport) / {unix, tcp}
# string  global_setup(xh_host)
# string  global_setup(xh_socket)
# int     global_setup(xh_port)
#
# enum    static_xh(transport)
# string  static_xh(host)
# string  static_xh(socket)
# int     static_xh(port)
# boolean win_pos(xh)         / true: Window ist bereits positioniert
#

proc xh_reset {window} {
  global global_setup static_xh
  set static_xh(transport) $global_setup(xh_transport)
  set static_xh(host)      $global_setup(xh_host)
  set static_xh(socket)    $global_setup(xh_socket)
  set static_xh(port)      $global_setup(xh_port)
  xh_pack $window
}

proc xh_pack {window} {
  global static_xh
  pack forget $window.unixframe $window.tcpframe $window.tcpframe\
      $window.buttonframe
  pack $window.radioframe -side top -fill x -expand 1
  switch $static_xh(transport) {
    unix {pack $window.unixframe -side top -fill x -expand 1}
    tcp  {pack $window.tcpframe -side top -fill x -expand 1}
  }
  pack $window.b_frame -side bottom -fill both -expand 1
}

proc xh_apply {} {
  global global_setup static_xh
  set global_setup(xh_transport) $static_xh(transport)
  set global_setup(xh_host)      $static_xh(host)
  set global_setup(xh_socket)    $static_xh(socket)
  set global_setup(xh_port)      $static_xh(port)
}

proc xh_ok {window} {
  xh_apply
  wm withdraw $window
}

proc xh_cancel {window} {
  wm withdraw $window
}

proc xh_win_open {} {
  global win_pos static_xh

  if {![winfo exists .xh]} {xh_win_create}
  xh_reset $static_xh(win)
  if $win_pos(xh) then {wm positionfrom .xh user}
  update idletasks
  wm deiconify .xh
  set win_pos(xh) 1
}

proc xh_win_create {} {
  global win_pos global_setup static_xh

  set win_pos(xh) 0
  set self .xh
  set static_xh(win) $self
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] xh"
  wm maxsize $self 10000 10000

  frame $self.radioframe -relief raised -borderwidth 1
  frame $self.unixframe -relief raised -borderwidth 1
  frame $self.tcpframe -relief raised -borderwidth 1
  frame $self.b_frame -relief raised -borderwidth 1

# radioframe
  label $self.radioframe.l -text "Transport:" -anchor w
  radiobutton $self.radioframe.unix\
    -relief raised -borderwidth 1\
    -variable static_xh(transport)\
    -value unix\
    -anchor w\
    -text Unix\
    -command "xh_pack $self"
  radiobutton $self.radioframe.tcp\
    -relief raised -borderwidth 1\
    -variable static_xh(transport)\
    -value tcp\
    -anchor w\
    -text TCP\
    -command "xh_pack $self"
  pack $self.radioframe.l -side top -padx 1m -fill x -expand 1
  pack $self.radioframe.unix -side top -padx 1m -fill x -expand 1
  pack $self.radioframe.tcp -side top -padx 1m -fill x -expand 1

# unixframe
  label $self.unixframe.l -text "Socketname:" -anchor w
  entry $self.unixframe.e -textvariable static_xh(socket) -width 20
  pack $self.unixframe.l -side top -padx 1m -pady 1m -fill x -expand 1
  pack $self.unixframe.e -side top -padx 1m -pady 1m -fill x -expand 1

# tcpframe
  label $self.tcpframe.hl -text "Host:" -anchor w
  entry $self.tcpframe.he -textvariable static_xh(host) -width 20
  label $self.tcpframe.pl -text "Port:" -anchor w
  entry $self.tcpframe.pe -textvariable static_xh(port) -width 20
  pack $self.tcpframe.hl -side top -padx 1m -fill x -expand 1
  pack $self.tcpframe.he -side top -padx 1m -fill x -expand 1
  pack $self.tcpframe.pl -side top -padx 1m -fill x -expand 1
  pack $self.tcpframe.pe -side top -padx 1m -fill x -expand 1

# buttonframe
  button $self.b_frame.ok -text OK -command "xh_ok $self"
  button $self.b_frame.a -text Apply -command "xh_apply"
  button $self.b_frame.r -text Reset -command "xh_reset $self"
  button $self.b_frame.c -text Cancel -command "xh_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
      -side left -fill both -padx 1m -pady 1m -expand 1
}
