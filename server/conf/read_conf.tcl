#!/bin/sh
#\
exec emssh $0 $*

ems_connect ikpe1103 4096
set ved [ems_open [lindex $argv 0]]
set ident [$ved ident]
puts "/*"
puts "[$ved name] [clock format [clock seconds]]"
puts "*/"
puts $ident

