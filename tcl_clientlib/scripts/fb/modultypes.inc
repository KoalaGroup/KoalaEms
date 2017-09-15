dnl ***************************************************************************
dnl                                                                           *
dnl modultypes.inc                                                            *
dnl                                                                           *
dnl 15.03.94; update: 26.10.94                                                *
dnl                                                                           *
dnl ***************************************************************************
dnl
dnl  R. Nellen                                               03.03.1993
dnl  KFA-IKP-DV
dnl
dnl          Module_Id fuer COSY Experiment Hardware
dnl
dnl
dnl  8 Character jeweils von 0 bis f zur Darstellung im HEX Format
dnl
dnl
dnl  -----------------------------------------------------------------
dnl  |       |       |       |       |       |       |       |       |
dnl  |       |       |       |       |       |       |       |       |
dnl  |       |       |       |       |       |       |       |       |
dnl  -----------------------------------------------------------------
dnl    |        |        |   \___________________  __________________/
dnl    |        |        |                       \/
dnl  BUS-Type Function Factory                Module Type
dnl
dnl  1=CAMAC  1=ADC    1=LeCroy       e.g.   4300b
dnl  2=VME    2=QDC    2=Silena              04301
dnl  3=DL350  3=TDC    3=Struck              043fc ==> FSC
dnl  4=Fastb. 4=Scaler 4=Kinetics            043be ==> Bitpattern
dnl           5=Flash  5=Philips
dnl           6=Contr. 6=CES
dnl           7=Memory 7=GSI
dnl           8=Logic  8=CERN
dnl           9=Bitpat 9=CAEN
dnl           a=Trig.M.
dnl
dnl           c=CPU
dnl
dnl           f=other  f=other

dnl CAMAC

module(FERA_ADC_4300B    ,0x1214300B)
module(FERA_TDC_4300B    ,0x1314300B)
module(FERA_DRV_4301     ,0x16104301)
module(FERA_MEM_4302     ,0x17104302)
module(FERA_TFC_4303     ,0x1f104303)
module(FSC               ,0x16f043FC)
module(BIT_PATTERN_UNIT  ,0x19f043BE)
module(LC_TDC_2277       ,0x13102277)
module(LC_TDC_2228       ,0x1312228A)
module(LC_TDC_2229       ,0x13102229)
module(SILENA_ADC_4418V  ,0x11204418)
module(CNRS_QDC_1612F    ,0x12F1612F)
module(CNRS_TDC_812F     ,0x13F0812F)
module(RAL11X_RDOUT	,0x18f0011a)

module(LC_SCALER_2551    ,0x14102551)
module(LC_SCALER_4434    ,0x14104434)
module(VCC2117           ,0x1c602117)

module(GSI_TRIGGER       ,0x1A70C100)
module(DRAMS_RECEIVER    ,0x1180D512)
module(DRAMS_CONTROL     ,0x168000DC)
module(DRAMS_EVENT       ,0x18800ECD)

module(SCALER_JEA20      ,0x19000010)
module(CES_HSM2170       ,0x19000011)
module(CES_HM2161        ,0x19000012)
module(IM_JIR10          ,0x19000013)

module(CAEN_DISC_C193    ,0x1f90C193)
module(LC_LOGIC_4516     ,0x18104516)
module(LC_DISC_4413      ,0x1f104413)
module(LC_REGISTER_2371  ,0x1f102371)

module(CAENNET_SLOW      ,0x1690c139)

module(IGOR              ,0x1f403060)
module(CAEN_PROG_IO      ,0x1890C219)

dnl FASTBUS

module(PH_ADC_10C2       ,0x415010C2)
module(PH_TDC_10C6       ,0x435010C6)
module(STR_330           ,0x4C3068A0)
module(GSI_TRIGGER_FB    ,0x4A70F100)
module(FVSBI             ,0x466068B7)
module(KIN_ADC_F465      ,0x4140100A)
module(KIN_TDC_F432      ,0x43401008)
module(LC_CAT_1810       ,0x48101039)
module(LC_TDC_1872A      ,0x43101036)
module(LC_TDC_1875       ,0x43101049)
module(LC_TDC_1875A      ,0x43101037)
module(LC_TDC_1876       ,0x4310103B)
module(LC_TDC_1877       ,0x4310103C)
module(LC_TDC_1879       ,0x43101035)
module(LC_ADC_1881       ,0x4110104B)
module(LC_ADC_1881M      ,0x4110104F)
module(LC_ADC_1885F      ,0x41101045)
module(STR_197           ,0x4F306854)

dnl VME

module(FIC_8232          ,0x2C608232)
module(FIC_8234          ,0x2C608234)
module(ELTEC_E6          ,0x2CF000E6)
module(GSI_TRIGGER_VME   ,0x2A70E100)
module(CES_8251          ,0x2F608251)

dnl ????

module(NE526             ,526)
module(E217              ,217)
module(E362              ,362)
module(ESSI              ,422)
module(LED_2800          ,2800)
module(ROC717            ,717)
module(E233_0            ,2330)
module(E233_1            ,2331)
module(CHANNEL           ,123)
module(ILL_CHANNEL       ,30)
module(IEC               ,100)

dnl ***************************************************************************
dnl ***************************************************************************
