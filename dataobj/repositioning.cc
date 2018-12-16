#include "repositioning.h"
#include "../descriptor/vehicle_desc.h"

repositioning_t repositioning_t::instance;
koord repositioning_t::default_offset = koord(0,4);

void repositioning_t::set_offset(const char* name, koord k) {
  table.set(name, k);
}

bool repositioning_t::cancel_offset(const char* name) {
  koord* k = table.access(name);
  if(  k==NULL  ||  *k==koord(0,0)  ) {
    // given vehicle is not repositioned.
    return false;
  } else {
    table.remove(name);
    return true;
  }
}

koord repositioning_t::get_offset(const char* c) {
  koord* k = table.access(c);
  if(  k  ) {
    return *k;
  } else {
    return koord(0,0);
  }
}