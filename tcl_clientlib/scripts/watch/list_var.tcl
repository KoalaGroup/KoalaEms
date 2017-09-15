# lists a single variable

proc list_var {ved idx} {

if [catch {$ved var size $idx} mist] {
  puts $mist
  return
}
set size $mist
puts "variable $idx: size $size"
set vl [$ved var read $idx]
if {$size>50} {
  set ssize 20
} else {
  set ssize $size
}
for {set i 0} {$i<$ssize} {incr i} {
  puts -nonewline [format {0x%X } [lindex $vl $i]]
}
if {$size>$ssize} {
  puts "..."
} else {
  puts ""
}
}
