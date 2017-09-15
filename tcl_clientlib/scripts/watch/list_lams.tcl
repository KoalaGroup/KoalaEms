#list all LAM handling program invocations

proc list_lams {ved} {

if [catch {set lams [$ved namelist pi lam]} mist] {puts $mist; return}
if {$lams == ""} {
  puts "--- no LAMs defined"
} else {
  puts "--- LAMs:"
  foreach i $lams {list_lam $ved $i}
  }
puts ""
}
