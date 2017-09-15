# lists a single lam domain

proc list_dom_lam {ved idx} {

puts "domain lam $idx:"
if [catch {puts "  [$ved lamproclist upload $idx]"} mist] {puts $mist}
}
