#include "api.h"

/** @file api_map_objects.cc exports all map-objects. */

#include "../api_class.h"
#include "../api_function.h"

#include "../../simdings.h"
#include "../../simworld.h"
#include "../../dataobj/scenario.h"
#include "../../dings/baum.h"
#include "../../dings/gebaeude.h"

using namespace script_api;

// use pointers to ding_t_tag[ <type> ] to tag the ding_t classes
static uint8 ding_t_tag[256];

/*
 * template struct to bind ding_t-type to ding_t classes
 */
template<class D> struct bind_code;

/*
 * Class to templatify access of ding_t objects
 */
template<class D> struct access_dings {

	/*
	 * Access object: check whether object is still on this tile.
	 */
	static D* get_by_pos(HSQUIRRELVM vm, SQInteger index)
	{
		SQUserPointer tag = ding_t_tag + bind_code<D>::dingtype;
		SQUserPointer p = NULL;
		if (SQ_SUCCEEDED(sq_getinstanceup(vm, index, &p, tag))  &&  p) {
			D *d = static_cast<D*>(p);
			koord3d pos = param<koord3d>::get(vm, index);
			grund_t *gr = welt->lookup(pos);
			if (gr  &&  gr->obj_ist_da(d)) {
				return d;
			}
			else {
				// object or tile disappeared: clear userpointer
				sq_setinstanceup(vm, index, NULL);
			}
		}
		return NULL;
	}

	/*
	 * Create instance: call constructor with coordinates.
	 */
	static SQInteger push_with_pos(HSQUIRRELVM vm, D* const& d)
	{
		if (d == NULL) {
			sq_pushnull(vm);
			return 1;
		}
		koord pos = d->get_pos().get_2d();
		welt->get_scenario()->koord_w2sq(pos);
		sint16 x = pos.x;
		sint16 y = pos.y;
		sint8  z = d->get_pos().z;
		if (!SQ_SUCCEEDED(push_instance(vm, script_api::param<D*>::squirrel_type(), x, y, z))) {
			return SQ_ERROR;
		}
		sq_setinstanceup(vm, -1, d);
		return 1;
	}

};

SQInteger exp_ding_pos_constructor(HSQUIRRELVM vm)
{
	sint16 x = param<sint16>::get(vm, 2);
	sint16 y = param<sint16>::get(vm, 3);
	sint8  z = param<sint16>::get(vm, 4);
	// set coordinates
	sq_pushstring(vm, "x", -1); param<sint16>::push(vm, x); sq_set(vm, 1);
	sq_pushstring(vm, "y", -1); param<sint16>::push(vm, y); sq_set(vm, 1);
	sq_pushstring(vm, "z", -1); param<sint8 >::push(vm, z); sq_set(vm, 1);
	return SQ_OK;
}

// we have to resolve instances of derived classes here...
SQInteger script_api::param<ding_t*>::push(HSQUIRRELVM vm, ding_t* const& d)
{
	if (d == NULL) {
		sq_pushnull(vm);
		return 1;
	}
	ding_t::typ type = d->get_typ();
	switch(type) {
		case ding_t::baum:
			return script_api::param<baum_t*>::push(vm, (baum_t*)d);

		case ding_t::gebaeude:
			return script_api::param<gebaeude_t*>::push(vm, (gebaeude_t*)d);

		case ding_t::way:
		{
			waytype_t wt = d->get_waytype();
			switch(wt) {
				default: script_api::param<weg_t*>::push(vm, (weg_t*)d);
			}
		}
		default:
			return access_dings<ding_t>::push_with_pos(vm, d);
	}
}

ding_t* script_api::param<ding_t*>::get(HSQUIRRELVM vm, SQInteger index)
{
	return access_dings<ding_t>::get_by_pos(vm, index);
}

template<> struct bind_code<ding_t> { static const uint8 dingtype = ding_t::ding; };

// macro to implement get and push for ding_t's with position
#define getpush_ding_pos(D, type) \
	D* script_api::param<D*>::get(HSQUIRRELVM vm, SQInteger index) \
	{ \
		return access_dings<D>::get_by_pos(vm, index); \
	} \
	SQInteger script_api::param<D*>::push(HSQUIRRELVM vm, D* const& d) \
	{ \
		return access_dings<D>::push_with_pos(vm, d); \
	} \
	template<> struct bind_code<D> { static const uint8 dingtype = type; };

// implementation of get and push by macros
getpush_ding_pos(baum_t, ding_t::baum);
getpush_ding_pos(gebaeude_t, ding_t::gebaeude);
getpush_ding_pos(weg_t, ding_t::way);

