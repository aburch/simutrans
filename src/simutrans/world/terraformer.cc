/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "terraformer.h"

#include "../dataobj/scenario.h"
#include "../descriptor/ground_desc.h"
#include "../tool/simmenu.h"
#include "simworld.h"


terraformer_t::node_t::node_t() :
	x(-1),
	y(-1),
	changed(0)
{
}


terraformer_t::node_t::node_t(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw, uint8 c) :
	x(x),
	y(y),
	hsw(hsw),
	hse(hse),
	hne(hne),
	hnw(hnw),
	changed(c)
{
}


bool terraformer_t::node_t::operator==(const terraformer_t::node_t &a) const
{
	return (a.x==x) && (a.y==y);
}


terraformer_t::terraformer_t(operation_t op, karte_t *welt) :
	actual_flag(1),
	ready(false),
	op(op),
	welt(welt)
{
}


bool terraformer_t::node_t::comp(const terraformer_t::node_t &a, const terraformer_t::node_t &b)
{
	int diff = a.x - b.x;
	if (diff == 0) {
		diff = a.y - b.y;
	}
	return diff<0;
}


void terraformer_t::add_node(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	if (!welt->is_within_limits(x,y)) {
		return;
	}

	const node_t test(x, y, hsw, hse, hne, hnw, actual_flag^3);
	node_t *other = list.insert_unique_ordered(test, node_t::comp);

	const sint8 factor = (op == terraformer_t::raise) ? +1 : -1;

	if (other) {
		if (factor*other->hsw < factor*test.hsw) {
			other->hsw = test.hsw;
			other->changed |= actual_flag ^ 3;
			ready = false;
		}

		if (factor*other->hse < factor*test.hse) {
			other->hse = test.hse;
			other->changed |= actual_flag ^ 3;
			ready = false;
		}

		if (factor*other->hne < factor*test.hne) {
			other->hne = test.hne;
			other->changed |= actual_flag ^ 3;
			ready = false;
		}

		if (factor*other->hnw < factor*test.hnw) {
			other->hnw = test.hnw;
			other->changed |= actual_flag ^ 3;
			ready = false;
		}
	}
	else {
		ready = false;
	}
}


void terraformer_t::generate_affected_tile_list()
{
	while( !ready) {
		actual_flag ^= 3; // flip bits
		// clear new_flag bit
		for(node_t& i : list) {
			i.changed &= actual_flag;
		}

		// process nodes with actual_flag set
		ready = true;
		for(uint32 j=0; j < list.get_count(); j++) {
			node_t& i = list[j];
			if (i.changed & actual_flag) {
				i.changed &= ~actual_flag;
				if (op == terraformer_t::raise) {
					prepare_raise(i);
				}
				else {
					prepare_lower(i);
				}
			}
		}
	}
}


const char *terraformer_t::can_raise_all(const player_t *player, bool keep_water) const
{
	assert(op == terraformer_t::raise);
	assert(ready);

	for(node_t const& i : list) {
		if (const char *err = can_raise_tile_to(i, player, keep_water)) {
			return err;
		}
	}
	return NULL;
}


const char *terraformer_t::can_lower_all(const player_t *player) const
{
	assert(op == terraformer_t::lower);
	assert(ready);

	for(node_t const& i : list) {
		if (const char *err = can_lower_tile_to(i, player)) {
			return err;
		}
	}

	return NULL;
}


int terraformer_t::apply()
{
	assert(ready);
	int n = 0;

	if (op == terraformer_t::raise) {
		for(node_t const& i : list) {
			n += raise_to(i);
		}
	}
	else {
		for(node_t const& i : list) {
			n += lower_to(i);
		}
	}

	return n;
}


