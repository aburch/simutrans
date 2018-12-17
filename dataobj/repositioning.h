#ifndef repositioning_h
#define repositioning_h

#include "../tpl/stringhashtable_tpl.h"
#include "koord.h"

/*
 * singleton.
 *
 * @author THLeaderH
 */

class koord;
class vehicle_desc_t;

class repositioning_t {
  
private:
  repositioning_t() {};
  ~repositioning_t() {};
  static repositioning_t instance;
  stringhashtable_tpl<koord> table;
  static koord default_offset;
  static char tab_filename[128];
  
public:
  static repositioning_t& get_instance() { return instance; };
  void set_offset(const char*, koord);
  void set_offset(const char* name) { set_offset(name, default_offset); }
  bool cancel_offset(const char*);
  koord get_offset(const char*);
  koord get_default_offset() const { return default_offset; }
  
  // these 2 functions should be called from simmain()
  void read_tabfile(const char*);
  void write_tabfile();
};

#endif