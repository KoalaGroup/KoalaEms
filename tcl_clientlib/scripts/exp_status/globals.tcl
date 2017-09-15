proc init_globals {} {
  global global_ready

  set global_ready 0
}
proc init_setup {} {
  global global_output
  global global_beam
  global global_beam_data
  global global_exp
  global global_exp_data

  set global_output(port) 22223

  set global_beam(host) zel385
  set global_beam(port) 22222
  set global_beam(bct) 0
  set global_beam(c11_targ) 1
  set global_beam_data(valid) 0
  # global_beam_data(time)
  # global_beam_data(bct)
  # global_beam_data(targ)

  set global_exp(host) ikpe1103
  set global_exp(port) 4096
  set global_exp(veds) {e01 e02 e03 e04 e06 e07}
  set global_exp_data(valid) 0
  # global_exp_data(triggrate)
  # global_exp_data(evrate)
  # global_exp_data(reldt)
  # global_exp_data(status) {{e01 running 12345} {...} ...}
  # global_exp_data(tape)?
}
