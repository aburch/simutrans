#include "repositioning.h"
#include "tabfile.h"
#include "../descriptor/vehicle_desc.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define dr_fopen fopen

repositioning_t repositioning_t::instance;
koord repositioning_t::default_offset = koord(0,4);
char repositioning_t::tab_filename[128] = "";

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

void repositioning_t::read_tabfile(const char* filename) {
  strcpy(tab_filename, filename);
  table.clear();
  
  FILE* file;
  file = dr_fopen(filename, "r");
	if(  file==NULL  ) {
    // there is no tab file.
    return;
  }
  
  char readline[256] = {'\0'};
  while(  fgets(readline, 256, file)!=NULL  ) {
    if(  readline[0]=='#'  ) {
      // comment line. ignore.
      continue;
    }
    const char* const obj_name = strtok(readline, "=");
    const char* const x_str = strtok(NULL, ",");
    const char* const y_str = strtok(NULL, ",");
    if(  obj_name  &&  x_str  &&  y_str  ) {
      koord k = koord(atoi(x_str), atoi(y_str));
      if(  strcmp(obj_name, "default_offset")==0  ) {
        default_offset = k;
      } else {
        // copy name to allocated memory.
        char* o_name = new char[strlen(obj_name)+1];
        strcpy(o_name, obj_name);
        table.set(o_name, k);
      }
    }
  }
  
  fclose(file);
}

void repositioning_t::write_tabfile() {
  if(  get_base_tile_raster_width()!=128  ) {
    return;
  }
  
  FILE* file;
  file = dr_fopen(tab_filename, "w");
  if(  file==NULL  ) {
    // cannot open tab file!
    return;
  }
  
  // write default offset
  fprintf(file, "default_offset=%d,%d\n", default_offset.x, default_offset.y);
  
  // write offset of vehicles
  FOR(stringhashtable_tpl<koord>, const& i, table) {
    fprintf(file, "%s=%d,%d\n", i.key, i.value.x, i.value.y);
  }
  
  fclose(file);
}