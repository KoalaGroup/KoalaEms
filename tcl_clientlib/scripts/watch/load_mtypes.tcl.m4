#
# global vars in this file:
#
# global_mtypes
#

proc load_modultypes {} {
  global global_mtypes
define(`module', `set global_mtypes($2) $1')
include(/usr/users/wuestner/ems/common/modultypes.inc)
}
