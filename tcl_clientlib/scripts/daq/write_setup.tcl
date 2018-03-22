# $ZEL: write_setup.tcl,v 1.23 2017/11/08 00:17:20 wuestner Exp $
# copyright 1998
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_verbose               boolean
# global_setupdata
# global_daq
# global_daq(_starttime)
# global_daq(_stoptime)
# global_noteheader
# global_interps($ved)
# errorCode
# global_waitvar
# global_waitproc
#

proc write_file_header {ved do file} {
  global global_verbose
  if {$global_verbose} {output "write file header: $file"}
  set nfile [file nativename $file]
  if [catch {$ved dataout writefile $do $nfile} mist] {
    output "write filerecord to dataout $do of VED [$ved name]: $mist" tag_red
    set res -1
  } else {
    if {$global_verbose} {output "wrote $file"}
    set res 0
  }
  return $res
}

proc write_text_header {ved do header} {
  global errorCode global_waitvar global_waitproc
  global global_verbose
  set weiter 1
  set second_try 0
  if {$global_verbose} {output "write text header: [lindex $header 0]"}
#  set to [ems_timeout 190]
  while {$weiter} {
    if [catch {$ved dataout write $do $header} mist] {
      if {$errorCode=="EMS_REQ 38" || $errorCode=="EMS_REQ 35"} {
        output "write inforecord to dataout $do of VED [$ved name]: busy" tag_orange
        output_append "  will retry in 1 second"
        set global_waitvar 0
        set global_waitproc [after 1000 {wait_handler}]
        vwait global_waitvar
        if {$global_waitvar!=1} {
          output "  abgewürgt"
          set res -1
          set weiter 0
        } else {
          set second_try 1
        }
      } else {
        output "write inforecord to dataout $do of VED [$ved name]: $mist" tag_red
        output "errorCode=$errorCode"
        if {[llength $errorCode]>=3} {
          # XXX
          output_append "([strerror_bsd [lindex $errorCode 2]])"
        }
        set res -1
        set weiter 0
      }
    } else {
      if {$second_try} {output_append "  ...successfully"}
      set res 0
      set weiter 0
    }
  }
#  ems_timeout $to
  return $res
}

proc write_setup_to_dataout {ved do} {
  global global_verbose

  foreach head $::headers::textheaders {
    if {$global_verbose} {
      output "write $head to ved [$ved name] dataout $do"
    }
    if {[write_text_header $ved $do [set ::headers::$head]]<0} {return -1}
  }
  foreach head $::headers::fileheaders {
    if {$global_verbose} {
      output "write fileheader $head to ved [$ved name] dataout $do"
    }
    if {[write_file_header $ved $do $head]<0} {return -1}
  }
  return 0
}

proc write_setup_to_dataouts {veds} {
  global global_setupdata global_verbose
  global global_interps

  if {$global_verbose} {output "write_setup_to_dataouts for $veds"}

  foreach ved $veds {
    set name $global_interps($ved)
    #output "testing $ved ($name)"
    #if {[${name} eval info exists dataout_for_config_records]} {
    #  output "dataout_for_config_records: [${name} eval set dataout_for_config_records]"
    #} else {
    #  output "dataout_for_config_records does not exist"
    #}
    #if {[${name} eval info exists eventbuilder]} {
    #  output "eventbuilder: [${name} eval set eventbuilder]"
    #} else {
    #  output "eventbuilder does not exist"
    #}
    #if {[${name} eval info exists dataout]} {
    #  output "dataout: [${name} eval array names dataout]"
    #} else {
    #  output "dataout does not exist"
    #}
    if [${name} eval info exists dataout_for_config_records] {
      set dataouts [${name} eval set dataout_for_config_records]
    } elseif {[${name} eval info exists eventbuilder]
              && [${name} eval set eventbuilder]>0
              && [${name} eval info exists dataout]} {
      set dataouts [${name} eval array names dataout]
    } else {
      set dataouts {}
    }
    foreach do $dataouts {
      if {$global_verbose} {output "write setup to dataout $do of ved $ved"}
      set res [write_setup_to_dataout $ved $do]
      if {$res!=0} {return -1}
    }
  }
  return 0
}
