# lists all domains

proc list_domains {ved} {

if [catch {set doms [$ved namelist 2 0]} mist] {puts $mist; return}

if {$doms == ""} {
  puts "--- no domains available"
} else {
  foreach i $doms {
    switch $i {
      1 {list_dom_modlist $ved}
      2 {list_dom_lams $ved}
      3 {list_dom_trigger $ved}
      4 {list_dom_event $ved}
      5 {list_dom_datains $ved}
      6 {list_dom_dataouts $ved}
      default {puts "unknown domaintype $i"}
    }
  }
}
}

proc changed_domain {ved data} {
  set typ [lindex $data 0]
  set rest [lrange $data 1 end]
  switch $typ {
    1 {changed_dom_modlist $ved $rest}
    2 {changed_dom_lams $ved $rest}
    3 {changed_dom_trigger $ved $rest}
    4 {changed_dom_event $ved $rest}
    5 {changed_dom_datain $ved $rest}
    6 {changed_dom_dataout $ved $rest}
    default {puts "changed unknown domaintype $typ"}
  }
}
