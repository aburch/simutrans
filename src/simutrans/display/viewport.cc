/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "viewport.h"

#include "simgraph.h"
#include "../world/simworld.h"
#include "../dataobj/environment.h"
#include "../dataobj/koord3d.h"
#include "../ground/grund.h"
#include "../obj/zeiger.h"


void viewport_t::set_viewport_ij_offset( const koord &k )
{
	view_ij_off=k;
	update_cached_values();
}


koord viewport_t::get_map2d_coord( const koord3d &viewpos ) const
{
	// just calculate the offset from the z-position

	const sint16 new_yoff = tile_raster_scale_y(viewpos.z*TILE_HEIGHT_STEP,cached_img_size);
	sint16 lines = 0;
	if(new_yoff>0) {
		lines = (new_yoff + (cached_img_size/4))/(cached_img_size/2);
	}
	else {
		lines = (new_yoff - (cached_img_size/4))/(cached_img_size/2);
	}
	return world->get_closest_coordinate( viewpos.get_2d() - koord( lines, lines ) );
}


koord viewport_t::get_viewport_coord( const koord& coord ) const
{
	return coord+cached_aggregated_off;
}


scr_coord viewport_t::get_screen_coord( const koord3d& pos, const koord& off) const
{
	// Historic disheartening comment to be preserved:
	// better not try to twist your brain to follow the retransformation ...

	koord scr_pos_2d = get_viewport_coord(pos.get_2d());

	const sint16 x = (scr_pos_2d.x-scr_pos_2d.y)*(cached_img_size/2)
		+ tile_raster_scale_x(off.x, cached_img_size)
		+ x_off;
	const sint16 y = (scr_pos_2d.x+scr_pos_2d.y)*(cached_img_size/4)
		+ tile_raster_scale_y(off.y-pos.z*TILE_HEIGHT_STEP, cached_img_size)
		+ ((cached_disp_width/cached_img_size)&1)*(cached_img_size/4)
		+ y_off;

	return scr_coord(x,y);
}


scr_coord viewport_t::scale_offset( const koord &value )
{
	return scr_coord(tile_raster_scale_x( value.x, cached_img_size ), tile_raster_scale_y( value.x, cached_img_size ));
}


// change the center viewport position
void viewport_t::change_world_position( koord new_ij, sint16 new_xoff, sint16 new_yoff )
{
	// truncate new_xoff, modify new_ij.x
	new_ij.x -= new_xoff/cached_img_size;
	new_ij.y += new_xoff/cached_img_size;
	new_xoff %= cached_img_size;

	// truncate new_yoff, modify new_ij.y
	int lines = 0;
	if(new_yoff>0) {
		lines = (new_yoff + (cached_img_size/4))/(cached_img_size/2);
	}
	else {
		lines = (new_yoff - (cached_img_size/4))/(cached_img_size/2);
	}
	new_ij -= koord( lines, lines );
	new_yoff -= (cached_img_size/2)*lines;

	new_ij = world->get_closest_coordinate(new_ij);

	//position changed? => update and mark dirty
	if(new_ij!=ij_off  ||  new_xoff!=x_off  ||  new_yoff!=y_off) {
		ij_off = new_ij;
		x_off = new_xoff;
		y_off = new_yoff;
		world->set_dirty();
		update_cached_values();
	}
}


// change the center viewport position for a certain ground tile
// any possible convoi to follow will be disabled
void viewport_t::change_world_position( const koord3d& new_ij )
{
	follow_convoi = convoihandle_t();
	change_world_position( get_map2d_coord( new_ij ) );
}


