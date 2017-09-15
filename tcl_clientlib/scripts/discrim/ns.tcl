proc aaa {i} {
  puts "aaa($i) in [namespace current]"
}

aaa 1
#namespace export aaa

namespace eval mist {
  aaa 2
  proc bbb {i} {
    puts "bbb($i) in [namespace current]"
  }
  bbb 3
  namespace import ::aaa
  aaa 4
}

mist::bbb 6

namespace import mist::bbb

bbb 7
