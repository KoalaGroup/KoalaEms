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
  global $list
  if {[info exists $list]} {
    if {[lsearch -exact $list $data]==-1} {
      lappend $list $data
    }
  }
}

proc save_setup {name} {
  global global_setup
  set f [open $name w]
  foreach i [array names global_setup] {
    puts $f "set global_setup($i) \{[set global_setup($i)]\}"
  }
  if {[info exists global_setup(extra)]} {
    foreach i $global_setup(extra) {
      global $i
      puts $f "global $i"
      puts $f "ladd global_setup(extra) $i"
      if {[array size $i]==0} {
        puts $f "set $i \{[set $i]\}"
      } else {
        foreach j [array names $i] {
          puts $f "set $i\($j\) \{[eval set $i\($j\)]\}"
        }
      }
    }
  }
  close $f
}

proc restore_setup {name} {
  global global_setup
  if [file exists $name] {
    source $name
  }
}
