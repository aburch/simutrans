#ifndef route_block_tester_t_h
#define route_block_tester_t_h

#include "../dataobj/koord3d.h"

class route_block_tester_t
{
 public:

  virtual bool is_blocked(koord3d pos, koord3d dest) const = 0;

};

#endif // route_block_tester_t_h
