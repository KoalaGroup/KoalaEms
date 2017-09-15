# $ZEL: xh_commu.tcl,v 1.5 2004/11/18 12:07:45 wuestner Exp $
# copyright 1998 ... 2004
#   Peter Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#

# global vars in this file:
#
# global_commu(sock)
# global_peers(<PATH>)  : peername
# global_pathes(<PATH>) : arrname
# global_arrays(<NAME>) : path or empty
#

proc newdata {channel} {
  global global_arrays global_pathes
  set name $global_pathes($channel)
  if {[gets $channel x]==-1} {
    close $channel
    set global_arrays($name) ""
    return
  } else {
    set time [lindex $x 0]
    set val [lindex $x 1]
#    puts "$name add $time $val"
    $name add $time $val
  }
}

proc masterdata {channel} {
  global global_arrays hi
  global global_arrdata_limit
  global global_arrdata_color global_arrdata_width global_arrdata_scale
  global global_arrdata_vis global_arrdata_style
  global global_datacachedir

  if {[gets $channel x]==-1} {
    close $channel
    foreach i [array names global_arrays] {
      if {$global_arrays($i)==$channel} {
        set global_arrays($i) ""
      }
    }
    return
  } else {
#    puts "masterdata: got $x"
    foreach i $x {
#      puts "i=$i"
      set key [lindex $i 0]
#      puts "masterdata: key=$key"
      switch -exact $key {
        value {
          set arr [lindex $i 1]
          set time [lindex $i 2]
          set val [lindex $i 3]
          $arr add $time $val
        }
        name {
#          puts "--> set arr [lindex $i 1]"
          set arr [lindex $i 1]
#          puts "--> set name [lindex $i 2]"
          set name [lindex $i 2]
          if {[info exists global_arrays($arr)]} {
            set global_arrays($arr) $channel
            $arr name $name
            legend_update
          } else {
            #histoarray -type file -file "/var/tmp/xh/$arr" $arr $name
#            puts "histoarray -type file -file '$global_datacachedir/$arr' $arr $name"
            histoarray -type file -file "$global_datacachedir/$arr" $arr $name
            set global_arrays($arr) $channel
            if [info exists global_arrdata_limit($arr)] {
              set limit $global_arrdata_limit($arr)
              if {[llength $limit]==1} {
                $arr limit $limit
              } else {
                $arr limit [lindex $limit 0] [lindex $limit 1]
              }
            }
            # set color width scale vis limit style
            if {![info exists global_arrdata_color($arr)]} {
              set global_arrdata_color($arr) [newcolor]
            }
            if {![info exists global_arrdata_width($arr)]} {
              set global_arrdata_width($arr) 0
            }
            if {![info exists global_arrdata_scale($arr)]} {
              set global_arrdata_scale($arr) 0
            }
            if {![info exists global_arrdata_vis($arr)]} {
              set global_arrdata_vis($arr) 0
            }
            if {![info exists global_arrdata_style($arr)]} {
              set global_arrdata_style($arr) boxes
            }
            $hi(win) attach $arr\
                -color $global_arrdata_color($arr)\
                -width $global_arrdata_width($arr)\
                -exp $global_arrdata_scale($arr)\
                -enabled $global_arrdata_vis($arr)\
                -style $global_arrdata_style($arr)
            #$hi(win) arrconfig $arr -enabled 0
            legend_update
          }
          ladd global_setup_extra global_arrdata_limit
          ladd global_setup_extra global_arrdata_color
          ladd global_setup_extra global_arrdata_width
          ladd global_setup_extra global_arrdata_scale
          ladd global_setup_extra global_arrdata_vis
          ladd global_setup_extra global_arrdata_style
        }
        default {
        puts "unknown key $key; 'name' or 'value' are expected"
        }
      }
    }
  }
}

proc newclient {channel} {
  global global_pathes global_arrays hi
  global global_arrdata_limit
  global global_arrdata_color global_arrdata_width global_arrdata_scale
  global global_arrdata_vis global_arrdata_style
  global global_datacachedir

  if {[gets $channel x]==-1} {
    close $channel
    return
  }
#  puts "newclient: got $x"
  set key [lindex $x 0]
  set name [lindex $x 1]
#  puts "newclient: key=$key name=$name"
  switch -exact $key {
    name {
#       puts "new connection with name $name"
      if {[info exists global_arrays($name)]} {
        if {$global_arrays($name)!=""} {
      #   puts "array $name already in use by $global_arrays($name)"
          close $channel
        } else {
#         puts "use old array"
          set global_pathes($channel) $name
          set global_arrays($name) $channel
          fileevent $channel readable "newdata $channel"
        }
      } else {
#       puts "create new array"
        if [catch {histoarray  -type file -file "$global_datacachedir/$name" $name} mist] {
          puts $mist
          close $channel
        } else {
          set global_pathes($channel) $name
          set global_arrays($name) $channel
          if [info exists global_arrdata_limit($name)] {
            set limit $global_arrdata_limit($name)
            if {[llength $limit]==1} {
              $name limit $limit
            } else {
              $name limit [lindex $limit 0] [lindex $limit 1]
            }
          }
          # set color width scale vis limit style
          if {![info exists global_arrdata_color($name)]} {
            set global_arrdata_color($name) [newcolor]
          }
          if {![info exists global_arrdata_width($name)]} {
            set global_arrdata_width($name) 0
          }
          if {![info exists global_arrdata_scale($name)]} {
            set global_arrdata_scale($name) 0
          }
          if {![info exists global_arrdata_vis($name)]} {
            set global_arrdata_vis($name) 1
          }
          if {![info exists global_arrdata_style($name)]} {
            set global_arrdata_style($name) boxes
          }
          $hi(win) attach $name\
              -color $global_arrdata_color($name)\
              -width $global_arrdata_width($name)\
              -exp $global_arrdata_scale($name)\
              -enabled $global_arrdata_vis($name)\
              -style $global_arrdata_style($name)
          fileevent $channel readable "newdata $channel"
          legend_update
          ladd global_setup_extra global_arrdata_limit
          ladd global_setup_extra global_arrdata_color
          ladd global_setup_extra global_arrdata_width
          ladd global_setup_extra global_arrdata_scale
          ladd global_setup_extra global_arrdata_vis
          ladd global_setup_extra global_arrdata_style
        }
      }
    }
    master {
      #puts "new mastersource: $name"
      fileevent $channel readable "masterdata $channel"
    }
    default {
      puts "unknown key $key; 'name' or 'master' are expected"
      close $channel
    }
  }
}

proc new_connection {channel address port} {
  global global_commu 
  set global_peers($channel) $address
#  puts "new client ($address:$port) on $channel"
  fileevent $channel readable "newclient $channel"
}

proc comm_init {port} {
  global global_commu

  for {set pforte $port; set ok 0} {!$ok} {} {
    if [catch {socket -server new_connection $pforte} mist] {
      puts "port $pforte: $mist"
      incr pforte
    } else {
      set ok 1
    }
  }
  puts "use port $pforte"
  set global_commu(sock) $mist
  return $pforte
}
