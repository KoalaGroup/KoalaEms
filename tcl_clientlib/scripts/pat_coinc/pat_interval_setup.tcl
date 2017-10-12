# $ZEL: pat_interval_setup.tcl,v 1.1 2002/09/26 12:15:13 wuestner Exp $
# P. Wuestner 2002; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars in this file:
#
# int global_setup(interval)
# boolean win_pos(interval)         / true: Window ist bereits positioniert
#

proc interval_apply {} {
    global global_setup static_interval
    if {$static_interval(interval)<1000} {set static_interval(interval) 1000}
    set global_setup(interval) $static_interval(interval)
    restart_loop
}

proc interval_ok {window} {
    interval_apply
    wm withdraw $window
}

proc interval_cancel {window} {
    wm withdraw $window
}

proc interval_win_open {} {
    global global_setup win_pos static_interval

    if {![winfo exists .interval]} {interval_win_create}
    set static_interval(interval) $global_setup(interval)
    if $win_pos(interval) then {wm positionfrom .interval user}
    update idletasks
    wm deiconify .interval
    set win_pos(interval) 1
}

proc interval_win_create {} {
    global win_pos global_setup static_interval

    set win_pos(interval) 0
    set self .interval
    set static_interval(win) $self
    toplevel $self
    wm withdraw $self
    wm group $self .
    wm title $self "[winfo name .] interval"
    wm maxsize $self 10000 10000

    frame $self.intframe -relief raised -borderwidth 1
    frame $self.b_frame -relief raised -borderwidth 1

# intframe
    label $self.intframe.il -text "ms:" -anchor w
    entry $self.intframe.ie -textvariable static_interval(interval) -width 20
    pack $self.intframe.il -side top -padx 1m -fill x -expand 1
    pack $self.intframe.ie -side top -padx 1m -fill x -expand 1

# buttonframe
    button $self.b_frame.ok -text OK -command "interval_ok $self"
    button $self.b_frame.a -text Apply -command "interval_apply"
    button $self.b_frame.c -text Cancel -command "interval_cancel $self"
    pack $self.b_frame.ok $self.b_frame.a $self.b_frame.c\
        -side left -fill both -padx 1m -pady 1m -expand 1

    pack $self.intframe -side top -fill x -expand 1
    pack $self.b_frame -side bottom -fill both -expand 1
}
