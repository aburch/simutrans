/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/** @file api_good.cc exports goods packet (ware_t) as good_x. */

#include "api_obj_desc_base.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simware.h"
#include "../../descriptor/goods_desc.h"
#include "../../halthandle_t.h"
#include "../../tpl/vector_tpl.h"

using namespace script_api;


static void* ware_t_tag = &ware_t_tag;


void* script_api::param<ware_t*>::tag()
{
	return ware_t_tag;
}


ware_t* script_api::param<ware_t*>::get(HSQUIRRELVM vm, SQInteger index)
{
	return get_attached_instance<ware_t>(vm, index, param<ware_t*>::tag());
}


SQInteger script_api::param<ware_t*>::push(HSQUIRRELVM vm, ware_t* const& v)
{
	if (!v) { sq_pushnull(vm); return 1; }
	return param<ware_t>::push(vm, *v);
}


void* script_api::param<ware_t>::tag()
{
	return ware_t_tag;
}


ware_t script_api::param<ware_t>::get(HSQUIRRELVM vm, SQInteger index)
{
	ware_t* w = get_attached_instance<ware_t>(vm, index, param<ware_t*>::tag());
	return w ? *w : ware_t();
}


SQInteger script_api::param<ware_t>::push(HSQUIRRELVM vm, ware_t const& v)
{
	if (SQ_FAILED(push_class(vm, "good_x"))) {
		return SQ_ERROR;
	}
	if (SQ_FAILED(sq_createinstance(vm, -1))) {
		sq_pop(vm, 1);
		return SQ_ERROR;
	}
	sq_remove(vm, -2);
	attach_instance(vm, -1, new ware_t(v));
	return 1;
}


static halthandle_t good_get_destination(ware_t* ware)
{
	return ware->get_ziel();
}


static const vector_tpl<halthandle_t>& good_get_transit_halts(ware_t* ware)
{
	return ware->get_transit_halts();
}


static koord good_get_destination_pos(ware_t* ware)
{
	return ware->get_zielpos();
}


static const goods_desc_t* good_get_desc(ware_t* ware)
{
	return ware->get_desc();
}


void export_good(HSQUIRRELVM vm)
{
	/**
	 * Represents a goods packet being transported by a vehicle.
	 * Obtained via @ref convoy_x::get_cargo.
	 */
	create_class(vm, "good_x");
	sq_settypetag(vm, -1, param<ware_t*>::tag());

	/**
	 * Destination halt of this goods packet.
	 * @returns halt_x or null if no destination halt
	 */
	register_method(vm, &good_get_destination, "get_destination", true);

	/**
	 * Transit halts for this goods packet (including the final destination halt).
	 * @returns array of halt_x
	 */
	register_method(vm, &good_get_transit_halts, "get_transit_halts", true);

	/**
	 * Destination position of this goods packet (target tile coordinate).
	 * @returns coord
	 */
	register_method(vm, &good_get_destination_pos, "get_destination_pos", true);

	/**
	 * Goods type descriptor of this goods packet.
	 * @returns good_desc_x
	 */
	register_method(vm, &good_get_desc, "get_desc", true);

	end_class(vm);
}
