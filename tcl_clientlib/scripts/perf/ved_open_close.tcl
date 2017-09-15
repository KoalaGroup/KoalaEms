# $Id: ved_open_close.tcl,v 1.2 1998/06/29 18:02:40 wuestner Exp $
#

proc ServerUnsol {ved head data} {
  puts "Unsol from $ved: header: $head data: $data"
}

# proc ServerDisconnect {ved} {
#   global dialog_nr
#   set win .error_$dialog_nr
#   incr dialog_nr
#   tk_dialog $win Error "lost connection to VED [$ved name]" {} 0 Dismiss
#   close_ved $ved
# }

proc ServerDisconnect {ved} {
  puts "lost connection to VED $ved"
  close_ved $ved
}

proc StatusChanged {ved data} {
  # puts "StatusChanged $ved $data"
  set namespace ::ns_$ved
  if {[set ${namespace}::ready]!=1} {return}
  set action [lindex $data 0]
  set object [lindex $data 1]
  switch $object {
    1 {
    # ved
      if {$action==6} {
        set ${namespace}::dataindom {}
        set ${namespace}::dataoutdom {}
        set ${namespace}::dataoutobj {}
        init_perfvals $namespace
        ::Display::update_ved_frame [set ${namespace}::ved] $namespace
      }
    }
    2 {
    # Domain
      if {($action==1) || ($action==2)} {
        set domain [lindex $data 2]
        set idx [lindex $data 3]
        switch $domain {
          3 {
          # Trigger
            if {$action==1} {
            # create
              $ved trigger upload : gotdomtrigger $ved ? gotnothing_ignore $ved
            } else {
            # delete
              set ${namespace}::ro(trigger) ---
            }
          }
          5 {
          # Datain
            if {$action==1} {
            # create
              lappend ${namespace}::dataindom $idx
              init_di_perfvals $namespace $idx
              $ved datain upload $idx : gotdidefinition $ved $idx ? gotnothing_ignore $ved $idx
            } else {
            # delete
              set ii [lsearch -exact [set ${namespace}::dataindom] $idx]
              set ${namespace}::dataindom [lreplace [set ${namespace}::dataindom] $ii $ii]
            }
            ::Display::update_ved_frame [set ${namespace}::ved] $namespace
          }
          6 {
          # Dataout
            if {$action==1} {
            # create
              lappend ${namespace}::dataoutdom $idx
              init_do_perfvals $namespace $idx
              $ved dataout upload $idx : gotdodefinition $ved $idx ? gotnothing_ignore $ved $idx
            } else {
            # delete
              set ii [lsearch -exact [set ${namespace}::dataoutdom] $idx]
              set ${namespace}::dataoutdom [lreplace [set ${namespace}::dataoutdom] $ii $ii]
              set ii [lsearch -exact [set ${namespace}::dataoutobj] $idx]
              if {$ii>=0} {
                set ${namespace}::dataoutobj [lreplace [set ${namespace}::dataoutobj] $ii $ii]
              }
            }
            ::Display::update_ved_frame [set ${namespace}::ved] $namespace
          }
        }
      }
    }
  }
  ::Loop::ask $ved
}

proc open_ved {ved} {
  global dialog_nr
  if [catch {set ved [ems_open $ved]} mist] {
    set win .error_$dialog_nr
    incr dialog_nr
    tk_dialog $win Error "$mist" {} 0 Dismiss
  } else {
    ved_close_menu_change
    namespace eval ::ns_$ved {}
    ::Display::add_ved $ved ::ns_$ved
    $ved typedunsol StatusChanged {StatusChanged %v %d}
    $ved unsol {ServerUnsol %v %h %d}
    get_ved_info $ved
  }
}

proc close_ved {ved} {
  ::Loop::delete $ved
  ::Display::delete_ved $ved
  $ved close
  ved_close_menu_change
}
