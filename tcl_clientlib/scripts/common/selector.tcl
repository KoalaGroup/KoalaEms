# $Id: selector.tcl,v 1.1 1999/05/07 15:18:41 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#

proc selector_ok {w} {
  global global_select_var
  set global_select_var [$w.l curselection]
}

proc selector_cancel {w} {
  global global_select_var
  set global_select_var -1
}

proc selector {w title label text default_a default_e selectmode} {
    global global_select_var

    # 1. Create the top-level window and divide it into top
    # and bottom parts.

    catch {destroy $w}
    toplevel $w -class Selector
    wm title $w $title
    wm iconname $w Selector
    wm protocol $w WM_DELETE_WINDOW { }

    # The following command means that the dialog won't be posted if
    # [winfo parent $w] is iconified, but it's really needed;  otherwise
    # the dialog can become obscured by other windows in the application,
    # even though its grab keeps the rest of the application from being used.

    wm transient $w [winfo toplevel [winfo parent $w]]

    label $w.h -text $label
    listbox $w.l -height [llength $text] -selectmode $selectmode
    foreach t $text {
      $w.l insert end $t
    }
    $w.l selection set $default_a $default_e

    frame $w.b
    button $w.b.ok -text OK -default active\
        -command "selector_ok $w"
    button .ask_for_tape.b.cancel -text Cancel \
        -command "selector_cancel $w"
    pack $w.b.ok $w.b.cancel -side left -expand 1 -fill x
    pack $w.h .ask_for_tape.l -side top -fill both
    pack $w.b -side bottom -fill both
    pack $w.l -side top -expand 1 -fill both

#     if {$default >= 0} {
# 	bind $w <Return> "
# 	    $w.button$default configure -state active -relief sunken
# 	    update idletasks
# 	    after 100
# 	    set tkPriv(button) $default
# 	"
#     }

    # 5. Create a <Destroy> binding for the window that sets the
    # button variable to -1;  this is needed in case something happens
    # that destroys the window, such as its parent window being destroyed.

    bind $w <Destroy> {set global_select_var -1}

    # 6. Withdraw the window, then update all the geometry information
    # so we know how big it wants to be, then center the window in the
    # display and de-iconify it.

    wm withdraw $w
    update idletasks
    set x [expr {[winfo screenwidth $w]/2 - [winfo reqwidth $w]/2 \
	    - [winfo vrootx [winfo parent $w]]}]
    set y [expr {[winfo screenheight $w]/2 - [winfo reqheight $w]/2 \
	    - [winfo vrooty [winfo parent $w]]}]
    wm geom $w +$x+$y
    wm deiconify $w

    # 7. Set a grab and claim the focus too.

    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {$oldGrab != ""} {
	set grabStatus [grab status $oldGrab]
    }
    grab $w
    focus $w

    # 8. Wait for the user to respond, then restore the focus and
    # return the index of the selected button.  Restore the focus
    # before deleting the window, since otherwise the window manager
    # may take the focus away so we can't redirect it.  Finally,
    # restore any grab that was in effect.

    tkwait variable global_select_var
    catch {focus $oldFocus}
    catch {
	# It's possible that the window has already been destroyed,
	# hence this "catch".  Delete the Destroy handler so that
	# global_select_var doesn't get reset by it.

	bind $w <Destroy> {}
	destroy $w
    }
    if {$oldGrab != ""} {
	if {$grabStatus == "global"} {
	    grab -global $oldGrab
	} else {
	    grab $oldGrab
	}
    }
    return $global_select_var
}
