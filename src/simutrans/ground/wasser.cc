/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "wasser.h"

#include "../dataobj/environment.h"
#include "../descriptor/ground_desc.h"
#include "../obj/gebaeude.h"
#include "../world/simworld.h"


int wasser_t::stage = 0;
bool wasser_t::change_stage = false;


wasser_t::wasser_t(loadsave_t *file, koord pos) :
	grund_t(koord3d(pos,0) ),
	ribi(ribi_t::none),
	canal_ribi(ribi_t::none)
{
	rdwr(file);
}


wasser_t::wasser_t(koord3d pos) :
	grund_t(pos),
	ribi(ribi_t::none),
	canal_ribi(ribi_t::none),
	display_ribi(ribi_t::none)
{
	set_hoehe( welt->get_water_hgt( pos.get_2d() ) );
	slope = slope_t::flat;
}


// for animated waves
void wasser_t::prepare_for_refresh()
{
	if(!welt->is_fast_forward()  &&  env_t::water_animation>0) {
		int new_stage = (welt->get_ticks() / env_t::water_animation) % ground_desc_t::water_animation_stages;
		wasser_t::change_stage = (new_stage != stage);
		wasser_t::stage = new_stage;
	}
	else {
		wasser_t::change_stage = false;
	}
}


/**
 * helper function: return maximal possible ribis, does not
 * take water ribi of sea tiles into account.
 */
ribi_t::ribi get_base_water_ribi(grund_t *gr)
{
	return gr->is_water() ? (ribi_t::ribi)ribi_t::all : gr->grund_t::get_weg_ribi(water_wt);
}


void wasser_t::calc_image_internal(const bool calc_only_snowline_change)
{
	if(  !calc_only_snowline_change  ) {
		koord pos2d( get_pos().get_2d() );
		sint8 height = welt->get_water_hgt( pos2d );\

		sint8 zpos = min( welt->lookup_hgt( pos2d ), height ); // otherwise slope will fail ...

		if(  grund_t::underground_mode == grund_t::ugm_level  &&  grund_t::underground_level < zpos  ) {
			set_image(IMG_EMPTY);
		}
		else {
			set_image( min( height - zpos, ground_desc_t::water_depth_levels ) /*ground_desc_t::get_ground_tile(0,zpos)*/ );
		}
		recalc_ribis();

		// artifical walls from here on ...
		grund_t::calc_back_image( height, slope_t::flat );
	}
}


void wasser_t::recalc_ribis()
{
	// test tiles to north, south, east and west and add to ribi if water
	ribi = ribi_t::none;
	canal_ribi = ribi_t::none;
	display_ribi = ribi_t::none;

	bool harbour = find<gebaeude_t>();

	for(  uint8 i = 0;  i < 4;  i++  ) {
		grund_t *gr_neighbour = NULL;
		ribi_t::ribi test = ribi_t::nesw[i];

		if( get_neighbour(gr_neighbour, invalid_wt, test) ) {
			// neighbour tile has water ways
			ribi_t::ribi ribi_neigh_base = get_base_water_ribi(gr_neighbour);
			if ((ribi_neigh_base & ribi_t::reverse_single(test))==0) {
				// we cannot go back to our tile
				continue;
			}

			// set water ribi bit
			ribi |= test;

			// test whether we can turn on neighbour canal tile
			if (!gr_neighbour->is_water()) {
				ribi_t::ribi ribi_orth = ribi_t::doubles( ribi_t::rotate90( test ));
				if ((ribi_neigh_base & ribi_orth) != ribi_orth) {
					// turning not possible, mark it as canal ribi
					canal_ribi |= test;
				}
				display_ribi |= test;
			}
			else {
				// if building is on one but not on the other tile
				// pretend tiles are not connected for displaying purposes
				if ( (gr_neighbour->find<gebaeude_t>() != NULL) == harbour) {
					display_ribi |= test;
				}
			}
		}
	}
}


void wasser_t::recalc_water_neighbours()
{
	recalc_ribis();

	for(  uint8 i = 0;  i < 4;  i++  ) {
		grund_t *gr_neighbour = NULL;
		ribi_t::ribi test = ribi_t::nesw[i];

		if( get_neighbour(gr_neighbour, invalid_wt, test) ) {
			if (wasser_t* gr_water = dynamic_cast<wasser_t*>(gr_neighbour) ) {
				gr_water->recalc_ribis();
			}
		}
	}
}


void wasser_t::rotate90()
{
	grund_t::rotate90();
	ribi = ribi_t::rotate90(ribi);
	canal_ribi = ribi_t::rotate90(canal_ribi);
	display_ribi = ribi_t::rotate90(display_ribi);
}


const char* wasser_t::get_name() const
{
	return pos.z > welt->get_groundwater() ? "Lake" : "Sea";
}
