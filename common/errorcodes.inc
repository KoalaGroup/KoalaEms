dnl ***************************************************************************
dnl                                                                           *
dnl errorcodes.inc                                                            *
dnl                                                                           *
dnl $ZEL: errorcodes.inc,v 2.23 2011/08/13 19:29:33 wuestner Exp $
dnl 25.11.94                                                                  *
dnl 08.06.2000 translated (PW)                                                *
dnl                                                                           *
dnl ***************************************************************************

e_code(IllRequest      ,illegal request type)
e_code(NotImpl         ,request is not implemented)

e_code(ArgNum          ,wrong number of arguments)
e_code(ArgRange        ,argument outside of valid range)

e_code(IllObject       ,object type unknown or not implemented)
e_code(ObjNonDel       ,object cannot be deleted)
e_code(ObjDef          ,object is already defined)

e_code(IllVar          ,invalid variable index)
e_code(NoVar           ,variable not defined)
e_code(VarDef          ,variable already defined)
e_code(IllVarSize      ,variable size does not match)

e_code(IllDomain       ,illegal domain index)
e_code(NoDomain        ,domain does not exist)
e_code(DomainDef       ,domain already defined)
e_code(IllDomainType   ,unknown domain type)

e_code(IllIS           ,illegal instrumentationsystem index)
e_code(NoIS            ,instrumentation system not defined)
e_code(ISDef           ,instrumentation system already defined)
e_code(NoISModulList   ,no instrumentation system modullist)
e_code(NoReadoutList   ,no readoutlist for this trigger number)
e_code(IllTrigPatt     ,illegal trigger pattern)

e_code(IllPI           ,invalid index for program invocation)
e_code(NoPI            ,program invocation not defined)
e_code(PIDef           ,program invocation already defined)
e_code(IllPIType       ,unknown type of program invocation)
e_code(PIActive        ,program invocation is active)
e_code(PINotActive     ,program invocation is not active)
e_code(NoTrigger       ,no trigger information loaded)
e_code(NoSuchTrig      ,unknown trigger procedure)

e_code(IllCapTyp       ,capability type not available)
e_code(BadProcList     ,error during test of procedure list)
e_code(ExecProcList    ,error during execution of procedure list)

e_code(TapeOpen_       ,tape already open)
e_code(TapeNotOpen_    ,tape not open)
e_code(DataoutBusy     ,output device is busy)
e_code(TapeEnabled_    ,tape already enabled)
e_code(TapeNotEnabled_ ,tape not enabled)

e_code(NoMem           ,not enough memory)
e_code(BufOverfl       ,buffer overflow)
e_code(NoVED           ,VED-ID not known)
e_code(System          ,error in system call)

e_code(Program         ,program error)
e_code(IllDo           ,invalid dataout index)
e_code(NoDo            ,dataout object does not exist)
e_code(TrigProc        ,error in the trigger procedure or its arguments)
e_code(Other           ,other error)
e_code(HWTestFailed    ,hardware test failure)
e_code(HW              ,hardware failure)
e_code(BadModTyp       ,bad module type)
e_code(NotOwner        ,not owner of object)
e_code(AddrTypNotImpl  ,addresstype not implemented)
e_code(BufTypNotImpl   ,buffertype not implemented)
e_code(BadModClass     ,bad module class)


pl_code(NoSuchProc     ,illegal procedure id)
pl_code(BadArgCounter  ,number of arguments ecceeds length of readoutlist)
pl_code(ArgNum         ,wrong number of arguments)
pl_code(ArgRange       ,argument outside of valid range)
pl_code(IllVar         ,illegal index of variable)
pl_code(NoVar          ,variable not defined)
pl_code(IllVarSize     ,illegal size of variable)
pl_code(NoMem          ,out of memory)
pl_code(BadHWAddr      ,bad hardware address)
pl_code(NoISModulList  ,no instrumentation system modullist)
pl_code(BadISModList   ,unappropriate modullist for IS)
pl_code(IllModAddr     ,illegal address in modullist)
pl_code(NotInModList   ,module type not found)
pl_code(BadModTyp      ,bad module type)
pl_code(HWTestFailed   ,hardware test failure)
pl_code(HW             ,hardware failure)
pl_code(VarDef         ,variable already defined)
pl_code(Other          ,other error)
pl_code(RecursiveCall  ,error in recursion)
pl_code(System         ,error in system call)
pl_code(Program        ,program error)
pl_code(Busy           ,device busy)
pl_code(Overflow       ,data overflow)
pl_code(Offline        ,device offline)
pl_code(Verify         ,verify failed)
pl_code(Timeout        ,command timed out)
pl_code(NotImpl        ,request is not implemented)
pl_code(InvalidUse     ,invalid command use)
pl_code(PIActive       ,program invocation is active)
pl_code(PINotActive    ,program invocation is not active)
pl_code(NoDomain       ,domain does not exist)
pl_code(NoFile         ,file does not exist)

rt_code(OutDev         ,output device failure)
rt_code(Trig           ,trigger failure)
rt_code(Mismatch       ,eventnumber mismatch)
rt_code(ExecProcList   ,failure executing procedure list)
rt_code(Other          ,other error)
rt_code(InDev          ,input device failure)
rt_code(DelayedRead    ,error during delayed read)

dnl ***************************************************************************
dnl ***************************************************************************
