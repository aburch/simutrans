/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_pathfinding.cc exports heap structure and construction helpers. */

#include "api_obj_desc_base.h"
#include "api_simple.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../bauer/brueckenbauer.h"
#include "../../bauer/wegbauer.h"
#include "../../descriptor/bridge_desc.h"
#include "../../descriptor/way_desc.h"
#include "../../tpl/binary_heap_tpl.h"
#include "../../simworld.h"

using namespace script_api;


struct heap_node_t {
	sint32 weight;
	sint32 value;

	heap_node_t(sint32 w, sint32 v) : weight(w), value(v) {}
	// dereferencing to be used in binary_heap_tpl
	inline sint32 operator * () const { return weight; }
};

typedef binary_heap_tpl<heap_node_t> simple_heap_t;

namespace script_api{
	declare_specialized_param(heap_node_t, "t|x|y", "simple_heap_x::node_x");
	declare_specialized_param(simple_heap_t*, "t|x|y", "simple_heap_x");
};

SQInteger heap_constructor(HSQUIRRELVM vm)
{
	simple_heap_t* heap = new simple_heap_t();
	attach_instance(vm, 1, heap);
	return SQ_OK;
}

void* script_api::param<simple_heap_t*>::tag()
{
	return (void*)&heap_constructor;
}

simple_heap_t* script_api::param<simple_heap_t*>::get(HSQUIRRELVM vm, SQInteger index)
{
	simple_heap_t *heap = get_attached_instance<simple_heap_t>(vm, index, param<simple_heap_t*>::tag());
	return heap;
}

SQInteger param<heap_node_t>::push(HSQUIRRELVM vm, heap_node_t const& v)
{
	sq_newtable(vm);
	create_slot<sint32>(vm, "weight", v.weight);
	create_slot<sint32>(vm, "value", v.value);
	return 1;
}

void simple_heap_insert(simple_heap_t *heap, sint32 weight, sint32 value)
{
	heap->insert( heap_node_t(weight, value) );
}

SQInteger simple_heap_pop(HSQUIRRELVM vm) // instance
{
	simple_heap_t *heap = param<simple_heap_t*>::get(vm, 1);
	if (heap) {
		if (!heap->empty()) {
			return param<heap_node_t>::push(vm, heap->pop());
		}
		else {
			return sq_raise_error(vm, "Called pop() on empty heap");
		}
	}
	else {
		return sq_raise_error(vm, "Not a heap instance"); // should not happen
	}
}


SQInteger way_builder_constructor(HSQUIRRELVM vm) // instance, player
{
	// get player parameter
	player_t *player = get_my_player(vm);
	if (player == NULL) {
		player = param<player_t*>::get(vm, 2);
	}
	if (player == NULL) {
		return SQ_ERROR;
	}
	way_builder_t* bob = new way_builder_t(player);

	bob->set_keep_existing_faster_ways(true);
	bob->set_keep_city_roads(true);

	attach_instance(vm, 1, bob);
	return SQ_OK;
}

void way_builder_set_build_types(way_builder_t *bob, const way_desc_t *way)
{
	if (way) {
		bob->init_builder( (way_builder_t::bautyp_t)way->get_waytype(), way, NULL /*tunnel*/, NULL /*bridge*/);
	}
}

bool way_builder_is_allowed_step(way_builder_t *bob, grund_t *from, grund_t *to)
{
	if (from == NULL || to == NULL) {
		return false;
	}
	sint32 costs = 0;
	return bob->is_allowed_step(from, to, &costs);
}


koord3d bridge_builder_find_end_pos(player_t *player, koord3d pos, my_ribi_t mribi, const bridge_desc_t *bridge, uint32 min_length)
{
	const char* err;
	sint8 height;
	ribi_t::ribi ribi(mribi);

	if (player == NULL  ||  bridge == NULL) {
		return koord3d::invalid;
	}
	grund_t *from = welt->lookup(pos);
	if (from == NULL  ||  !bridge_builder_t::can_place_ramp(player, from, bridge->get_wtyp(), ribi_t::backward(ribi))) {
		return koord3d::invalid;
	}
	return bridge_builder_t::find_end_pos(player, pos, ribi, bridge, err, height, false, min_length, false);
}


void export_pathfinding(HSQUIRRELVM vm)
{
	/**
	 * Exports a binary heap structure with
	 * with simple nodes.
	 */
	create_class(vm, "simple_heap_x", 0);
	/**
	 * Constructor
	 */
	register_function(vm, heap_constructor, "constructor", 1, "x");
	sq_settypetag(vm, -1, param<simple_heap_t*>::tag());

	/// Clears the heap.
	register_method(vm, &simple_heap_t::clear, "clear");
	/// @returns number of nodes in heap
	register_method(vm, &simple_heap_t::get_count, "len");
	/// @returns true when heap is empty
	register_method(vm, &simple_heap_t::empty, "is_empty");
	/**
	 * Inserts a node into the heap.
	 * @param weight heap is sorted with respect to weight
	 * @param value the data to be stored
	 */
	register_method(vm, simple_heap_insert, "insert", true);
	/**
	 * Pops the top node of the heap.
	 * Raises error if heap is empty.
	 * @returns top node
	 */
	register_function(vm, simple_heap_pop, "pop", 1, "x");
#ifdef SQAPI_DOC
		/**
		 * Helper node class.
		 */
		class node_x {
			int weight; ///< heap is sorted with respect to weight
			int value;  ///< the data to be stored
		}
#endif
	end_class(vm);

	/**
	 * Class with helper methods for way planning.
	 */
	create_class(vm, "way_planner_x", 0);
	/**
	 * Constructor
	 * @param pl player that wants to plan, for ai scripts is set to ai player_x::self
	 * @typemask way_planner_x(player_x)
	 */
	register_function(vm, way_builder_constructor, "constructor", 2, "xx");
	sq_settypetag(vm, -1, param<way_builder_t*>::tag());

	/**
	 * Sets way descriptor, which is needed for @ref is_allowed_step.
	 * @param way descriptor of way to be planned.
	 */
	register_method(vm, way_builder_set_build_types, "set_build_types", true);
	/**
	 * Checks if player can build way from @p from to @p to.
	 * @param from from here
	 * @param to to here, @p from and @p to must be adjacent.
	 */
	register_method(vm, way_builder_is_allowed_step, "is_allowed_step", true);

	end_class(vm);

	/**
	 * Class with helper methods for bridge planning.
	 */
	create_class(vm, "bridge_planner_x", 0);
	/**
	 * Find suitable end tile for bridge starting at @p pos going into direction @p dir.
	 * @param pl who wants to build a bridge
	 * @param pos start tile for bridge
	 * @param dir direction
	 * @param bridge bridge descriptor
	 * @param min_length bridge should have this minimal length
	 * @returns coordinate of end tile or an invalid coordinate
	 */
	STATIC register_method(vm, bridge_builder_find_end_pos, "find_end", false, true);

	end_class(vm);
}


void* script_api::param<way_builder_t*>::tag()
{
	return (void*)&way_builder_constructor;
}

way_builder_t* script_api::param<way_builder_t*>::get(HSQUIRRELVM vm, SQInteger index)
{
	way_builder_t *bob = get_attached_instance<way_builder_t>(vm, index, param<way_builder_t*>::tag());
	return bob;
}

