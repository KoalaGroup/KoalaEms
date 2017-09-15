#
# Diese Prozedur trifft alle Vorbereitungen, um periodisch Scalerinhalte
# und Eventrate auslesen zu koennen
#
# global vars:
#
# global_setup (wie immer)
# global_status
#   uninitialized
#   initialized
#   running
#

proc scaler_init {} {
  set res [scaler_do_init]
  if {$res<0} {
    if [ems_connected] ems_disconnect
  }
  return $res
}

proc scaler_do_init {} {
  global global_setup global_status

  puts "scaler_init start"

# Verbindung zu commu aufnehmen  
  set res [connect_commu]
  if {$res<0} {return $res}

# Verbindungen zu VEDs aufnehmen
  if {$global_setup(evrate_ved)!=""} {
    set res [open_ved $global_setup(evrate_ved) global_evrate_ved \
        {for eventrate}]
    if {$res<0} {return $res}
  } else {
    set global_evrate_ved {}
  }

  set global_veds {}
  foreach ved $global_setup(veds) {
    set res [open_ved $ved cved]
    if {$res<0} {return $res}
    lappend global_veds $cved
    if {$cved!=""} {lappend global_veds $cved}
  }
  output "all VEDs open"
  update idletasks

# Modullisten testen
  output "global_veds: $global_veds"
  foreach ved $global_veds {
    if {$ved!=""} {
      load_modullist $ved
      if {$res<0} {return $res}
    }
  }

  set global_status initialized
  enable_actions $global_status
  puts "scaler_init done"
}

proc scaler_reset {} {
  global global_setup global_status

  puts "scaler_reset start"
  if [ems_connected] {
    output "disconnect from commu ..."
    ems_disconnect
  }
    set global_status uninitialized
  enable_actions $global_status
  puts "scaler_reset done"
}

proc veds_initialized {} {
  return 0
}
