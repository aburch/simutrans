
#include "../../simskin.h"


#include "text_reader.h"
#include "intlist_reader.h"
#include "image_reader.h"
#include "imagelist_reader.h"
#include "imagelist2d_reader.h"
#include "bridge_reader.h"
#include "tunnel_reader.h"
#include "building_reader.h"
#include "roadsign_reader.h"
#include "citycar_reader.h"
#include "pedestrian_reader.h"
#include "crossing_reader.h"
#include "factory_reader.h"
#include "good_reader.h"
#include "ground_reader.h"
#include "way_reader.h"
#include "root_reader.h"
#include "xref_reader.h"
#include "sound_reader.h"
#include "skin_reader.h"
#include "tree_reader.h"
#include "vehicle_reader.h"


/////////////////////////////////////////////////////////////////////////////
//
//  static data
//
//  Da die Klassen selbstregistrierend sind, kriegt der Linker nicht mit,
//  wenn eine fehlt, solang die instance auch im Klassenfile liegt.
//  Daher lieber alle Instanzen hier versammeln.
/////////////////////////////////////////////////////////////////////////////

inthashtable_tpl<obj_type, obj_reader_t *> *obj_reader_t::obj_reader = NULL;

text_reader_t text_reader_t::the_instance;
intlist_reader_t intlist_reader_t::the_instance;
image_reader_t image_reader_t::the_instance;
imagelist_reader_t imagelist_reader_t::the_instance;
imagelist2d_reader_t imagelist2d_reader_t::the_instance;
root_reader_t root_reader_t::the_instance;
xref_reader_t xref_reader_t::the_instance;

sound_reader_t sound_reader_t::the_instance;

menuskin_reader_t menuskin_reader_t::the_instance;
cursorskin_reader_t cursorskin_reader_t::the_instance;
symbolskin_reader_t symbolskin_reader_t::the_instance;
miscimages_reader_t miscimages_reader_t::the_instance;

good_reader_t good_reader_t::the_instance;

ground_reader_t ground_reader_t::the_instance;
way_reader_t way_reader_t::the_instance;
crossing_reader_t crossing_reader_t::the_instance;

bridge_reader_t bridge_reader_t::the_instance;
tunnel_reader_t tunnel_reader_t::the_instance;

smoke_reader_t smoke_reader_t::the_instance;

building_reader_t building_reader_t::the_instance;
tile_reader_t tile_reader_t::the_instance;
factory_reader_t factory_reader_t::the_instance;
factory_supplier_reader_t factory_supplier_reader_t::the_instance;
factory_product_reader_t factory_product_reader_t::the_instance;
factory_smoke_reader_t factory_smoke_reader_t::the_instance;

vehicle_reader_t vehicle_reader_t::the_instance;

roadsign_reader_t roadsign_reader_t::the_instance;
citycar_reader_t citycar_reader_t::the_instance;
pedestrian_reader_t pedestrian_reader_t::the_instance;

tree_reader_t tree_reader_t::the_instance;
