/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "text_writer.h"
#include "image_writer.h"
#include "imagelist_writer.h"
#include "imagelist2d_writer.h"
#include "bridge_writer.h"
#include "tunnel_writer.h"
#include "building_writer.h"
#include "roadsign_writer.h"
#include "citycar_writer.h"
#include "pedestrian_writer.h"
#include "crossing_writer.h"
#include "factory_writer.h"
#include "good_writer.h"
#include "ground_writer.h"
#include "groundobj_writer.h"
#include "way_writer.h"
#include "way_obj_writer.h"
#include "root_writer.h"
#include "xref_writer.h"
#include "skin_writer.h"
#include "sound_writer.h"
#include "tree_writer.h"
#include "vehicle_writer.h"


/**
 * static data
 *
 * These classes are self-registering. The linker will not notify if there is a missing class, since it
 * may be also instantiated in the class itself. Therefore, all classes MUST be declared here to force
 * the linking and raise an error if we lack any of them.
 */
stringhashtable_tpl<obj_writer_t*>* obj_writer_t::writer_by_name = NULL;
inthashtable_tpl<obj_type, obj_writer_t*>* obj_writer_t::writer_by_type = NULL;

text_writer_t text_writer_t::the_instance;
image_writer_t image_writer_t::the_instance;
imagelist_writer_t imagelist_writer_t::the_instance;
imagelist2d_writer_t imagelist2d_writer_t::the_instance;
root_writer_t root_writer_t::the_instance;
xref_writer_t xref_writer_t::the_instance;

sound_writer_t sound_writer_t::the_instance;

menuskin_writer_t menuskin_writer_t::the_instance;
cursorskin_writer_t cursorskin_writer_t::the_instance;
symbolskin_writer_t symbolskin_writer_t::the_instance;
miscimages_writer_t miscimages_writer_t::the_instance;

goods_writer_t goods_writer_t::the_instance;

ground_writer_t ground_writer_t::the_instance;

way_writer_t way_writer_t::the_instance;
way_obj_writer_t way_obj_writer_t::the_instance;
crossing_writer_t crossing_writer_t::the_instance;
bridge_writer_t bridge_writer_t::the_instance;
tunnel_writer_t tunnel_writer_t::the_instance;

field_writer_t field_writer_t::the_instance;
groundobj_writer_t groundobj_writer_t::the_instance;
smoke_writer_t smoke_writer_t::the_instance;

building_writer_t building_writer_t::the_instance;
tile_writer_t tile_writer_t::the_instance;
factory_writer_t factory_writer_t::the_instance;
factory_supplier_writer_t factory_supplier_writer_t::the_instance;
factory_product_writer_t factory_product_writer_t::the_instance;
factory_smoke_writer_t factory_smoke_writer_t::the_instance;
factory_field_group_writer_t factory_field_group_writer_t::the_instance;
factory_field_class_writer_t factory_field_class_writer_t::the_instance;

vehicle_writer_t vehicle_writer_t::the_instance;

roadsign_writer_t roadsign_writer_t::the_instance;
citycar_writer_t citycar_writer_t::the_instance;
pedestrian_writer_t pedestrian_writer_t::the_instance;

tree_writer_t tree_writer_t::the_instance;
