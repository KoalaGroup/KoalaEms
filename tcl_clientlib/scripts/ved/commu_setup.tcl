#
# global vars in this file:
#
# enum    global_setup(commu_transport) / {default, unix, tcp}
# string  global_setup(commu_host)
# string  global_setup(commu_socket)
# int     global_setup(commu_port)
# boolean global_setup(commu_autoconnect)
# boolean global_setup(offline)
#
# enum    global_commu(transport)
# string  global_commu(host)
# string  global_commu(socket)
# int     global_commu(port)
# boolean global_commu(autoconnect)
# boolean global_commu(win_pos)         / true: Window ist bereits positioniert
#

proc commu_sure {} {
  set res [
  tk_dialog .d {Are You Sure?} {There is an active communication.\
 Do You want to replace it?} question -1 {YES} {NO}]
  return [expr !$res]
}

proc commu_copy {} {
  global global_setup global_commu
  set global_commu(transport) $global_setup(commu_transport)
  set global_commu(host)      $global_setup(commu_host)
  set global_commu(socket)    $global_setup(commu_socket)
  set global_commu(port)      $global_setup(commu_port)
  set global_commu(autoconnect) $global_setup(commu_autoconnect)
}

proc commu_pack {window} {
  global global_commu
  pack forget $window.unixframe $window.tcpframe $window.tcpframe\
      $window.buttonframe
  pack $window.radioframe -side top -fill x -expand 1
  switch $global_commu(transport) {
    unix {pack $window.unixframe -side top -fill x -expand 1}
    tcp  {pack $window.tcpframe -side top -fill x -expand 1}
  }
  pack $window.b_frame -side bottom -fill both -expand 1
}

proc commu_apply {window} {
  global global_setup global_commu
  set global_setup(commu_transport) $global_commu(transport)
  set global_setup(commu_host)      $global_commu(host)
  set global_setup(commu_socket)    $global_commu(socket)
  set global_setup(commu_port)      $global_commu(port)
  set global_setup(commu_autoconnect) $global_commu(autoconnect)
  if {$global_setup(offline) == 0} {
    if [ems_connected] {
      if [commu_sure] connect_commu
    } else {
      connect_commu
    }
  }
}

proc commu_ok {window} {
  commu_apply $window
  wm withdraw $window
}

proc commu_reset {window} {
  commu_copy
  commu_pack $window
}

proc commu_cancel {window} {
  commu_copy
  wm withdraw $window
}

proc commu_win_open {} {
  global global_commu
  commu_copy
  commu_pack .commu
  if $global_commu(win_pos) then {wm positionfrom .commu user}
  update idletasks
  wm deiconify .commu
  set global_commu(win_pos) 1
}

proc create_commu_win {parent} {
  global global_setup global_commu

  if {[info exists global_setup(commu_transport)] == 0} {
    set global_setup(commu_transport) default
  }
  if {[info exists global_setup(commu_host)] == 0} {
    set global_setup(commu_host) "zelas3"
  }
  if {[info exists global_setup(commu_socket)] == 0} {
    set global_setup(commu_socket) "/var/tmp/emscomm"
  }
  if {[info exists global_setup(commu_port)] == 0} {
    set global_setup(commu_port) "4096"
  }
  if {[info exists global_setup(commu_autoconnect)] == 0} {
    set global_setup(commu_autoconnect) "0"
  }
  commu_copy
  set global_commu(win_pos) 0
  set self [string trimright $parent .].commu
  toplevel $self
  wm withdraw $self
  wm group $self $parent

  frame $self.radioframe -relief raised -borderwidth 1
  frame $self.unixframe -relief raised -borderwidth 1
  frame $self.tcpframe -relief raised -borderwidth 1
  frame $self.b_frame -relief raised -borderwidth 1

# radioframe
  label $self.radioframe.l -text "Transport:" -anchor w
  radiobutton $self.radioframe.default\
    -relief raised -borderwidth 1\
    -variable global_commu(transport)\
    -value default\
    -anchor w\
    -text Default\
    -selectcolor black\
    -command "commu_pack $self"
  radiobutton $self.radioframe.unix\
    -relief raised -borderwidth 1\
    -variable global_commu(transport)\
    -value unix\
    -anchor w\
    -text Unix\
    -selectcolor black\
    -command "commu_pack $self"
  radiobutton $self.radioframe.tcp\
    -relief raised -borderwidth 1\
    -variable global_commu(transport)\
    -value tcp\
    -anchor w\
    -text TCP\
    -selectcolor black\
    -command "commu_pack $self"
  checkbutton $self.radioframe.auto\
    -relief raised -borderwidth 1\
    -variable global_commu(autoconnect)\
    -anchor w\
    -text Autconnect\
    -selectcolor black
  pack $self.radioframe.l -side top -padx 1m -fill x -expand 1
  pack $self.radioframe.default -side top -padx 1m -fill x -expand 1
  pack $self.radioframe.unix -side top -padx 1m -fill x -expand 1
  pack $self.radioframe.tcp -side top -padx 1m -fill x -expand 1
  pack $self.radioframe.auto -side top -padx 1m -fill x -expand 1

# unixframe
  label $self.unixframe.l -text "Socketname:" -anchor w
  entry $self.unixframe.e -textvariable global_commu(socket) -width 20
  pack $self.unixframe.l -side top -padx 1m -pady 1m -fill x -expand 1
  pack $self.unixframe.e -side top -padx 1m -pady 1m -fill x -expand 1

# tcpframe
  label $self.tcpframe.hl -text "Host:" -anchor w
  entry $self.tcpframe.he -textvariable global_commu(host) -width 20
  label $self.tcpframe.pl -text "Port:" -anchor w
  entry $self.tcpframe.pe -textvariable global_commu(port) -width 20
  pack $self.tcpframe.hl -side top -padx 1m -fill x -expand 1
  pack $self.tcpframe.he -side top -padx 1m -fill x -expand 1
  pack $self.tcpframe.pl -side top -padx 1m -fill x -expand 1
  pack $self.tcpframe.pe -side top -padx 1m -fill x -expand 1

# buttonframe
  button $self.b_frame.ok -text OK -command "commu_ok $self"
  button $self.b_frame.a -text Apply -command "commu_apply $self"
  button $self.b_frame.r -text Reset -command "commu_reset $self"
  button $self.b_frame.c -text Cancel -command "commu_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
      -side left -fill both -padx 1m -pady 1m -expand 1
}
