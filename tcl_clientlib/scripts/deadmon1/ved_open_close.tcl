# $Id: ved_open_close.tcl,v 1.1 2000/07/15 21:37:03 wuestner Exp $
# copyright:
# 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#

proc ServerUnsol {ved head data} {
  LOG::put "Unsol from $ved: header: $head data: $data" tag_orange
}

proc ServerDisconnect {ved} {
  LOG::put "lost connection to VED $ved" tag_red
  close_ved $ved
}

proc gotdomtrigger {ved args} {
  LOG::put "gotdomtrigger $ved $args"
  set procedure [lindex $args 0]
  if {$procedure != "zel.1"} return
  set master [lindex $args [expr ([lindex $args 1]+3)/4+2]]
  LOG::put "master=$master"
  set ns_${ved}::is_master $master
}

proc gotnothing_ignore {ved args} {
  LOG::put "gotnothing_ignore $ved $args"
}

proc StatusChanged {ved data} {
  LOG::put "StatusChanged $ved $data"
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
          }
          6 {
          # Dataout
          }
        }
      }
    }
  }
  ::Loop::ask $ved
}

proc open_ved {vedname} {
  global global_setup
  if [catch {set ved [ems_open $vedname]} mist] {
    LOG::put "$mist" tag_red{}
  } else {
    ved_close_menu_change
    namespace eval ::ns_$ved {}
    set ns_${ved}::name $vedname
    set ns_${ved}::ved $ved
    set ns_${ved}::damals 0
    set ns_${ved}::is_master 0
    add_ved_to_display ::ns_$ved
    $ved typedunsol StatusChanged {StatusChanged %v %d}
    $ved unsol {ServerUnsol %v %h %d}
    #get_ved_info ::ns_$ved
    $ved trigger upload : gotdomtrigger $ved ? gotnothing_ignore $ved
    ::LOOP::add ::ns_$ved
    lappend global_setup(veds) $vedname
  }
}

proc close_ved {ved} {

  ::LOOP::delete ::ns_$ved
  set space ns_$ved
  remove_ved_from_display $space
  [set ${space}::ved] close
  #LOG::put "[set ${space}::ved] closed"
  namespace delete $space
  ved_close_menu_change
}
