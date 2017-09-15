#
# Diese Prozedur wird aufgerufen, wenn eines der Kommandozeilenargumente "help"
# lautet, oder der Initialisierungscode anderweitig entscheidet, dass der
# Benutzer eine Hilfe noetig hat.
#

proc printhelp {} {
  global argv
  puts "usage: $argv0 [-s {ems_socket}|\[-h {ems_host}\] \[-p {ems_port}\]\] 
  \[-setup {file}\] \[-port {dataport}\] \[-gui\]"
}
