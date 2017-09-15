typedef struct {
   int    motoradresse,
          motortyp,
          encoderadresse,
          encodertyp,
          encoder,
          encoderalt,
          justieren,
          pulse,
          direction,
          ziel,
          dif,
          again,
          signal_timeout,
          signal_look,
          alm_set_id,
          alm_cycle_id,
          var,
          try,
          t_out,
          rep,
          channel;
} stdelement;

typedef 
struct listelement {
   stdelement         element; 
   struct listelement *next;
} listpointer;

