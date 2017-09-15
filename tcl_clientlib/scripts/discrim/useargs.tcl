# $Id: useargs.tcl,v 1.1 2000/03/24 17:13:23 wuestner Exp $
# 

proc use_args {} {
  global global_setup
}

proc printhelp {} {
  global argv0
  puts ""
  puts "usage: $argv0 \[-help\]"
  puts "  -help prints this info"
}
