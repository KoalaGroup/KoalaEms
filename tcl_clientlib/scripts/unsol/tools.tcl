proc bgerror {args} {
  output "background error:"
  output_append $args
}

proc unsol_ende {} {
  close_log
  ende
}

proc dummytool {} {
}
