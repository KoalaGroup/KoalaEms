# $ZEL: modultypes.tcl,v 1.4 2003/02/04 19:27:55 wuestner Exp $
# P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

array set modultype {
 FERA_ADC_4300B     0x1214300B
 FERA_TDC_4300B     0x1314300B
 FERA_DRV_4301      0x16104301
 FERA_MEM_4302      0x17104302
 FERA_TFC_4303      0x1f104303
 FSC                0x16f043FC
 BIT_PATTERN_UNIT   0x19f043BE
 LC_TDC_2277        0x13102277
 LC_TDC_2228        0x1312228A
 LC_TDC_2229        0x13102229
 SILENA_ADC_4418V   0x11204418
 CNRS_QDC_1612F     0x12F1612F
 CNRS_TDC_812F      0x13F0812F
 RAL11X_RDOUT       0x18f0011a

 LC_SCALER_2551     0x14102551
 LC_SCALER_4434     0x14104434
 VCC2117            0x1c602117

 GSI_TRIGGER        0x1A70C100
 DRAMS_RECEIVER     0x1180D512
 DRAMS_CONTROL      0x168000DC
 DRAMS_EVENT        0x18800ECD

 SCALER_JEA20       0x19000010
 CES_HSM2170        0x19000011
 CES_HM2161         0x19000012
 IM_JIR10           0x19000013

 CAEN_DISC_C193     0x1f90C193
 LC_LOGIC_4516      0x18104516
 LC_DISC_4413       0x1f104413
 LC_REGISTER_2371   0x1f102371

 CAENNET_SLOW       0x1690c139

 IGOR               0x1f403060
 CAEN_PROG_IO       0x1890C219

 PH_ADC_10C2        0x415010C2
 PH_TDC_10C6        0x435010C6
 STR_330            0x4C3068A0
 GSI_TRIGGER_FB     0x4A70F100
 FVSBI              0x466068B7
 KIN_ADC_F465       0x4140100A
 KIN_TDC_F432       0x43401008
 LC_CAT_1810        0x48101039
 LC_TDC_1872A       0x43101036
 LC_TDC_1875        0x43101049
 LC_TDC_1875A       0x43101037
 LC_TDC_1876        0x4310103B
 LC_TDC_1877        0x4310103C
 LC_TDC_1879        0x43101035
 LC_ADC_1881        0x4110104B
 LC_ADC_1881M       0x4110104F
 LC_ADC_1885F       0x41101045
 STR_197            0x4F306854

 FIC_8232           0x2C608232
 FIC_8234           0x2C608234
 ELTEC_E6           0x2CF000E6
 GSI_TRIGGER_VME    0x2A70E100
 CES_8251           0x2F608251

 NE526              526
 E217               217
 E362               362
 ESSI               422
 LED_2800           2800
 ROC717             717
 E233_0             2330
 E233_1             2331
 CHANNEL            123
 ILL_CHANNEL        30
 IEC                100
}

proc mtype {name} {
  global modultype
  if [info exists modultype($name)] {
    return $modultype($name)
  } else {
    puts "modultype $name is not known; please update modultypes.tcl"
    return -1
  }
}

proc modulname {type} {
  global modultype
  set mname {}
  foreach name [array names modultype] {
    if {$type==$modultype($name)} {
      set mname $name
      break
    }
  }
  if {$mname=={}} {
    return "unknown modultype [format {0x%08x} $type]"
  } else {
    return $mname
  }
}

proc fbmodulname {type} {
  global modultype
  set mname {}
  # set type [expr ($type>>16) & 0xffff]
  foreach name [array names modultype] {
    set mtype $modultype($name)
    if {(($mtype&0xf0000000)==0x40000000) && (($mtype&0xffff)==$type)} {
      set mname $name
      break
    }
  }
  if {$mname=={}} {
    return "unknown modultype [format {0x%04x} $type]"
  } else {
    return $mname
  }
}

