# $Id: menu_control.tcl,v 1.6 1999/04/09 12:32:35 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# Diese Prozedur erzeugt das Setupmenu im Hauptmenu
#

proc create_menu_daqsetup {parent} {
  set self [string trimright $parent .].daqsetup
  menu $self -title "DAQ control"

  $self add command -label "clear setup" -underline 0 \
      -command clear_setup_data

  $self add command -label "reset VEDs" -underline 0 \
      -command reset_button

  $self add command -label "clear Log" -underline 0 \
      -command {truncate_log 0}

  $self add command -label "open VED status" -underline 0 \
      -command map_status_display

  $self add command -label "open TAPE status" -underline 0 \
      -command map_tstatus_display

  $self add command -label "stop status_update" -underline 0 \
      -command {stop_status_loop}

  $self add command -label "restart status_update" -underline 0 \
      -command {start_status_loop}

  $self add command -label "stop actions" -underline 0 \
      -command stop_wait_actions

  $self add command -label "dump config to log" -underline 0 \
      -command dump_config

  $self add checkbutton -label "verbose log" -underline 0 \
      -variable global_verbose -selectcolor black

  return $self
  }
