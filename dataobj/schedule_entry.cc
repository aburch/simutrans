#include "schedule_entry.h"
#include "../simworld.h"

bool schedule_entry_t::operator ==(const schedule_entry_t &a) {
  return a.pos == this->pos
    &&  a.minimum_loading    == this->minimum_loading
    &&  a.waiting_time_shift == this->waiting_time_shift
    &&  a.get_stop_flags()   == this->stop_flags
    &&  a.spacing            == this->spacing
    &&  a.spacing_shift      == this->spacing_shift
    &&  a.delay_tolerance    == this->delay_tolerance;
}

schedule_entry_t& schedule_entry_t::operator = (const schedule_entry_t& a) {
  if(  this!=&a  ) {
    this->pos = a.pos;
    this->minimum_loading = a.minimum_loading;
    this->waiting_time_shift = a.waiting_time_shift;
    this->stop_flags = a.get_stop_flags();
    this->spacing = a.spacing;
    this->spacing_shift = a.spacing_shift;
    this->delay_tolerance = a.delay_tolerance;
    departure_slots.clear();
  }
  return *this;
}

schedule_entry_t::schedule_entry_t(const schedule_entry_t &a) {
  *this = a;
}

void schedule_entry_t::set_spacing(uint16 a, uint16 b, uint16 c) {
  spacing = a;
  spacing_shift = b;
  delay_tolerance = c;
}
