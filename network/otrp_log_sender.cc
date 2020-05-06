/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "otrp_log_sender.h"

#include "network_file_transfer.h"
#include "../dataobj/environment.h"
#include "../simdebug.h"
#include "../simversion.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simrandom.h"

#include <thread>

void send_log(const char* query) {
  cbuffer_t dummy;
  const char *err = network_http_get( "himeshi.dip.jp:15000", query, dummy );
  if(  err  &&  dbg  ) {
    dbg->warning("log_sender_t::send_launch_log()", "%s", err);
  }
  delete query;
}

otrp_log_sender_t::otrp_log_sender_t() {
  launch_id = simrand((1<<15));
}

// id, otrp version, language, os, pak, status
const char* build_query(uint32 id, const char* status) {
  cbuffer_t* buf = new cbuffer_t();
  buf->printf("/registerOTRPUsage?id=%d&version=%d.%d&lang=%s&os=%s&pak=%s&stat=%s", 
  id, OTRP_VERSION_MAJOR, OTRP_VERSION_MINOR, env_t::language_iso,
  "windows", "pak128", status);
  return buf->get_str();
}

void otrp_log_sender_t::send_launch_log() {
  const char* query = build_query(launch_id, "launch");
  dbg->message("log_sender_t::send_launch_log()", "send query: %s", query);
  std::thread thr(send_log, query);
  thr.detach();
}

void otrp_log_sender_t::send_quit_log() {
  const char* query = build_query(launch_id, "quit");
  dbg->message("log_sender_t::send_quit_log()", "send query: %s", query);
  std::thread thr(send_log, query);
  thr.detach();
}
