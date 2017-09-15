proc test_caplist {ved name} {
  set list [$ved caplist]
  foreach ii $list {
    set iii [lindex $ii 1]
    if {"$iii"=="$name"} {return 1}
  }
  return 0
}
