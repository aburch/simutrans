/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_OTRP_LOG_SENDER_H
#define NETWORK_OTRP_LOG_SENDER_H

#include "../simtypes.h"

class otrp_log_sender_t
{
private:
  uint32 launch_id;
  
public:
  otrp_log_sender_t();
  
  void send_launch_log();
  void send_quit_log();
};

#endif
