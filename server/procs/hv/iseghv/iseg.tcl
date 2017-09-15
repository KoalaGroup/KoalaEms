proc put_response {r} {
    puts -nonewline "response:"
    foreach v $r {
        puts -nonewline [format { %02x} $v]
    }
    puts {}
}

proc bcd2dec {bcd} {
    set dec [expr $bcd&0xf]
    incr dec [expr (($bcd>>4)&0xf)*10]
    return $dec
}

proc iseg_serial {ved bus module} {
    # direction 1; data-id 0xe0; num=0
    puts "$ved command1 iseghv_rw $bus 1 $module 0xe0 0"
    if {[catch {set r [$ved command1 iseghv_rw $bus 1 $module 0xe0 0]} mist]} {
        puts $mist
    } else {
        put_response $r
        if {[lindex $r 0]==0} {
            set id [expr ([lindex $r 1]>>3)&0x3f]
            set ser [bcd2dec [lindex $r 7]]
            incr ser [expr [bcd2dec [lindex $r 6]]*100]
            incr ser [expr [bcd2dec [lindex $r 5]]*10000]
            set firm [bcd2dec [lindex $r 9]]
            incr firm [expr ([lindex $r 8]&0xf)*100]
            set mode [expr ([lindex $r 8]>>4)&0xf]
            set channels [expr [lindex $r 10]&0xf]
            puts [format {id=%d ser=%06d ver=%03d mode=%d, %d channels} \
                    $id $ser $firm $mode $channels]
        } else {
            puts "error!"
        }
    }
}
#emssh8.4> ved_can command1 iseghv_rw 0 1 2 0xe0 0
#0 528 0 7 224 71 48 17 65 1 8
#CAN_Write id=0x211 type=0x0 len=1 0xe0
#LINUX_CAN_Read id=0x210 type=0x0 len=7 0xe0 0x47 0x30 0x11 0x41 0x1 0x8

proc iseg_start {ved bus} {
    $ved command1 iseghv_init $bus
    set r [$ved command1 iseghv_found_devices $bus]
    set r [lrange $r 2 end]
    foreach v $r {
        iseg_serial $ved $bus $v
    }
}

proc iseg_on_off {ved bus module channel} {
    $ved command1 iseghv_write $bus $module 0xcc 2 0 $channel
    set r [$ved command1 iseghv_rw $bus 1 $module 0xcc 0]
    put_response $r
}

proc iseg_info {ved bus module} {
    set r [$ved command1 iseghv_rw $bus 1 $module 0xcc 0]
    put_response $r
    set channels [lindex $r 6]
    puts -nonewline "active channels:"
    for {set i 0} {$i<8} {incr i} {
        if {$channels&(1<<$i)} {
            puts -nonewline " $i"
        }
    }
    puts {}
}

proc iseg_voltage {ved bus module channel} {
    set r [$ved command1 iseghv_get_act_volt $bus $module $channel]
    put_response $r
    set u1 [lindex $r 7] ;# computed by server
    set u2 [expr (([lindex $r 5]<<8)+[lindex $r 6])*3000./50000.]
    puts "$u1 V ($u2 V)"
}

proc iseg_set_voltage {ved bus module channel voltage} {
    set r [$ved command1 iseghv_set_volt $bus $module $channel $voltage]
    put_response $r
}

proc iseg_set_ramp {ved bus module ramp} {
    set r [$ved command1 iseghv_set_ramp $bus $module $ramp]
    put_response $r
}

proc iseg_ramp {ved bus module} {
    set r [$ved command1 iseghv_set_ramp $bus $module]
    put_response $r
}

proc iseg_voltage_limit {ved bus module} {
    set r [$ved command1 iseghv_get_volt_limit $bus $module]
    put_response $r
}

proc iseg_current {ved bus module channel} {
    set r [$ved command1 iseghv_get_act_curr $bus $module $channel]
    put_response $r
    #set u1 [lindex $r 7] ;# computed by server
    #set u2 [expr (([lindex $r 5]<<8)+[lindex $r 6])*3000./50000.]
    #puts "$u1 V ($u2 V)"
}

proc iseg_set_current {ved bus module channel current} {
    set r [$ved command1 iseghv_set_curr $bus $module $channel $current]
    put_response $r
}

proc iseg_set_current_trip {ved bus module channel current} {
    set r [$ved command1 iseghv_set_curr_trip $bus $module $channel $current]
    put_response $r
}

proc iseg_status {ved bus module channel} {
    set r [$ved command1 iseghv_get_status $bus $module $channel]
    put_response $r
}

proc iseg_gen_status {ved bus module} {
    set r [$ved command1 iseghv_get_general_status $bus $module]
    put_response $r
    set v [lindex $r 5]
    if {($v&0x1)==0} {puts {v_limt, c_limit or trip has been exceeded}}
    if {($v&0x2)==0} {puts {V is ramping}}
    if {($v&0x4)==0} {puts {safety loop is broken}}
    if {($v&0x8)!=0} {puts {V is not stable}}
    if {($v&0x10)==0} {puts {fine adjustment is off}}
    if {($v&0x20)==0} {puts {supply voltages or temperature out of range}}
    if {($v&0x40)!=0} {puts {kill funktion enabled}}
    if {($v&0x80)!=0} {puts {store enabled}}
}


# # init
# CAN_Write id=0x200 type=0x0 len=2 0xd8 0x0
# CAN_Write id=0x208 type=0x0 len=2 0xd8 0x0
# ...
# CAN_Write id=0x3f8 type=0x0 len=2 0xd8 0x0
# 
# CAN_Write id=0x210 type=0x0 len=2 0xd8 0x1
# 
# # rampe
# CAN_Write id=0x210 type=0x0 len=3 0xd0 0x6 0x82
# 
# # channel 1 on
# CAN_Write id=0x210 type=0x0 len=3 0xcc 0x0 0x1
# CAN_Write id=0x211 type=0x0 len=1 0xcc
# 
# # set voltage
# CAN_Write id=0x210 type=0x0 len=3 0xa0 0x82 0x35
# 
# 
# # reset
# CAN_Write id=0x210 type=0x0 len=3 0xa0 0x82 0x35
# CAN_Write id=0x210 type=0x0 len=3 0x9f 0x0 0x0
# CAN_Write id=0x211 type=0x0 len=1 0xf8
# CAN_Write id=0x210 type=0x0 len=3 0xcc 0x0 0xff
# CAN_Write id=0x211 type=0x0 len=1 0xcc


# device 2 found, class=6
# CAN_Write id=0x211 type=0x0 len=1 0xe0           ?
# CAN_Write id=0x210 type=0x0 len=3 0xd0 0x6 0x82  ok
# CAN_Write id=0x210 type=0x0 len=3 0xcc 0x0 0xff  ok
# CAN_Write id=0x211 type=0x0 len=1 0xcc           ok
# CAN_Write id=0x210 type=0x0 len=3 0xa0 0x0 0x0   falsch
