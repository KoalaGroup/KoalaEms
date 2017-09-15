#$Id: save_restore.tcl,v 1.2 1998/05/14 21:23:00 wuestner Exp $
#
# global vars in this file:
#
# global_setup(...)
#
# setupfile
#

proc printsetup {} {
  global global_setup
  puts "global setup:"
  foreach i [array names global_setup] {
    puts "  $i\t=[set global_setup($i)]"
  }
}

proc ladd {list data} {
  global $list global_verbose

  if $global_verbose {puts "ladd $list $data"}
  if {[info exists $list]} {
    if {[lsearch -exact [set $list] $data]==-1} {
      lappend $list $data
    }
  } else {
    lappend $list $data
  }
  if $global_verbose {puts "$list=[set $list]"}
}

proc save_setup {name} {
  global global_setup global_setup_extra

  puts "save setup to $name"
  set f [open $name w]
  puts $f "# saved at [clock format [clock seconds]]"
  foreach i [lsort [array names global_setup]] {
    puts $f "set global_setup($i) \{[set global_setup($i)]\}"
  }
  if {[info exists global_setup_extra]} {
    foreach i [lsort $global_setup_extra] {
      global $i
      if [info exists $i] {
        puts $f "global $i"
        puts $f "ladd global_setup_extra $i"
        if {[array size $i]==0} {
          puts $f "set $i \{[set $i]\}"
        } else {
          foreach j [lsort [array names $i]] {
            puts $f "set $i\($j\) \{[eval set $i\($j\)]\}"
          }
        }
      }
    }
  }
  close $f
}

proc restore_setup {name} {
  global global_setup

  if [file exists $name] {
    puts "restore setup from $name"
    source $name
  } else {
    puts "setupfile $name not found"
  }
}
