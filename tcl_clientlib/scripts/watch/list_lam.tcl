# list a single LAM

proc listlam {ved idx} {

puts "program invocation lam $idx:"
if [catch {puts "  [$ved lam status $idx]"} mist] {puts $mist}
}
