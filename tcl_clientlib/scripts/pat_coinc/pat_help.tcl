# $ZEL: pat_help.tcl,v 1.1 2002/09/26 12:15:12 wuestner Exp $
# P. Wuestner 2002; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

#
# Diese Prozedur wird aufgerufen, wenn eines der Kommandozeilenargumente "help"
# lautet, oder der Initialisierungscode anderweitig entscheidet, dass der
# Benutzer eine Hilfe noetig hat.
#

proc printhelp {} {
  puts "script options: \[-s {ems_socket}|\[-h {ems_host}\] \[-p {ems_port}\]\]\
 \[-setup {file}\]"
}
