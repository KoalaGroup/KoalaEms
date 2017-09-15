struct profistatus{
  /* Busparameter */
  char station;
  char baud_rate;
  unsigned short tsl;
  unsigned short min_tsdr;
  unsigned short max_tsdr;
  int ttr;
  unsigned char tqui;
  unsigned char tset;
  char g;
  char hsa;
  char max_retry_limit;
  char dummy[3]; /* align */
  /* Statistik */
  unsigned int frame_send_count;
  unsigned int sd_count;
  unsigned short retry_count;
  unsigned short sd_error_count;
  /* Real Target Rotation Time */
  unsigned int trr;
};