void viewport_t::change_world_position(const koord3d& pos, const koord& off, scr_coord sc)
{
	// see get_viewport_coord and update_cached_values
	koord scr_pos_2d = pos.get_2d() - view_ij_off;

	const sint16 c2 = cached_img_size/2;
	const sint16 c4 = cached_img_size/4;

	// see get_screen_coord - do not twist your brain etc
	// solve (-ijx + ijy)*(cached_img_size/2) + x_off = sc.x - xfix;
	// note: xfix and yfix need 32-bit precision or will overflow
	const sint32 xfix = (scr_pos_2d.x - scr_pos_2d.y)*(cached_img_size/2)
		+ tile_raster_scale_x(off.x, cached_img_size);

	// solve (-ijx - ijy)*(cached_img_size/4) + y_off = sc.y - yfix;
	const sint32 yfix = (scr_pos_2d.x + scr_pos_2d.y)*(cached_img_size/4)
		+ tile_raster_scale_y(off.y-pos.z*TILE_HEIGHT_STEP, cached_img_size)
		+ ((cached_disp_width/cached_img_size)&1)*(cached_img_size/4);

	// calculate center position
	const sint16 new_ij_x = (-(c2*(sc.y - yfix ))/c4 - (sc.x - xfix) ) / (2*c2);
	const sint16 new_ij_y = (-(c2*(sc.y - yfix ))/c4 + (sc.x - xfix) ) / (2*c2);

	// set new offsets to solve equation
	const sint16 new_x_off = sc.x - xfix - (-new_ij_x + new_ij_y) * c2;
	const sint16 new_y_off = sc.y - yfix - (-new_ij_x - new_ij_y) * c4;

	change_world_position(koord(new_ij_x, new_ij_y), new_x_off, new_y_off);
}


grund_t* viewport_t::get_ground_on_screen_coordinate(scr_coord screen_pos, sint32 &found_i, sint32 &found_j, const bool intersect_grid) const
{
	const int rw1 = cached_img_size;
	const int rw2 = rw1/2;
	const int rw4 = rw1/4;

	/*
	* berechnung der basis feldkoordinaten in i und j
	* this would calculate raster i,j koordinates if there was no height
	*  die formeln stehen hier zur erinnerung wie sie in der urform aussehen

	int base_i = (screen_x+screen_y)/2;
	int base_j = (screen_y-screen_x)/2;

	int raster_base_i = (int)floor(base_i / 16.0);
	int raster_base_j = (int)floor(base_j / 16.0);

	*/

	screen_pos.y += - y_off - rw2 - ((cached_disp_width/rw1)&1)*rw4;
	screen_pos.x += - x_off - rw2;

	const int i_off = rw4*(ij_off.x+get_viewport_ij_offset().x);
	const int j_off = rw4*(ij_off.y+get_viewport_ij_offset().y);

	bool found = false;
	// uncomment to: ctrl-key selects ground
	//bool select_karten_boden = event_get_last_control_shift()==2;

	// fallback: take kartenboden if nothing else found
	grund_t *bd = NULL;
	grund_t *gr = NULL;
	// for the calculation of hmin/hmax see simview.cc
	// for the definition of underground_level see grund_t::set_underground_mode
	const sint8 hmin = grund_t::underground_mode != grund_t::ugm_all ? min( world->get_groundwater() - 4, grund_t::underground_level ) : world->get_minimumheight();
	const sint8 hmax = grund_t::underground_mode == grund_t::ugm_all ? world->get_maximumheight() : min( grund_t::underground_level, world->get_maximumheight() );

	// find matching and visible grund
	for(sint8 hgt = hmax; hgt>=hmin; hgt--) {

		const int base_i = (screen_pos.x/2 + screen_pos.y   + tile_raster_scale_y((hgt*TILE_HEIGHT_STEP),rw1))/2;
		const int base_j = (screen_pos.y   - screen_pos.x/2 + tile_raster_scale_y((hgt*TILE_HEIGHT_STEP),rw1))/2;

		found_i = (base_i + i_off) / rw4;;
		found_j = (base_j + j_off) / rw4;;

		gr = world->lookup(koord3d(found_i,found_j,hgt));
		if (gr != NULL) {
			found = /*select_karten_boden ? gr->ist_karten_boden() :*/ gr->is_visible();
			if( ( gr->get_typ() == grund_t::tunnelboden || gr->get_typ() == grund_t::monorailboden ) && gr->get_weg_nr(0) == NULL && !gr->get_leitung()  &&  gr->find<zeiger_t>()) {
				// This is only a dummy ground placed by tool_build_tunnel_t or tool_build_way_t as a preview.
				found = false;
			}
			if (found) {
				break;
			}

			if (bd==NULL && gr->ist_karten_boden()) {
				bd = gr;
			}
		}
		if (grund_t::underground_mode==grund_t::ugm_level && hgt==hmax) {
			// fallback in sliced mode, if no ground is under cursor
			bd = world->lookup_kartenboden(found_i,found_j);
		}
		else if (intersect_grid){
			// We try to intersect with virtual nonexistent border tiles in south and east.
			if(  (gr = world->lookup_gridcoords( koord3d( found_i, found_j, hgt ) ))  ){
				found = true;
				break;
			}
		}

		// Last resort, try to intersect with the same tile +1 height, seems to be necessary on steep slopes
		// *NOTE* Don't do it on border tiles, since it will extend the range in which the cursor will be considered to be
		// inside world limits.
		if( found_i==(world->get_size().x-1)  ||  found_j == (world->get_size().y-1) ) {
			continue;
		}
		gr = world->lookup(koord3d(found_i,found_j,hgt+1));
		if(gr != NULL) {
			found = /*select_karten_boden ? gr->ist_karten_boden() :*/ gr->is_visible();
			if( gr->is_dummy_ground() && gr->find<zeiger_t>()) {
				// This is only a dummy ground placed by tool_build_tunnel_t or tool_build_way_t as a preview.
				found = false;
			}
			if (found) {
				break;
			}
			if (bd==NULL && gr->ist_karten_boden()) {
				bd = gr;
			}
		}
	}

	if(found) {
		return gr;
	}
	else {
		if(bd!=NULL){
			found_i = bd->get_pos().x;
			found_j = bd->get_pos().y;
			return bd;
		}
		return NULL;
	}
}


