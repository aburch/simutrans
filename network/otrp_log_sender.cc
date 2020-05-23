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
  launch_time = std::time(nullptr);
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

const char* build_query(uint32 id, const char* saved) {
  cbuffer_t* buf = new cbuffer_t();
  buf->printf("%s&id=%d", saved, id);
  return buf->get_str();
}

void otrp_log_sender_t::send_launch_log() {
  if(  strlen(env_t::otrp_statistics_log)==0  ) {
    // statistics log is not saved.
    dbg->warning("log_sender_t::send_launch_log()", "statistics log is not send.");
    return;
  }
  const char* query = build_query(launch_id, env_t::otrp_statistics_log);
  dbg->message("log_sender_t::send_launch_log()", "sent query: %s", query);
  std::thread thr(send_log, query);
  thr.detach();
}

// last_id, otrp version, language, os, pak, network_status, duration
void otrp_log_sender_t::save_statistics() {
  cbuffer_t os;
  get_os(os);
  uint8 network = env_t::networkmode + ((env_t::server>0)<<1);
  sprintf(env_t::otrp_statistics_log, "/registerOTRPUsage?last_id=%d&version=%d.%d&lang=%s&os=%s&pak=%s&network=%d&duration=%ld", 
  launch_id, OTRP_VERSION_MAJOR, OTRP_VERSION_MINOR, env_t::language_iso,
  os.get_str(), env_t::objfilename.c_str(), network, std::time(nullptr)-launch_time);
}
