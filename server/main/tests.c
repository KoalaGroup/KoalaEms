#include <stdio.h>

#include "signals.h"
#include "timers.h"
#include "scheduler.h"

void mg(){
  printf("ende!!!\n");
  print_tasks();
  print_signals();
  print_timers();
  done_sched();
  done_timers();
  done_signalhandler();
}


void aaa(){
  printf("aaa\n");
}

void nix(){
  printf("nix\n");
}

void main(){
  init_signalhandler();
  install_shutdownhandler(mg);
  init_timers();
  init_sched();

  insert_poll_task(nix,1,10000,"nix");

  exec_periodic(aaa,1,"aaa");

  sched_mainloop();;
}
