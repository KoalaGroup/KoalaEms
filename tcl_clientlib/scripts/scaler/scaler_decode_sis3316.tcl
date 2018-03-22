# $ZEL: scaler_decode_sis3316.tcl,v 1.1 2016/05/02 15:37:02 wuestner Exp $
# created: 2016-03-04 PeWue
#
# global vars:
# 
# global_timestamp // timestamp updated by got_Timestamp und used by the
#                  // scaler decoders
# scaler_time
# scaler_time_last
# scaler_cont
# scaler_cont_last
# 

# data for sis3316_statistics:
# 
# number of modules
#  for each module:
#   four times (for each adc fpga):
#    const 24; number of following words
#    four times (for each channel):
#     Internal trigger counter
#     Hit trigger counter
#     Dead time trigger counter
#     Pileup trigger counter
#     Veto trigger counter
#     He trigger counter

# currently exactly one module is supported
proc got_sis3316_statistics {idx offs num jetzt rest} {
    upvar $rest list
    global global_timestamp
    global scaler_time
    global scaler_time_last
    global scaler_cont
    global scaler_cont_last

    #puts "got_sis3316_statistics $idx $offs $num $jetzt len=[llength $list] $list"

    # For each ADC group we will have 24 values (6 per channel).
    set numgroup [expr $num/24]

    # The whole block is preceded bye the module count
    # and each group is (hopefully) preceded by the constant 24
    set we_need [expr $numgroup*25+1]

    if {[llength $list]<$we_need} {
        output "got_sis3316_statistics: need $we_need words for $num channels \
but got only llength $list]"
        output "got: idx=$idx; offs=$offs list=$list"
        return -1
    }

    set nummod [lindex $list 0]
    set list [lrange $list 1 end]

    #puts "numgroup=$numgroup nummod=$nummod we_need=$we_need"

    # iterate over the modules
    for {set mod 0} {$mod<$nummod} {incr mod} {

        # iterate over the ADC groups
        for {set group 0} {$group<$numgroup} {incr group} {
            if {[lindex $list 0]!=24} {
                output "got_sis3316_statistics: expected constant 24, but got [lindex $list 0]"
                return -1
            }
            set list [lrange $list 1 end] ;# skip the '24'
            for {set i 0} {$i<24} {incr i} {
                set scaler_cont_last($offs) $scaler_cont($offs)
                set scaler_cont($offs) [lindex $list $i]
                set scaler_time_last($offs) $scaler_time($offs)
                set scaler_time($offs) $global_timestamp
                incr offs
            }
            set list [lrange $list 24 end] ;# skip 24 words
       }

    }

   return 0
}
