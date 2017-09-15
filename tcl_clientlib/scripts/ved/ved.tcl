#
# global vars in this file:
#
# liste  global_setup(veds)
# window global_<vedname>(menu)
#

proc open_ved {name type} {
  global global_setup
  if [catch {set ved [ems_open $name $type]} mist] {
    bgerror $mist
    set res -1
  } else {
    lappend global_setup(veds) $ved
    set global_setup($ved) "$name $type"
    set menu [create_menu_ved_$type $ved]
    global global_${ved}
    set global_${ved}(menu) $menu
    set res 0
  }
  return $res
}

proc close_ved {ved} {
  global global_menu global_setup
  global global_${ved}
  set i [lsearch -exact $global_setup(veds) $ved]
  set global_setup(veds) [lreplace $global_setup(veds) $i $i]
  unset global_setup($ved)
  $ved close
  rename [set global_${ved}(menu)] {}
}

proc ems_fakeved_command {name args} {
  if "\"[lindex $args 0]\"==\"close\"" {
    rename $name {}
  } else {
    output "$name is not really open"
  }
}

proc ems_fake_open {name type} {
  set newname ved_$name
  if {[info proc $newname]!=""} {
    set num 1
    set newname ved_${name}#$num
    while {[info proc $newname]!=""} {
      incr num
      set newname ved_${name}#$num
    }
  }
  set comm [format "proc %s {args} {ems_fakeved_command %s %s}"\
      $newname $newname {$args}]
  eval $comm
  return $newname
}

proc open_ved_fake {name type} {
  global global_setup
  set ved [ems_fake_open $name $type]
  lappend global_setup(veds) $ved
  set global_setup($ved) "$name $type"
  set menu [create_menu_ved_$type $ved]
  global global_${ved}
  set global_${ved}(menu) $menu
}

proc ved_identify {ved} {
  if [catch {
    set erg [$ved identify]
    } mist] {
    bgerror $mist
  } else {
    output        "VED version      : [lindex $erg 0]"
    output_append "Req-tab version  : [lindex $erg 1]"
    output_append "Unsol-tab version: [lindex $erg 2]"
    foreach i [lrange $erg 3 end] {
      output_append $i
    }
  }
}

proc ved_caplist {ved type} {
  if [catch {
    $ved version_separator .
    set erg [$ved caplist $type]
    } mist] {
    bgerror $mist
  } else {
    set n 0
    foreach i $erg {
      if {$n==0} {
        output $i
        incr n
      } else {
        output_append $i
      }
    }
  }
}
