#! /bin/sh
#\
exec wish $0 $*

# $ZEL: show_tpg300.tcl,v 1.1 2011/08/26 14:29:03 wuestner Exp $
# copyright 1998, 1999, 2001
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

array set units {
    0 ERROR
    1 mbar
    2 Torr
    3 Pascal
}

array set states {
    0 {OK}
    1 {underrange}
    2 {overrange}
    3 {error}
    4 {off}
    5 {no hardware}
}

proc read_config {fname} {
    global name

    if {[catch {set f [open $fname RDONLY]} mist]} {
        puts $mist
        return -1
    }
    if {[catch {set ff [read $f]} mist]} {
        puts $mist
        return -1
    }
    close $f

    set file [split $ff "\n"]
    set idx 0
    foreach line $file {
        incr idx
        #puts $l
        set l [string trim $line]
        if {[string length $l]==0 || [string index $l 0]=={#}} continue
        if {[llength $l]!=3} {
            puts "cannot parse line $idx:"
            puts "$l"
            return -1
        }
        set slave [lindex $l 0]
        set names [lindex $l 1]
        set wanted [lindex $l 2]
        foreach w $wanted {
            if {$w>3 || $w>[llength $names]} {
                puts "wrong index $w in line $idx:"
                puts $l"
                return -1
            }
            set name([format {%02d_%d} $slave $w]) [lindex $names $w]
        }
    }
    if {0} {
        foreach i [lsort [array names name]] {
            puts "$i $name($i)"
        }
    }
    return 0
}

proc open_window {} {
    global entry_ro name updated value unit

    set maintitle {TPG300 readout}
    if [regexp #.* [winfo name .] number] {
      append maintitle "$number"
    }
    wm title . $maintitle
    wm maxsize . 10000 10000
    tk_focusFollowsMouse

    frame .head -borderwidth 3 -relief ridge
    label .head.l -text "Updated"
    entry .head.e -textvariable updated -relief flat\
        -state $entry_ro -width 0
    frame .st -borderwidth 3 -relief ridge

    set n 0;
    foreach i [lsort [array names name]] {
        #puts "$i $name($i)"
        label .st.l$i -text $name($i)
        label .st.c$i -text { }
        entry .st.v$i -textvariable value($i)
        #entry .st.u$i -textvariable unit($i)

        grid configure .st.l$i -column 0 -row $n -sticky w
        grid configure .st.c$i -column 1 -row $n -sticky w
        grid configure .st.v$i -column 2 -row $n -sticky w
        #grid configure .st.u$i -column 3 -row $n -sticky w
        incr n
    }

    grid columnconfigure .st 2 -weight 1
    pack .head -fill x -expand 1
    pack .head.e -fill x -expand 1
    pack .st -fill x -expand 1 ;# -padx 3 -pady 3

    return 0;
}

proc parse_tpg300 {bytes} {
    global name units states value unit

    # scan the header
    #puts "xlen: [string length $bytes]"
    binary scan $bytes {Iu1Iu1Iu1Iu1R1Iu1R1Iu1R1Iu1R1} \
        wordcount addr state\
        st(0) val(0)\
        st(1) val(1)\
        st(2) val(2)\
        st(3) val(3)

    if {[expr $state & 0xff]} {
        for {set i 0} {$i<4} {incr i} {
            set idx [format {%02d_%d} $addr $i]
            if {![info exists name($idx)]} continue
            set value($idx) ERROR
            set unit($idx) {}
        }
        #puts "error slave $addr"
    } else {
        set iunit [expr ($state>>8)&3]
        set u $units($iunit)
        for {set i 0} {$i<4} {incr i} {
            set idx [format {%02d_%d} $addr $i]
            if {![info exists name($idx)]} continue
            #puts -nonewline "$idx $name($idx) "
            if {$st($i)!=0} {
                #puts $states($st($i))
                set value($idx) $states($st($i))
                set unit($idx) {}
            } else {
                #puts [format {%.1e %s} $val($i) $u]
                set value($idx) [format {%.1e %s} $val($i) $u]
                #set unit($idx) $u
            }
        }
    }

    #puts [format {%08x %08x %08x} $wordcount $addr $state]

    return [expr $wordcount+1]
}

proc do_read {sock} {
    global updated

    # read the word count
    set bytes [read $sock 4]
    binary scan $bytes {Iu1} wordcount

    # read all the other data
    set bytes [read $sock [expr $wordcount*4]]
    # scan the header
    binary scan $bytes {Iu1Iu2Iu1} version time number
    #puts [format {haed: %08x %08x %08x %08x %08x } $wordcount $version [lindex $time 0] [lindex $time 1] $number]

    #puts "    [clock format [lindex $time 0]]"
    set updated [clock format [lindex $time 0]]
    #puts "len : [string length $bytes]"
    set xbytes [string range $bytes 16 end]
    for {set i 0} {$i<$number} {incr i} {
        set used [parse_tpg300 $xbytes]
        set xbytes [string range $xbytes [expr $used*4] end]
    }

    set updated [clock format [lindex $time 0] -format {%Y-%m-%d %H:%M:%S}]

    return 0
}

proc open_socket {host port} {
    if {[catch {set sock [socket $host $port]} mist]} {
        puts $mist
        return -1
    }
    fconfigure $sock -translation binary -blocking 0
    fileevent $sock readable [list do_read $sock]
    return 0
}

proc main {} {
    global argc argv argv0
    global entry_ro

    # readonly attribute for "entry"
    if {[info tclversion]>8.3} {
        set entry_ro {readonly}
    } else {
        set entry_ro {disabled}
    }

    # .rc_tpg300.tcl may set:
    #   configfile
    #   host
    #   port
    if {[catch {source ~/.rc_tpg300.tcl}]} {
        if {$argc!=3} {
            puts "usage: $argv0 configfile host port"
            return
        }
        set configfile [lindex $argv 0]
        set host [lindex $argv 1]
        set port [lindex $argv 2]
    }

    if {[read_config $configfile]<0} return

    if {[open_window]<0} return
    update idletasks

    if {[open_socket $host $port]<0} return

    vwait forever
}

main