koord3d viewport_t::get_new_cursor_position( const scr_coord &screen_pos, bool grid_coordinates )
{
	const int rw4 = cached_img_size/4;

	int offset_y = 0;
	if(world->get_zeiger()->get_yoff() == Z_PLAN) {
		// already ok
	}
	else {
		// shifted by a quarter tile
		offset_y += rw4;
	}

	sint32 grid_x, grid_y;
	const grund_t *bd = get_ground_on_screen_coordinate(scr_coord(screen_pos.x, screen_pos.y + offset_y), grid_x, grid_y, grid_coordinates);

	// no suitable location found (outside map, ...)
	if (!bd) {
		return koord3d::invalid;
	}

	// offset needed for the raise / lower tool.
	sint8 groff = 0;

	if( bd->is_visible()  &&  grid_coordinates) {
		groff = bd->get_hoehe(world->get_corner_to_operate(koord(grid_x, grid_y))) - bd->get_hoehe();
	}

	return koord3d(grid_x, grid_y, bd->get_disp_height() + groff);
}


void viewport_t::metrics_updated()
{
	cached_disp_width = display_get_width();
	cached_disp_height = display_get_height();
	cached_img_size = get_tile_raster_width();

	set_viewport_ij_offset(koord(
		- cached_disp_width/(2*cached_img_size) - cached_disp_height/cached_img_size,
		  cached_disp_width/(2*cached_img_size) - cached_disp_height/cached_img_size
		) );
}


void viewport_t::rotate90( sint16 y_size )
{
	ij_off.rotate90(y_size);
	update_cached_values();
}


viewport_t::viewport_t( karte_t *world, const koord ij_off , sint16 x_off , sint16 y_off )
	: prepared_rect()
{
	this->world = world;
	assert(world);

	follow_convoi = convoihandle_t();

	this->ij_off = ij_off;
	this->x_off = x_off;
	this->y_off = y_off;

	metrics_updated();

}
