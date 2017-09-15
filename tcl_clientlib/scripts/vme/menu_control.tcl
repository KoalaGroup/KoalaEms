# $Id: menu_control.tcl,v 1.1 2000/07/15 21:33:33 wuestner Exp $
# © 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# Diese Prozedur erzeugt das Filemenu im Hauptmenu
#

proc create_menu_control {parent} {
  set self [string trimright $parent .].control
  menu $self -title "VME Control"

  $self add command -label "\[Re\]connect to Server" -underline 0\
      -command {connect_to_server}

  $self add command -label "Disconnect from Server" -underline 0\
      -command {disconnect_from_server}

  $self add command -label "Insert Variable ..." -underline 0 \
      -command var_insert::win_open

  $self add command -label "Delete selected Variable" -underline 0 \
      -command varframe::delete_var

  return $self
  }