void terraformer_t::prepare_raise(const node_t node)
{
	assert(welt->is_within_limits(node.x,node.y));

	const grund_t *gr = welt->lookup_kartenboden_nocheck(node.x,node.y);
	const sint8 water_hgt = welt->get_water_hgt_nocheck(node.x,node.y);
	const sint8 h0 = gr->get_hoehe();

	// old height
	const sint8 h0_sw = gr->is_water() ? min(water_hgt, welt->lookup_hgt_nocheck(node.x,  node.y+1) ) : h0 + corner_sw( gr->get_grund_hang() );
	const sint8 h0_se = gr->is_water() ? min(water_hgt, welt->lookup_hgt_nocheck(node.x+1,node.y+1) ) : h0 + corner_se( gr->get_grund_hang() );
	const sint8 h0_ne = gr->is_water() ? min(water_hgt, welt->lookup_hgt_nocheck(node.x+1,node.y  ) ) : h0 + corner_ne( gr->get_grund_hang() );
	const sint8 h0_nw = gr->is_water() ? min(water_hgt, welt->lookup_hgt_nocheck(node.x,  node.y  ) ) : h0 + corner_nw( gr->get_grund_hang() );

	// new height
	const sint8 hn_sw = max(node.hsw, h0_sw);
	const sint8 hn_se = max(node.hse, h0_se);
	const sint8 hn_ne = max(node.hne, h0_ne);
	const sint8 hn_nw = max(node.hnw, h0_nw);

	// nothing to do?
	if(  !gr->is_water()  &&  h0_sw >= node.hsw  &&  h0_se >= node.hse  &&  h0_ne >= node.hne  &&  h0_nw >= node.hnw  ) {
		return;
	}

	// calc new height and slope
	const sint8 hneu    = min( min( hn_sw, hn_se ), min( hn_ne, hn_nw ) );
	const sint8 hmaxneu = max( max( hn_sw, hn_se ), max( hn_ne, hn_nw ) );

	const uint8 max_hdiff = ground_desc_t::double_grounds ? 2 : 1;

	const bool ok = (hmaxneu - hneu <= max_hdiff); // may fail on water tiles since lookup_hgt might be modified from previous raise_to calls
	if(  !ok  &&  !gr->is_water()  ) {
		assert(false);
	}

	// sw
	if (h0_sw < node.hsw) {
		add_node( node.x - 1, node.y + 1, node.hsw - max_hdiff, node.hsw - max_hdiff, node.hsw, node.hsw - max_hdiff );
	}
	// s
	if (h0_sw < node.hsw  ||  h0_se < node.hse) {
		const sint8 hs = max( node.hse, node.hsw ) - max_hdiff;
		add_node( node.x, node.y + 1, hs, hs, node.hse, node.hsw );
	}
	// se
	if (h0_se < node.hse) {
		add_node( node.x + 1, node.y + 1, node.hse - max_hdiff, node.hse - max_hdiff, node.hse - max_hdiff, node.hse );
	}
	// e
	if (h0_se < node.hse  ||  h0_ne < node.hne) {
		const sint8 he = max( node.hse, node.hne ) - max_hdiff;
		add_node( node.x + 1, node.y, node.hse, he, he, node.hne );
	}
	// ne
	if (h0_ne < node.hne) {
		add_node( node.x + 1,node.y - 1, node.hne, node.hne - max_hdiff, node.hne - max_hdiff, node.hne - max_hdiff );
	}
	// n
	if (h0_nw < node.hnw  ||  h0_ne < node.hne) {
		const sint8 hn = max( node.hnw, node.hne ) - max_hdiff;
		add_node( node.x, node.y - 1, node.hnw, node.hne, hn, hn );
	}
	// nw
	if (h0_nw < node.hnw) {
		add_node( node.x - 1, node.y - 1, node.hnw - max_hdiff, node.hnw, node.hnw - max_hdiff, node.hnw - max_hdiff );
	}
	// w
	if (h0_sw < node.hsw  ||  h0_nw < node.hnw) {
		const sint8 hw = max( node.hnw, node.hsw ) - max_hdiff;
		add_node( node.x - 1, node.y, hw, node.hsw, node.hnw, hw );
	}
}


