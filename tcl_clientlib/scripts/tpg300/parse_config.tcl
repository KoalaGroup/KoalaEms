#! /bin/sh
#\
exec tclsh $0 $*

# $ZEL: parse_config.tcl,v 1.1 2011/08/26 14:29:03 wuestner Exp $
# copyright 1998, 1999, 2001
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc main {} {
    global argc argv argv0

    if {$argc!=1} {
        puts "usage: $argv0 filename"
        return
    }

    if {[catch {set f [open [lindex $argv 0] RDONLY]} mist]} {
        puts $mist
        return
    }
    if {[catch {set ff [read $f]} mist]} {
        puts $mist
        return
    }
    close $f

    set file [split $ff "\n"]
    set idx 0
    foreach line $file {
        incr idx
        set l [string trim $line]
        if {[string length $l]==0 || [string index $l 0]=={#}} continue
        if {[llength $l]!=3} {
            puts "cannot parse line $idx:"
            puts "$l"
            return
        }
        set slave [lindex $l 0]
        set names [lindex $l 1]
        set wanted [lindex $l 2]
        foreach w $wanted {
            if {$w>3 || $w>[llength $names]} {
                puts "wrong index $w in line $idx:"
                puts $l"
                return
            }
            set name([format {%02d_%d} $slave $w]) [lindex $names $w]
            puts [format {%2d %d %s} $slave $w [lindex $names $w]]
        }
        #puts $l
    }
if {0} {
    foreach i [lsort [array names name]] {
        puts "$i $name($i)"
    }
}
}

main
