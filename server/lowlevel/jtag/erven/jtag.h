/*!
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * ---
 * Copyright (C) 2008, Willi Erven <w.erven@fz-juelich.de>
 */

#ifndef _JTAG_H
#define _JTAG_H

//
// -------------------------- Configuration Buffer --------------------------
//
typedef struct {
	char			jtag_menu;
	char			mode;
	u_int16_t	jtag_ctrl;
	u_int16_t	jtag_dev;
	u_int16_t	jtag_instr;
	u_int16_t	jtag_dlen;
	u_int32_t	jtag_data;

#define C_JT_NM		5
	char		jtag_file[C_JT_NM][80];
	char		jtag_svf_file[C_JT_NM][80];
} JT_CONF;
//
//--------------------------- global function -------------------------------
//
int JTAG_menu(
	void			(*jtag_putcsr)(u_int16_t),
	u_int16_t	(*jtag_getcsr)(void),
	u_int32_t	(*jtag_getdata)(void),
	JT_CONF		*jtag_conf,
	u_char		jtag_mode);

#endif
