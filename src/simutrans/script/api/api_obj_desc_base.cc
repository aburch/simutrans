/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api_obj_desc_base.h"

#include "../../builder/brueckenbauer.h"
#include "../../builder/fabrikbauer.h"
#include "../../builder/hausbauer.h"
#include "../../builder/tunnelbauer.h"
#include "../../builder/vehikelbauer.h"
#include "../../builder/goods_manager.h"
#include "../../builder/wegbauer.h"
#include "../../descriptor/bridge_desc.h"
#include "../../descriptor/tunnel_desc.h"
#include "../../descriptor/vehicle_desc.h"
#include "../../descriptor/goods_desc.h"
#include "../../descriptor/way_desc.h"
#include "../../descriptor/way_obj_desc.h"
#include "../../obj/baum.h"
#include "../../obj/roadsign.h"
#include "../../obj/wayobj.h"
#include "../../../squirrel/sq_extensions.h"

using namespace script_api;

static const way_desc_t *my_get_desc(const char *name)
{
	return way_builder_t::get_desc(name);
}

implement_desc_param(tree_desc_t, "tree_desc_x", &tree_builder_t::find_tree);
implement_desc_param(building_desc_t, "building_desc_x", &hausbauer_t::get_desc);
implement_desc_param(goods_desc_t, "good_desc_x", (const goods_desc_t* (*)(const char*))(&goods_manager_t::get_info) );
implement_desc_param(way_desc_t, "way_desc_x", &my_get_desc);
implement_desc_param(vehicle_desc_t, "vehicle_desc_x", &vehicle_builder_t::get_info);
implement_desc_param(tunnel_desc_t, "tunnel_desc_x", &tunnel_builder_t::get_desc);
implement_desc_param(bridge_desc_t, "bridge_desc_x", &bridge_builder_t::get_desc);
implement_desc_param(roadsign_desc_t, "sign_desc_x", &roadsign_t::find_desc);
implement_desc_param(way_obj_desc_t, "wayobj_desc_x", &wayobj_t::find_desc);
implement_desc_param(factory_desc_t, "factory_desc_x", &factory_builder_t::get_desc);

/**
 * Macro to get the implementation of get method based on unique tag.
 */
#define implement_class_with_tag(class) \
	/* tag: points to itself */ \
	static void *class ## _tag = &class ## _tag; \
	\
	const class* param<const class*>::get(HSQUIRRELVM vm, SQInteger index) \
	{ \
		return (const class*)get_instanceup(vm, index, tag(), squirrel_type()); \
	} \
	\
	void* param<const class*>::tag() \
	{ \
		return class ## _tag; \
	}

// use the macro to obtain the interface of some abstract classes
implement_class_with_tag(obj_named_desc_t);
implement_class_with_tag(obj_desc_timelined_t);
implement_class_with_tag(obj_desc_transport_related_t);

SQInteger param<const building_tile_desc_t*>::push(HSQUIRRELVM vm, const building_tile_desc_t* b)
{
	return param<const building_desc_t*>::push(vm, b ? b->get_desc() : NULL);
}