void terraformer_t::prepare_lower(const node_t node)
{
	assert(welt->is_within_limits(node.x,node.y));
	const grund_t *gr = welt->lookup_kartenboden_nocheck(node.x,node.y);
	const sint8 water_hgt = welt->get_water_hgt_nocheck(node.x,node.y);

	const sint8 h0 = gr->get_hoehe();

	// which corners have to be raised?
	const sint8 h0_sw = gr->is_water() ? min( water_hgt, welt->lookup_hgt_nocheck(node.x,  node.y+1) ) : h0 + corner_sw( gr->get_grund_hang() );
	const sint8 h0_se = gr->is_water() ? min( water_hgt, welt->lookup_hgt_nocheck(node.x+1,node.y+1) ) : h0 + corner_se( gr->get_grund_hang() );
	const sint8 h0_ne = gr->is_water() ? min( water_hgt, welt->lookup_hgt_nocheck(node.x,  node.y+1) ) : h0 + corner_ne( gr->get_grund_hang() );
	const sint8 h0_nw = gr->is_water() ? min( water_hgt, welt->lookup_hgt_nocheck(node.x,  node.y  ) ) : h0 + corner_nw( gr->get_grund_hang() );

	const uint8 max_hdiff = ground_desc_t::double_grounds ? 2 : 1;

	// sw
	if (h0_sw > node.hsw) {
		add_node(node.x - 1, node.y + 1, node.hsw + max_hdiff, node.hsw + max_hdiff, node.hsw, node.hsw + max_hdiff);
	}
	// s
	if (h0_se > node.hse || h0_sw > node.hsw) {
		const sint8 hs = min( node.hse, node.hsw ) + max_hdiff;
		add_node( node.x, node.y + 1, hs, hs, node.hse, node.hsw);
	}
	// se
	if (h0_se > node.hse) {
		add_node( node.x + 1, node.y + 1, node.hse + max_hdiff, node.hse + max_hdiff, node.hse + max_hdiff, node.hse);
	}
	// e
	if (h0_se > node.hse || h0_ne > node.hne) {
		const sint8 he = max( node.hse, node.hne ) + max_hdiff;
		add_node( node.x + 1, node.y, node.hse, he, he, node.hne);
	}
	// ne
	if (h0_ne > node.hne) {
		add_node( node.x + 1, node.y - 1, node.hne, node.hne + max_hdiff, node.hne + max_hdiff, node.hne + max_hdiff);
	}
	// n
	if (h0_nw > node.hnw  ||  h0_ne > node.hne) {
		const sint8 hn = min( node.hnw, node.hne ) + max_hdiff;
		add_node( node.x, node.y - 1, node.hnw, node.hne, hn, hn);
	}
	// nw
	if (h0_nw > node.hnw) {
		add_node( node.x - 1, node.y - 1, node.hnw + max_hdiff, node.hnw, node.hnw + max_hdiff, node.hnw + max_hdiff);
	}
	// w
	if (h0_nw > node.hnw || h0_sw > node.hsw) {
		const sint8 hw = min( node.hnw, node.hsw ) + max_hdiff;
		add_node( node.x - 1, node.y, hw, node.hsw, node.hnw, hw);
	}
}


const char *terraformer_t::can_raise_tile_to(const node_t &node, const player_t *player, bool keep_water) const
{
	assert(welt->is_within_limits(node.x,node.y));

	const grund_t *gr = welt->lookup_kartenboden_nocheck(node.x,node.y);
	const sint8 water_hgt = welt->get_water_hgt_nocheck(node.x,node.y);

	const sint8 max_hgt = max(max(node.hsw,node.hse),max(node.hne,node.hnw));

	if(  gr->is_water()  &&  keep_water  &&  max_hgt > water_hgt  ) {
		return "";
	}

	return can_raise_plan_to(player, node.x, node.y, max_hgt);
}


// lower plan
// new heights for each corner given
const char* terraformer_t::can_lower_tile_to(const node_t &node, const player_t *player) const
{
	assert(welt->is_within_limits(node.x,node.y));

	const sint8 hneu = min( min( node.hsw, node.hse ), min( node.hne, node.hnw ) );

	if( hneu < welt->get_min_allowed_height() ) {
		return "Maximum tile height difference reached.";
	}

	// water heights
	// check if need to lower water height for higher neighbouring tiles
	for(  sint16 i = 0 ;  i < 8 ;  i++  ) {
		const koord neighbour = koord( node.x, node.y ) + koord::neighbours[i];
		if(  welt->is_within_limits(neighbour)  &&  welt->get_water_hgt_nocheck(neighbour) > hneu  ) {
			if (!welt->is_plan_height_changeable( neighbour.x, neighbour.y )) {
				return "";
			}
		}
	}

	return can_lower_plan_to(player, node.x, node.y, hneu );
}


