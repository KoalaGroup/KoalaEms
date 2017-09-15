/*
 * lowlevel/unixvme/vme_address_modifiers.h
 * $ZEL: vme_address_modifiers.h,v 1.1 2002/09/13 14:51:10 wuestner Exp $
 * created 07.Sep.2002 PW
 */

#ifndef _vme_address_modifiers_h_
#define _vme_address_modifiers_h_

#define AM_a24_super_block        0x3f
#define AM_a24_super_program      0x3e
#define AM_a24_super_data         0x3d
#define AM_a24_super_64_bit_block 0x3c
#define AM_a24_block              0x3b
#define AM_a24_program            0x3a
#define AM_a24_data               0x39
#define AM_a24_64_bit_block       0x38
#define AM_a40blt                 0x37
#define AM_reserved_36            0x36
#define AM_a40_lock               0x35
#define AM_a40_access             0x34
#define AM_reserved_33            0x33
#define AM_a24_lock               0x32
#define AM_reserved_31            0x31
#define AM_reserved_30            0x30
#define AM_configuration_rom      0x2f
#define AM_reserved_2e            0x2e
#define AM_a16_super              0x2d
#define AM_a16_lock               0x2c
#define AM_reserved_2b            0x2b
#define AM_reserved_2a            0x2a
#define AM_a16                    0x29
#define AM_reserved_28            0x28
#define AM_reserved_27            0x27
#define AM_reserved_26            0x26
#define AM_reserved_25            0x25
#define AM_reserved_24            0x24
#define AM_reserved_23            0x23
#define AM_reserved_22            0x22
#define AM_2e_vme_3u              0x21
#define AM_2e_vme_6u              0x20
#define AM_user_defined_1f        0x1f
#define AM_user_defined_1e        0x1e
#define AM_user_defined_1d        0x1d
#define AM_user_defined_1c        0x1c
#define AM_user_defined_1b        0x1b
#define AM_user_defined_1a        0x1a
#define AM_user_defined_19        0x19
#define AM_user_defined_18        0x18
#define AM_user_defined_17        0x17
#define AM_user_defined_16        0x16
#define AM_user_defined_15        0x15
#define AM_user_defined_14        0x14
#define AM_user_defined_13        0x13
#define AM_user_defined_12        0x12
#define AM_user_defined_11        0x11
#define AM_user_defined_10        0x10
#define AM_a32_super_block        0x0f
#define AM_a32_super_program      0x0e
#define AM_a32_super_data         0x0d
#define AM_a32_super_64_bit_block 0x0c
#define AM_a32_block              0x0b
#define AM_a32_program            0x0a
#define AM_a32_data               0x09
#define AM_a32_64_bit_block       0x08
#define AM_reserved_07            0x07
#define AM_reserved_06            0x06
#define AM_a32_lock               0x05
#define AM_a64_lock               0x04
#define AM_a64_block              0x03
#define AM_reserved_02            0x02
#define AM_a64_single             0x01
#define AM_a64_64_bit_block       0x00

#endif
