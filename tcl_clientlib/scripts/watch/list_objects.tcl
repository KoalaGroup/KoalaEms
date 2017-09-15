# lists all existing objects of the given VED

proc list_objects {ved namelist verbose} {

foreach i $namelist {
  switch $i {
    1 {list_ved $ved $verbose}
    2 {list_domains $ved}
    3 {list_iss $ved}
    4 {list_vars $ved}
    5 {list_pis $ved}
    6 {list_dos $ved}
    default {puts "unknown objecttype $i"}
  }
}
}
