# opens a VED and determines the typ of it
# reopens that VED with the correct type qualifier
# writes global vars:
#   ved      // ved command
#   vedtype  // type of VED
#   namelist // list of implemented objects
#
# side effect:
#   a VED object is created

proc open_ved {name} {
global ved

if [catch {set ved [ems_open $name]} mist] {
  puts $mist
  return -1
  }
return 0
}
