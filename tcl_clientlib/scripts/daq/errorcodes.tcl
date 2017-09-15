# $ZEL: errorcodes.tcl,v 1.4 2009/08/19 17:00:42 wuestner Exp $
#   P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc ems_errcode {code} {
  global ems_errcodes

  if [info exists ems_errcodes($code)] {
    return $ems_errcodes($code)
  } else {
    return [format {unknown ems errorcode %d} $code]
  }
}

proc ems_plcode {code} {
  global ems_plcodes

  if [info exists ems_plcodes($code)] {
    return $ems_plcodes($code)
  } else {
    return [format {unknown ems plcode %d} $code]
  }
}

proc ems_rtcode {code} {
  global ems_rtcodes

  if [info exists ems_rtcodes($code)] {
    return $ems_rtcodes($code)
  } else {
    return [format {unknown ems rtcode %d} $code]
  }
}
