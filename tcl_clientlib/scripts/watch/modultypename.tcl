# returns (if possible) the typename of a module

proc modultypename {id} {
global global_mtypes
  
if {[info exists global_mtypes] == 0} load_modultypes

foreach i [array names global_mtypes] {
  if {$i==$id} {
    return [set global_mtypes($i)]
  }
}
return {}
}