int terraformer_t::raise_to(const node_t &node)
{
	assert(welt->is_within_limits(node.x,node.y));

	int n = 0;
	grund_t *gr = welt->lookup_kartenboden_nocheck(node.x,node.y);
	const sint8 water_hgt = welt->get_water_hgt_nocheck(node.x,node.y);
	const sint8 h0 = gr->get_hoehe();

	// old height
	const sint8 h0_sw = gr->is_water() ? min(water_hgt, welt->lookup_hgt_nocheck(node.x,   node.y+1) ) : h0 + corner_sw( gr->get_grund_hang() );
	const sint8 h0_se = gr->is_water() ? min(water_hgt, welt->lookup_hgt_nocheck(node.x+1, node.y+1) ) : h0 + corner_se( gr->get_grund_hang() );
	const sint8 h0_ne = gr->is_water() ? min(water_hgt, welt->lookup_hgt_nocheck(node.x+1, node.y  ) ) : h0 + corner_ne( gr->get_grund_hang() );
	const sint8 h0_nw = gr->is_water() ? min(water_hgt, welt->lookup_hgt_nocheck(node.x,   node.y  ) ) : h0 + corner_nw( gr->get_grund_hang() );

	// new height
	const sint8 hn_sw = max(node.hsw, h0_sw);
	const sint8 hn_se = max(node.hse, h0_se);
	const sint8 hn_ne = max(node.hne, h0_ne);
	const sint8 hn_nw = max(node.hnw, h0_nw);

	// nothing to do?
	if(  !gr->is_water()  &&  h0_sw >= node.hsw  &&  h0_se >= node.hse  &&  h0_ne >= node.hne  &&  h0_nw >= node.hnw  ) {
		return 0;
	}

	// calc new height and slope
	const sint8 hneu    = min( min( hn_sw, hn_se ), min( hn_ne, hn_nw ) );
	const sint8 hmaxneu = max( max( hn_sw, hn_se ), max( hn_ne, hn_nw ) );

	const uint8 max_hdiff  = ground_desc_t::double_grounds ? 2 : 1;
	const sint8 disp_hneu  = max( hneu,  water_hgt );
	const sint8 disp_hn_sw = max( hn_sw, water_hgt );
	const sint8 disp_hn_se = max( hn_se, water_hgt );
	const sint8 disp_hn_ne = max( hn_ne, water_hgt );
	const sint8 disp_hn_nw = max( hn_nw, water_hgt );
	const slope_t::type sneu = encode_corners(disp_hn_sw - disp_hneu, disp_hn_se - disp_hneu, disp_hn_ne - disp_hneu, disp_hn_nw - disp_hneu);

	const bool ok = (hmaxneu - hneu <= max_hdiff); // may fail on water tiles since lookup_hgt might be modified from previous raise_to calls
	if (!ok && !gr->is_water()) {
		assert(false);
	}

	// change height and slope, for water tiles only if they will become land
	if(  !gr->is_water()  ||  (hmaxneu > water_hgt  ||  (hneu == water_hgt  &&  hmaxneu == water_hgt)  )  ) {
		gr->set_pos( koord3d( node.x, node.y, disp_hneu ) );
		gr->set_grund_hang( sneu );
		welt->access_nocheck(node.x,node.y)->angehoben();
		welt->set_water_hgt_nocheck(node.x, node.y, welt->get_groundwater()-4);
	}

	// update north point in grid
	welt->set_grid_hgt_nocheck(node.x, node.y, hn_nw);
	welt->calc_climate(koord(node.x,node.y), true);

	if ( node.x+1 == welt->get_size().x ) {
		// update eastern grid coordinates too if we are in the edge.
		welt->set_grid_hgt_nocheck(node.x+1, node.y, hn_ne);
		welt->set_grid_hgt_nocheck(node.x+1, node.y+1, hn_se);
	}

	if ( node.y+1 == welt->get_size().y ) {
		// update southern grid coordinates too if we are in the edge.
		welt->set_grid_hgt_nocheck(node.x,   node.y+1, hn_sw);
		welt->set_grid_hgt_nocheck(node.x+1, node.y+1, hn_se);
	}

	n += hn_sw - h0_sw + hn_se - h0_se + hn_ne - h0_ne  + hn_nw - h0_nw;

	welt->lookup_kartenboden_nocheck(node.x,node.y)->calc_image();

	if ( (node.x+2) < welt->get_size().x ) {
		welt->lookup_kartenboden_nocheck(node.x+1,node.y)->calc_image();
	}

	if ( (node.y+2) < welt->get_size().y ) {
		welt->lookup_kartenboden_nocheck(node.x,node.y+1)->calc_image();
	}

	return n;
}


