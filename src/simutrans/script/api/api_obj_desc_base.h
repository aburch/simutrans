/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_API_API_OBJ_DESC_BASE_H
#define SCRIPT_API_API_OBJ_DESC_BASE_H


/** @file api_obj_desc.h templates for transfer of desc-pointers */

#include "../api_param.h"
#include "../api_class.h"

class obj_named_desc_t;
class obj_desc_timelined_t;
class obj_desc_transport_related_t;
class tree_desc_t;
class bridge_desc_t;
class building_desc_t;
class building_tile_desc_t;
class factory_desc_t;
class goods_desc_t;
class roadsign_desc_t;
class tunnel_desc_t;
class vehicle_desc_t;
class way_desc_t;
class way_obj_desc_t;

namespace script_api {

#define declare_desc_param(T, sqtype) \
	template<> \
	struct param<const T*> { \
		\
		typedef const T* (*GETFUNC)(const char*);\
		\
		static GETFUNC getfunc(); \
		\
		declare_types("t|x|y", sqtype); \
		\
		static void* tag() \
		{ \
			return (void*)(getfunc()); \
		} \
		static const T* get(HSQUIRRELVM vm, SQInteger index); \
		\
		static SQInteger push(HSQUIRRELVM vm, const T* b); \
	}; \
	template<> \
	struct param<T*> { \
		\
		declare_types("t|x|y", sqtype); \
	};

#define implement_desc_param(T, sqtype, func) \
	param<const T*>::GETFUNC param<const T*>::getfunc() \
	{ \
		return func; \
	} \
	const T* param<const T*>::get(HSQUIRRELVM vm, SQInteger index) \
	{ \
		return (const T*)get_instanceup(vm, index, tag(), sqtype); \
	} \
	SQInteger param<const T*>::push(HSQUIRRELVM vm, const T* b) \
	{ \
		if (b) { \
			return push_instance_up(vm, b); \
		} \
		else { \
			sq_pushnull(vm); return 1; \
		} \
	}

	declare_specialized_param(const obj_named_desc_t*, "t|x|y", "obj_desc_x");
	declare_param_mask(obj_named_desc_t*, "t|x|y", "obj_desc_x");

	declare_specialized_param(const obj_desc_timelined_t*, "t|x|y", "obj_desc_time_x");
	declare_param_mask(obj_desc_timelined_t*, "t|x|y", "obj_desc_time_x");

	declare_specialized_param(const obj_desc_transport_related_t*, "t|x|y", "obj_desc_transport_x");
	declare_param_mask(obj_desc_transport_related_t*, "t|x|y", "obj_desc_transport_x");

	declare_desc_param(tree_desc_t, "tree_desc_x");
	declare_desc_param(goods_desc_t, "good_desc_x");
	declare_desc_param(building_desc_t, "building_desc_x");
	declare_desc_param(way_desc_t, "way_desc_x");
	declare_desc_param(vehicle_desc_t, "vehicle_desc_x");
	declare_desc_param(tunnel_desc_t, "tunnel_desc_x");
	declare_desc_param(bridge_desc_t, "bridge_desc_x");
	declare_desc_param(roadsign_desc_t, "sign_desc_x");
	declare_desc_param(way_obj_desc_t, "wayobj_desc_x");
	declare_desc_param(factory_desc_t, "factory_desc_x");

	// only push the building_desc_t-pointer
	declare_desc_param(building_tile_desc_t, "building_desc_x");
};

#endif
