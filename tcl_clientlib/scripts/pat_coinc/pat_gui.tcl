# $ZEL: pat_gui.tcl,v 1.1 2002/09/26 12:15:12 wuestner Exp $
# P. Wuestner 2002; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc create_menu_all {parent} {
    set self [string trimright $parent .].all
    menubutton $self -text "Trigger Pattern" -menu $self.m
    menu $self.m -tearoff 0
    set filemenu [create_menu_file $self.m]
    $self.m add separator
    set setupmenu [create_menu_setup $self.m]
    pack $filemenu $setupmenu -side top -padx 1m
    #$self.m add command -label "Quit" -underline 0 -command ende
    return $self
  }

proc crate_main_win {} {
    global global_dispwin global_setup

    frame .menuframe -relief raised -borderwidth 2
    frame .bar -height 3 -background #000
    set global_dispwin(frame) [frame .displayframe -relief raised -borderwidth 2]
    create_grid $global_dispwin(frame)

    if {$global_setup(no_wm)} {
        set allmenu [create_menu_all .menuframe]
        pack $allmenu -side left -expand 1 -fill x
    } else {
        set filemenu [create_menu_file .menuframe]
        set setupmenu [create_menu_setup .menuframe]
        pack $filemenu $setupmenu -side left -padx 1m
    }
    pack_all
}

proc pack_all {} {
    global global_setup

    pack .menuframe .bar -side top -fill x
    pack .displayframe -side bottom -fill both -expand 1
}

proc repack_all {} {
    pack forget .menuframe .bar .displayframe
    pack_all
}

proc start_gui {} {
    crate_main_win
    update idletasks
    wm deiconify .
}