int terraformer_t::lower_to(const node_t &node)
{
	assert(welt->is_within_limits(node.x,node.y));

	int n = 0;
	grund_t *gr = welt->lookup_kartenboden_nocheck(node.x,node.y);
	sint8 water_hgt = welt->get_water_hgt_nocheck(node.x,node.y);
	const sint8 h0 = gr->get_hoehe();

	// old height
	const sint8 h0_sw = gr->is_water() ? min( water_hgt, welt->lookup_hgt_nocheck(node.x,   node.y+1) ) : h0 + corner_sw( gr->get_grund_hang() );
	const sint8 h0_se = gr->is_water() ? min( water_hgt, welt->lookup_hgt_nocheck(node.x+1, node.y+1) ) : h0 + corner_se( gr->get_grund_hang() );
	const sint8 h0_ne = gr->is_water() ? min( water_hgt, welt->lookup_hgt_nocheck(node.x+1, node.y  ) ) : h0 + corner_ne( gr->get_grund_hang() );
	const sint8 h0_nw = gr->is_water() ? min( water_hgt, welt->lookup_hgt_nocheck(node.x,   node.y  ) ) : h0 + corner_nw( gr->get_grund_hang() );

	// new height
	const sint8 hn_sw = min(node.hsw, h0_sw);
	const sint8 hn_se = min(node.hse, h0_se);
	const sint8 hn_ne = min(node.hne, h0_ne);
	const sint8 hn_nw = min(node.hnw, h0_nw);

	// nothing to do?
	if(  gr->is_water()  ) {
		if(  h0_nw <= node.hnw  ) {
			return 0;
		}
	}
	else if(  h0_sw <= node.hsw  &&  h0_se <= node.hse  &&  h0_ne <= node.hne  &&  h0_nw <= node.hnw  ) {
		return 0;
	}

	// calc new height and slope
	const sint8 hneu    = min( min( hn_sw, hn_se ), min( hn_ne, hn_nw ) );
	const sint8 hmaxneu = max( max( hn_sw, hn_se ), max( hn_ne, hn_nw ) );

	if(  hneu >= water_hgt  ) {
		// calculate water table from surrounding tiles - start off with height on this tile
		sint8 water_table = water_hgt >= h0 ? water_hgt : welt->get_groundwater() - 4;

		/*
		 * we test each corner in turn to see whether it is at the base height of the tile.
		 * If it is we then mark the 3 surrounding tiles for that corner for checking.
		 * Surrounding tiles are indicated by bits going anti-clockwise from
		 * (binary) 00000001 for north-west through to (binary) 10000000 for north
		 * as this is the order of directions used by koord::neighbours[]
		 */

		uint8 neighbour_flags = 0;

		if(  hn_nw == hneu  ) {
			neighbour_flags |= 0x83;
		}
		if(  hn_ne == hneu  ) {
			neighbour_flags |= 0xe0;
		}
		if(  hn_se == hneu  ) {
			neighbour_flags |= 0x38;
		}
		if(  hn_sw == hneu  ) {
			neighbour_flags |= 0x0e;
		}

		for(  sint16 i = 0;  i < 8 ;  i++  ) {
			const koord neighbour = koord( node.x, node.y ) + koord::neighbours[i];

			// here we look at the bit in neighbour_flags for this direction
			// we shift it i bits to the right and test the least significant bit

			if(  welt->is_within_limits( neighbour )  &&  ((neighbour_flags >> i) & 1)  ) {
				grund_t *gr2 = welt->lookup_kartenboden_nocheck( neighbour );
				const sint8 water_hgt_neighbour = welt->get_water_hgt_nocheck( neighbour );
				if(  gr2  &&  (water_hgt_neighbour >= gr2->get_hoehe())  &&  water_hgt_neighbour <= hneu  ) {
					water_table = max( water_table, water_hgt_neighbour );
				}
			}
		}

		for(  sint16 i = 0;  i < 8 ;  i++  ) {
			const koord neighbour = koord( node.x, node.y ) + koord::neighbours[i];
			if(  welt->is_within_limits( neighbour )  ) {
				grund_t *gr2 = welt->lookup_kartenboden_nocheck( neighbour );
				if(  gr2  &&  gr2->get_hoehe() < water_table  ) {
					i = 8;
					water_table = welt->get_groundwater() - 4;
				}
			}
		}

		// only allow water table to be lowered (except for case of sea level)
		// this prevents severe (errors!
		if(  water_table < welt->get_water_hgt_nocheck(node.x,node.y)  ) {
			water_hgt = water_table;
			welt->set_water_hgt_nocheck(node.x, node.y, water_table );
		}
	}

	// calc new height and slope
	const sint8 disp_hneu  = max( hneu,  water_hgt );
	const sint8 disp_hn_sw = max( hn_sw, water_hgt );
	const sint8 disp_hn_se = max( hn_se, water_hgt );
	const sint8 disp_hn_ne = max( hn_ne, water_hgt );
	const sint8 disp_hn_nw = max( hn_nw, water_hgt );
	const slope_t::type sneu = encode_corners(disp_hn_sw - disp_hneu, disp_hn_se - disp_hneu, disp_hn_ne - disp_hneu, disp_hn_nw - disp_hneu);

	// change height and slope for land tiles only
	if(  !gr->is_water()  ||  (hmaxneu > water_hgt)  ) {
		gr->set_pos( koord3d( node.x, node.y, disp_hneu ) );
		gr->set_grund_hang( (slope_t::type)sneu );
		welt->access_nocheck(node.x,node.y)->abgesenkt();
	}

	// update north point in grid
	welt->set_grid_hgt_nocheck(node.x, node.y, hn_nw);
	if ( node.x+1 == welt->get_size().x ) {
		// update eastern grid coordinates too if we are in the edge.
		welt->set_grid_hgt_nocheck(node.x+1, node.y,   hn_ne);
		welt->set_grid_hgt_nocheck(node.x+1, node.y+1, hn_se);
	}

	if ( node.y+1 == welt->get_size().y ) {
		// update southern grid coordinates too if we are in the edge.
		welt->set_grid_hgt_nocheck(node.x,   node.y+1, hn_sw);
		welt->set_grid_hgt_nocheck(node.x+1, node.y+1, hn_se);
	}

	// water heights
	// lower water height for higher neighbouring tiles
	// find out how high water is
	for(  sint16 i = 0;  i < 8;  i++  ) {
		const koord neighbour = koord( node.x, node.y ) + koord::neighbours[i];
		if(  welt->is_within_limits( neighbour )  ) {
			const sint8 water_hgt_neighbour = welt->get_water_hgt_nocheck( neighbour );
			if(water_hgt_neighbour > hneu  ) {
				if(  welt->min_hgt_nocheck( neighbour ) < water_hgt_neighbour  ) {
					// convert to flat ground before lowering water level
					welt->raise_grid_to( neighbour.x,     neighbour.y,     water_hgt_neighbour );
					welt->raise_grid_to( neighbour.x + 1, neighbour.y,     water_hgt_neighbour );
					welt->raise_grid_to( neighbour.x,     neighbour.y + 1, water_hgt_neighbour );
					welt->raise_grid_to( neighbour.x + 1, neighbour.y + 1, water_hgt_neighbour );
				}

				welt->set_water_hgt_nocheck( neighbour, hneu );
				welt->access_nocheck(neighbour)->correct_water();
			}
		}
	}

	welt->calc_climate( koord( node.x, node.y ), false );
	for(  sint16 i = 0;  i < 8;  i++  ) {
		const koord neighbour = koord( node.x, node.y ) + koord::neighbours[i];
		welt->calc_climate( neighbour, false );
	}

	// recalc landscape images - need to extend 2 in each direction
	for(  sint16 j = node.y - 2;  j <= node.y + 2;  j++  ) {
		for(  sint16 i = node.x - 2;  i <= node.x + 2;  i++  ) {
			if(  welt->is_within_limits( i, j )  /*&&  (i != x  ||  j != y)*/  ) {
				welt->recalc_transitions( koord (i, j ) );
			}
		}
	}

	n += h0_sw-hn_sw + h0_se-hn_se + h0_ne-hn_ne + h0_nw-hn_nw;

	welt->lookup_kartenboden_nocheck(node.x,node.y)->calc_image();
	if( (node.x+2) < welt->get_size().x ) {
		welt->lookup_kartenboden_nocheck(node.x+1,node.y)->calc_image();
	}

	if( (node.y+2) < welt->get_size().y ) {
		welt->lookup_kartenboden_nocheck(node.x,node.y+1)->calc_image();
	}

	return n;
}


