# $ZEL: pat_loop.tcl,v 1.1 2002/09/26 12:15:13 wuestner Exp $
# P. Wuestner 2002; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc connect_commu {} {
    global global_setup

    puts -nonewline "connect to commu: "
    if [catch {
        switch -exact $global_setup(commu_transport) {
            default {ems_connect}
            unix    {
                puts "  use $global_setup(commu_socket)"
                ems_connect $global_setup(commu_socket)
            }
            tcp     {
                puts "  use $global_setup(commu_port)@$global_setup(commu_host)"
                ems_connect $global_setup(commu_host) $global_setup(commu_port)
            }
    }} mist] {
        output $mist
        set res -1
    } else {
        puts "ok."
        set res 0
    }
    return $res
}

proc open_ved {} {
    global global_setup global_ved
    if [catch {set global_ved [ems_open $global_setup(ved)]} mist] {
        puts $mist
        return -1
    }
    return 0
}

proc read_var {} {
    global global_setup global_ved abs speed
    if [catch {set vals [$global_ved command1 GetVar $global_setup(var)]} mist] {
        puts $mist
        return -1
    }
    #puts $vals
    set interval [expr $global_setup(interval)/1000.]
    for {set i 0} {$i<16} {incr i} {
        set val [lindex $vals $i]
        set speed($i) [format {%.2f} [expr ($val-$abs($i))/$interval]]
        set abs($i) [lindex $vals $i]
    }
    return 0
}

proc loop {} {
    global global_setup global_after

    if {[ems_connected]==0} {
        if [connect_commu] {
            set global_after [after 60000 loop]
            return
        }
        if [open_ved] {
            catch {ems_disconnect}
            set global_after [after 60000 loop]
            return
        }
    }

    if [read_var] {
        catch {ems_disconnect}
        set global_after [after 60000 loop]
        return
    }

    set global_after [after $global_setup(interval) loop]
}

proc start_loop {} {
    global global_after

    set global_after [after 1000 loop]
}

proc restart_loop {} {
    global global_after

    after cancel $global_after
    if [catch {ems_disconnect} mist] {puts $mist}
    start_loop
}
