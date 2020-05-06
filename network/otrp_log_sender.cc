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
  launch_id = simrand((1<<31));
}

void get_os(cbuffer_t& buf) {
  #ifdef _WIN32
  buf.printf("windows");
  #elif __APPLE__
  buf.printf("mac");
  #elif __HAIKU__
  buf.printf("haiku");
  #elif __BEOS__
  buf.printf("beos");
  #elif __AMIGA__
  buf.printf("amiga");
  #else
  buf.printf("other");
  #endif
}

// id, otrp version, language, os, pak, network_status, status
const char* build_query(uint32 id, const char* status) {
  cbuffer_t os;
  get_os(os);
  uint8 network = env_t::networkmode;
  cbuffer_t* buf = new cbuffer_t();
  buf->printf("/registerOTRPUsage?id=%d&version=%d.%d&lang=%s&os=%s&pak=%s&network=%d&stat=%s", 
  id, OTRP_VERSION_MAJOR, OTRP_VERSION_MINOR, env_t::language_iso,
  os.get_str(), env_t::objfilename.c_str(), network, status);
  return buf->get_str();
}

void otrp_log_sender_t::send_launch_log() {
  const char* query = build_query(launch_id, "launch");
  dbg->message("log_sender_t::send_launch_log()", "sent query: %s", query);
  std::thread thr(send_log, query);
  thr.detach();
}

void otrp_log_sender_t::send_quit_log() {
  const char* query = build_query(launch_id, "quit");
  dbg->message("log_sender_t::send_quit_log()", "sent query: %s", query);
  std::thread thr(send_log, query);
  thr.detach();
}
