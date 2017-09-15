
proc output_go {} {
  global global_output

  set global_output(spath) [socket $global_beam(host) $global_beam(port)]
  set bl [binary format c 10]
  puts -nonewline $global_beam(path) $bl
  flush $global_beam(path)
  fileevent $global_beam(path) readable "beam_data $global_beam(path)"
}



proc start_output_loop {} {
  #after 1000 output_go
}