const char *terraformer_t::can_lower_plan_to(const player_t *player, sint16 x, sint16 y, sint8 h) const
{
	const planquadrat_t *plan = welt->access(x,y);
	const settings_t &settings = welt->get_settings();

	if(  plan==NULL  ) {
		return "";
	}

	if(  h < welt->get_groundwater() - 3  ) {
		return "";
	}

	const sint8 hmax = plan->get_kartenboden()->get_hoehe();
	if(  (hmax == h  ||  hmax == h - 1)  &&  (plan->get_kartenboden()->get_grund_hang() == 0  ||  welt->is_plan_height_changeable( x, y ))  ) {
		return NULL;
	}

	if(  !welt->is_plan_height_changeable(x, y)  ) {
		return "";
	}

	// tunnel slope below?
	const grund_t *gr = plan->get_boden_in_hoehe( h - 1 );
	if(  !gr  ) {
		gr = plan->get_boden_in_hoehe( h - 2 );
	}

	if(  !gr  && settings.get_way_height_clearance()==2  ) {
		gr = plan->get_boden_in_hoehe( h - 3 );
	}

	if(  gr  &&  h < gr->get_pos().z + slope_t::max_diff( gr->get_weg_hang() ) + settings.get_way_height_clearance()  ) {
		return "";
	}

	// tunnel below?
	while(h < hmax) {
		if(plan->get_boden_in_hoehe(h)) {
			return "";
		}
		h ++;
	}

	// check allowance by scenario
	if (welt->get_scenario()->is_scripted()) {
		return welt->get_scenario()->is_work_allowed_here(player, TOOL_LOWER_LAND|GENERAL_TOOL, ignore_wt, plan->get_kartenboden()->get_pos());
	}

	return NULL;
}


const char *terraformer_t::can_raise_plan_to(const player_t *player, sint16 x, sint16 y, sint8 h) const
{
	const planquadrat_t *plan = welt->access(x,y);
	if(  plan == NULL  ||  !welt->is_plan_height_changeable(x, y)  ) {
		return "";
	}

	// irgendwo eine Bruecke im Weg?
	const sint8 hmin = plan->get_kartenboden()->get_hoehe();
	while(h > hmin) {
		if(plan->get_boden_in_hoehe(h)) {
			return "";
		}
		h --;
	}

	// check allowance by scenario
	if (welt->get_scenario()->is_scripted()) {
		return welt->get_scenario()->is_work_allowed_here(player, TOOL_RAISE_LAND|GENERAL_TOOL, ignore_wt, plan->get_kartenboden()->get_pos());
	}

	return NULL;
}
