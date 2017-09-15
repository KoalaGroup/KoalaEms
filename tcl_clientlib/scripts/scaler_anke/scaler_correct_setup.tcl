#
# Diese Prozedur vervollstaendigt die Variablen, die von einem Konfigfile
# gelesen wurden. Als Ergebnis sind alle Setupvariablen vorhanden und
# initialisiert, deren Existenz spaeter vorausgesetzt wird
#

proc correct_setup {} {
  global global_setup
  global global_setup_names
  global global_setup_pane
  global global_setup_vis

  if {[info exists global_setup(auto_save)] == 0} {
    set global_setup(auto_save) 0}

  if {[info exists global_setup(auto_init)] == 0} {
    set global_setup(auto_init) 0}

  if {[info exists global_setup(auto_run)] == 0} {
    set global_setup(auto_run) 0}

  if {[info exists global_setup(graphical_display)] == 0} {
    set global_setup(graphical_display) 1}

  if {[info exists global_setup(numerical_display)] == 0} {
    set global_setup(numerical_display) 1}

  if {[info exists global_setup(commu_transport)] == 0} {
    set global_setup(commu_transport) default}

  if {[info exists global_setup(commu_host)] == 0} {
    set global_setup(commu_host) localhost}

  if {[info exists global_setup(commu_socket)] == 0} {
    set global_setup(commu_socket) /var/tmp/emscomm}

  if {[info exists global_setup(commu_port)] == 0} {
    set global_setup(commu_port) 4096}

  if {[info exists global_setup(xh_transport)] == 0} {
    set global_setup(xh_transport) tcp}

  if {[info exists global_setup(xh_host)] == 0} {
    set global_setup(xh_host) localhost}

  if {[info exists global_setup(xh_socket)] == 0} {
    set global_setup(xh_socket) /var/tmp/xhcomm}

  if {[info exists global_setup(xh_port)] == 0} {
    set global_setup(xh_port) 1234}

    if {[info exists global_setup(veds)] == 0} {
    set global_setup(veds) {}}

  if {[info exists global_setup(evrate_ved)] == 0} {
    set global_setup(evrate_ved) {}}

  if {[info exists global_setup(iss)] == 0} {
    set global_setup(iss) {}}

  if {[info exists global_setup(updateinterval)] == 0} {
    set global_setup(updateinterval) 2}

  if {[info exists global_setup(policy_ved)] == 0} {
    set global_setup(policy_ved) ask}

  if {[info exists global_setup(policy_modullist)] == 0} {
    set global_setup(policy_modullist) ask}

  if {[info exists global_setup(policy_is)] == 0} {
    set global_setup(policy_is) ask}

  if {[info exists global_setup(policy_memberlist)] == 0} {
    set global_setup(policy_memberlist) ask}

  if {[info exists global_setup(policy_lam)] == 0} {
    set global_setup(policy_lam) ask}

  if {[info exists global_setup(policy_modullist)] == 0} {
    set global_setup(policy_modullist) ask}

  if {[info exists global_setup(policy_modullist)] == 0} {
    set global_setup(policy_modullist) ask}

#  if {[info exists global_setup(extra)] == 0} {
#    set global_setup(extra) {
#      global_setup_modullists
#      global_setup_is
#      global_setup_variables
#      global_setup_memberlists
#    }
#    }
#  foreach i $global_setup(extra) {global $i}

  if {[info exists global_setup(num_panes)] == 0} {
    set global_setup(num_panes) 2}

  if {[info exists global_setup(num_channels)] == 0} {
    set global_setup(num_channels) 24}

  if {[info exists global_setup(show_index)] == 0} {
    set global_setup(show_index) 1}

  if {[info exists global_setup(show_name)] == 0} {
    set global_setup(show_name) 1}

  if {[info exists global_setup(show_content)] == 0} {
    set global_setup(show_content) 1}

  if {[info exists global_setup(show_rate)] == 0} {
    set global_setup(show_rate) 1}

  if {[info exists global_setup(show_average_rate)] == 0} {
    set global_setup(show_average_rate) 1}

  if {[info exists global_setup(show_min_rate)] == 0} {
    set global_setup(show_min_rate) 0}

  if {[info exists global_setup(show_max_rate)] == 0} {
    set global_setup(show_max_rate) 0}

  if {[info exists global_setup(termfont)] == 0} {
    set global_setup(termfont) -*-courier-medium-r-*-*-12-*-*-*-*-*-*-*}

  if {[info exists global_setup(interval_slider)] == 0} {
    set global_setup(interval_slider) 1}

  if {[info exists global_setup(print_auto)] == 0} {
    set global_setup(print_auto) 0}

  if {[info exists global_setup(print_command)] == 0} {
    set global_setup(print_command) ""}

  if {[info exists global_setup(triggered_channel)] == 0} {
    set global_setup(triggered_channel) 12}

  if {[info exists global_setup(measured_channel)] == 0} {
    set global_setup(measured_channel) 13}

  if {[info exists global_setup(view_deadtime)] == 0} {
    set global_setup(view_deadtime) 1}

  if {[info exists global_setup(display_repeat)] == 0} {
    set global_setup(display_repeat) 10}

  for {set i 0} {$i<$global_setup(num_channels)} {incr i} {
    if {[info exists global_setup_names($i)] == 0} {
      set global_setup_names($i) __$i}
    if {[info exists global_setup_pane($i)] == 0} {
      set global_setup_pane($i) 0}
    if {[info exists global_setup_vis($i)] == 0} {
      set global_setup_vis($i) 0}
  }
}
