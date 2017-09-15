// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	pcilocal.h -- for PCI local bus access functions

int region_access_menu(
	void	(*reg_info)(void),
	int	(*bus_error)(void));

int eeprom_menu(void);
