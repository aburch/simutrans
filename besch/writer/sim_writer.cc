/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *		Statische Instanzen alle writer_t Klassen die für Simutrans benötigt
 *		werden
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
#include "way_writer.h"
#include "way_obj_writer.h"
#include "root_writer.h"
#include "xref_writer.h"
#include "skin_writer.h"
#include "sound_writer.h"
#include "tree_writer.h"
#include "vehicle_writer.h"

/*
 *
 *  static data
 *
 *  Da die Klassen selbstregistrierend sind, kriegt der Linker nicht mit,
 *  wenn eine fehlt, solang die instance auch im Klassenfile liegt.
 *  Daher lieber alle Instanzen hier versammeln.
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

good_writer_t good_writer_t::the_instance;

ground_writer_t ground_writer_t::the_instance;

way_writer_t way_writer_t::the_instance;
way_obj_writer_t way_obj_writer_t::the_instance;
crossing_writer_t crossing_writer_t::the_instance;
bridge_writer_t bridge_writer_t::the_instance;
tunnel_writer_t tunnel_writer_t::the_instance;

field_writer_t field_writer_t::the_instance;
smoke_writer_t smoke_writer_t::the_instance;

building_writer_t building_writer_t::the_instance;
tile_writer_t tile_writer_t::the_instance;
factory_writer_t factory_writer_t::the_instance;
factory_supplier_writer_t factory_supplier_writer_t::the_instance;
factory_product_writer_t factory_product_writer_t::the_instance;
factory_smoke_writer_t factory_smoke_writer_t::the_instance;
factory_field_writer_t factory_field_writer_t::the_instance;

vehicle_writer_t vehicle_writer_t::the_instance;

roadsign_writer_t roadsign_writer_t::the_instance;
citycar_writer_t citycar_writer_t::the_instance;
pedestrian_writer_t pedestrian_writer_t::the_instance;

tree_writer_t tree_writer_t::the_instance;
