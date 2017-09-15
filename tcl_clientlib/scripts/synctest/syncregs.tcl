array set regname {
    MASTER        {master       }
    GO            {go           }
    T4LATCH       {t4latch      }
    TAW_ENA       {taw_ena      }
    AUX_OUT       {aux_out      }
    AUX_OUT0      {aux_out0     }
    AUX_OUT1      {aux_out1     }
    AUX_OUT2      {aux_out2     }
    AUX0_RST_TRIG {aux0_rst_trig}
    EN_INT        {en_int       }
    EN_INT_AUX    {en_int_aux   }
    EN_INT_0      {en_int_0     }
    EN_INT_1      {en_int_1     }
    EN_INT_2      {en_int_2     }
    EN_INT_EOC    {en_int_eoc   }
    EN_INT_SI     {en_int_si    }
    EN_INT_TRIGG  {en_int_trigg }
    EN_INT_GAP    {en_int_gap   }
    GSI           {gsi          }
    TRIG          {trig         }
    TRIG0         {trig0        }
    TRIG1         {trig1        }
    TRIG2         {trig2        }
    TRIG3         {trig3        }
    GO_RING       {go_ring      }
    VETO          {veto         }
    SI            {si           }
    INH           {inh          }
    AUX_IN        {aux_in       }
    AUX_IN0       {aux_in0      }
    AUX_IN1       {aux_in1      }
    AUX_IN2       {aux_in2      }
    EOC           {eoc          }
    SI_RING       {si_ring      }
    RES_TRIGINT   {res_trigint  }
    GAP           {gap          }
    TDT_RING      {tdt_ring     }
}

array set regmask {
    MASTER        0x00000001
    GO            0x00000002
    T4LATCH       0x00000004
    TAW_ENA       0x00000008
    AUX_OUT       0x00000070
    AUX_OUT0      0x00000010
    AUX_OUT1      0x00000020
    AUX_OUT2      0x00000040
    AUX0_RST_TRIG 0x00000080
    EN_INT        0x00007f00
    EN_INT_AUX    0x00000700
    EN_INT_0      0x00000100
    EN_INT_1      0x00000200
    EN_INT_2      0x00000400
    EN_INT_EOC    0x00000800
    EN_INT_SI     0x00001000
    EN_INT_TRIGG  0x00002000
    EN_INT_GAP    0x00004000
    GSI           0x00008000
    TRIG          0x000f0000
    TRIG0         0x00010000
    TRIG1         0x00020000
    TRIG2         0x00040000
    TRIG3         0x00080000
    GO_RING       0x00100000
    VETO          0x00200000
    SI            0x00400000
    INH           0x00800000
    AUX_IN        0x07000000
    AUX_IN0       0x01000000
    AUX_IN1       0x02000000
    AUX_IN2       0x04000000
    EOC           0x08000000
    SI_RING       0x10000000
    RES_TRIGINT   0x20000000
    GAP           0x40000000
    TDT_RING      0x80000000
}

array set regshift {
    MASTER        0
    GO            1
    T4LATCH       2
    TAW_ENA       3
    AUX_OUT       4
    AUX_OUT0      4
    AUX_OUT1      5
    AUX_OUT2      6
    AUX0_RST_TRIG 7
    EN_INT        8
    EN_INT_AUX    8
    EN_INT_0      8
    EN_INT_1      9
    EN_INT_2     10
    EN_INT_EOC   11
    EN_INT_SI    12
    EN_INT_TRIGG 13
    EN_INT_GAP   14
    GSI          15
    TRIG         16
    TRIG0        16
    TRIG1        17
    TRIG2        18
    TRIG3        19
    GO_RING      20
    VETO         21
    SI           22
    INH          23
    AUX_IN       24
    AUX_IN0      24
    AUX_IN1      25
    AUX_IN2      26
    EOC          27
    SI_RING      28
    RES_TRIGINT  29
    GAP          30
    TDT_RING     31
}
