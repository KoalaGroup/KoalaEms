#
#
#

proc load_modullist {ved} {
  global global_setup

  output "check modullist for VED [$ved name] ..."
  update idletasks

  set ml [$ved namelist domain ml]
  puts "ml ist $ml"

  return 0
}
