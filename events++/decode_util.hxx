#ifndef _decode_util_hxx_
#define _decode_util_hxx_

#include "global.hxx"
#include "mxdc32_data.hxx"

/* DecodeUtils contains the functions needed to decode a ems cluster.
   The algorithm is like this:
   1) Get a buffer containing a complete cluster. The cluster might comes
      from the file, socket or stdin or anywhere.
   2) parse_cluster->parse_events->parse_option->parse_event->parse_subevent
      parse_subevent will deploy the actual decoding work to specific parser
      based on the IS id. Three IS-es, thus three parser functions are defi-
      ned. They are parse_mxdc32 for QDC,ADC,TDC data, parse_scalor for sca-
      lor and pare_time for time infos.
   3) The decoding is organized event by event. It means after finishing de-
      coding one ems event, the decoded data will be processed or analyzed.
      The interface for the actual analyzing algorithm is collect_koala_data.
   4) parse_time and parse_scalor are quite straight-forward: the time info
      and scalor info for each ems event (each ems event correspond to each
      VME interrupt, which again corresponds to each CBLT readout) are deco-
      ded and saved in the ems_event buffer
   5) Each ems event contains multiple events for each Mesytec MXDC module.
      parse_mxdc32 will decode all the mxdc32 events of each VME module and
      save them in their corresponding buffer(i.e. mxdc32_private)
   6) collect_koala_event will loop through all the modules' buffer and try
      to assemble a complete experimental event and saved into koala_event
      for later processing.
*/

namespace DecodeUtil
{
  int mxdc32_get_idx_by_id(uint32_t id);
  int check_size(const char* txt, int idx, int size, int needed);
  void dump_data(const char* txt, const uint32_t *buf, int size, int idx);
}

#endif
