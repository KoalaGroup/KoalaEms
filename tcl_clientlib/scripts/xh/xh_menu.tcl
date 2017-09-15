proc create_menu {parent}	{
	set self [string trimright $parent .].menu
	frame $self
	set filemenu [create_menu_file $self]
	set setupmenu  [create_menu_setup $self]
	set commenu [create_menu_commands $self]
	set helpmenu [create_menu_help $self]
	pack $filemenu $setupmenu $commenu -side left -padx 1m
	pack $helpmenu -side right -anchor e
	return $self
}