// create class
template<class D>
void begin_ding_class(HSQUIRRELVM vm, const char* name, const char* base = NULL)
{
	SQInteger res = create_class(vm, name, base);
	if(  !SQ_SUCCEEDED(res)  ) {
		// todo Better error Message!
		dbg->error( "begin_ding_class()", "create_class failed for %s (Check for outdated scenario.nut)!", name );
	}
	// store typetag to identify pointers
	sq_settypetag(vm, -1, ding_t_tag + bind_code<D>::dingtype);
	// now functions can be registered
}


void export_map_objects(HSQUIRRELVM vm)
{
	/**
	 * Class to access objects on the map
	 * These classes cannot modify anything.
	 */
	begin_class(vm, "map_object_x", "extend_get,coord");
	sq_settypetag(vm, -1, ding_t_tag + bind_code<ding_t>::dingtype);
	/**
	 * @returns owner of the object.
	 */
	register_method(vm, &ding_t::get_besitzer, "get_owner");
	/**
	 * @returns raw name.
	 */
	register_method(vm, &ding_t::get_name, "get_name");
	/**
	 * @returns way type, can be @ref wt_invalid.
	 */
	register_method(vm, &ding_t::get_waytype, "get_waytype");
	/**
	 * @returns position.
	 */
	register_method(vm, &ding_t::get_pos, "get_pos");
	/**
	 * Checks whether player can remove this object.
	 * @returns error message or null if object can be removed.
	 */
	register_method(vm, &ding_t::ist_entfernbar, "is_removable");
	/**
	 * @returns type of object.
	 */
	register_method(vm, &ding_t::get_typ, "get_type");
	end_class(vm);


	/**
	 * Trees on the map.
	 */
	begin_ding_class<baum_t>(vm, "tree_x", "map_object_x");

	register_function(vm, exp_ding_pos_constructor, "constructor", 4, "xiii");
	/**
	 * @returns age of tree in months.
	 */
	register_method(vm, &baum_t::get_age, "get_age");

	end_class(vm);

	/**
	 * Buildings.
	 */
	begin_ding_class<gebaeude_t>(vm, "building_x", "map_object_x");

	register_function(vm, exp_ding_pos_constructor, "constructor", 4, "xiii");

	/**
	 * @returns factory if building belongs to one, otherwise null
	 */
	register_method(vm, &gebaeude_t::get_fabrik, "get_factory");
	/**
	 * @returns city if building belongs to one, otherwise null
	 */
	register_method(vm, &gebaeude_t::get_stadt, "get_city");
	/**
	 * @returns whether building is townhall
	 */
	register_method(vm, &gebaeude_t::ist_rathaus, "is_townhall");
	/**
	 * @returns whether building is headquarter
	 */
	register_method(vm, &gebaeude_t::ist_firmensitz, "is_headquarter");
	/**
	 * @returns whether building is a monument
	 */
	register_method(vm, &gebaeude_t::is_monument, "is_monument");
	/**
	 * Passenger level controls how many passengers will be generated by this building.
	 * @returns passenger level
	 */
	register_method(vm, &gebaeude_t::get_passagier_level, "get_passenger_level");
	/**
	 * Mail level controls how many mail will be generated by this building.
	 * @returns mail level
	 */
	register_method(vm, &gebaeude_t::get_post_level, "get_mail_level");

	end_class(vm);

	/**
	 * Ways.
	 */
	begin_ding_class<weg_t>(vm, "way_x", "map_object_x");

	register_function(vm, exp_ding_pos_constructor, "constructor", 4, "xiii");

	/**
	 * @return if this way has sidewalk - only meaningfull for roads
	 */
	register_method(vm, &weg_t::hat_gehweg, "has_sidewalk");
	/**
	 * @return whether way is electrified by some way-object
	 */
	register_method(vm, &weg_t::is_electrified, "is_electrified");
	/**
	 * @return whether there is a road-sign associated to the way on the tile
	 */
	register_method(vm, &weg_t::has_sign, "has_sign");
	/**
	 * @return whether there is a signal associated to the way on the tile
	 */
	register_method(vm, &weg_t::has_signal, "has_signal");
	/**
	 * @return whether there is a way-object associated to the way on the tile
	 */
	register_method(vm, &weg_t::has_wayobj, "has_wayobj");
	/**
	 * @return whether there is a crossing associated to the way on the tile
	 */
	register_method(vm, &weg_t::is_crossing, "is_crossing");

	end_class(vm);
}
