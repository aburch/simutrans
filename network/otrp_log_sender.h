/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_OTRP_LOG_SENDER_H
#define NETWORK_OTRP_LOG_SENDER_H

#include "../simtypes.h"
#include <ctime>

class otrp_log_sender_t
{
private:
  uint32 launch_id;
  std::time_t launch_time;
  
public:
  otrp_log_sender_t();
  
  void send_launch_log();
  void save_statistics();
};

#endif
