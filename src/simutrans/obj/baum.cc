/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "baum.h"

#include "groundobj.h"

#include "../ground/grund.h"
#include "../dataobj/environment.h"
#include "../dataobj/freelist.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../descriptor/tree_desc.h"
#include "../display/simimg.h"
#include "../player/simplay.h"
#include "../simdebug.h"
#include "../simtypes.h"
#include "../world/simworld.h"
#include "../utils/cbuffer.h"
#include "../utils/simrandom.h"

#include <cmath>
#include <cstdio>
#include <string>


FLAGGED_PIXVAL baum_t::outline_color = 0;

freelist_tpl<baum_t> baum_t::fl;

baum_t::baum_t(loadsave_t *file) :
	obj_t(),
	geburt(welt->get_current_month()),
	tree_id(0),
	season(0)
{
	rdwr(file);
}


baum_t::baum_t(koord3d pos) :
	obj_t(pos),
	geburt(welt->get_current_month() - simrand(baum_t::AGE_LIMIT-1)), // generate aged trees, might underflow
	season(0)
{
	tree_id = (uint8)tree_builder_t::random_tree_id_for_climate( welt->get_climate( pos.get_2d() ) );

	calc_off( welt->lookup( get_pos())->get_grund_hang() );
	calc_image();
}


baum_t::baum_t(koord3d pos, uint8 type, uint16 age, slope_t::type slope ) :
	obj_t(pos),
	geburt(welt->get_current_month() - age), // might underflow
	tree_id(type),
	season(0)
{
	calc_off( slope );
	calc_image();
}


baum_t::baum_t(koord3d pos, const tree_desc_t *desc) :
	obj_t(pos),
	geburt(welt->get_current_month()),
	tree_id(tree_builder_t::get_id_by_desc(desc)),
	season(0)
{

	calc_off( welt->lookup( get_pos())->get_grund_hang() );
	calc_image();
}


void baum_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "baum_t" );

	obj_t::rdwr(file);

	uint32 age = get_age() << 18;
	file->rdwr_long(age);

	// after loading, calculate anew
	age &= 0xFFF << 18;
	geburt = welt->get_current_month() - (age>>18);

	if(file->is_loading()) {
		char buf[128];
		file->rdwr_str(buf, lengthof(buf));

		const tree_desc_t *desc = tree_builder_t::get_desc_by_name( buf );
		if(  !desc  ) {
			desc = tree_builder_t::get_desc_by_name( translator::compatibility_name(buf) );
		}

		if(  desc  ) {
			tree_id = tree_builder_t::get_id_by_desc( desc );
		}
		else {
			// not a tree
			tree_id = tree_builder_t::get_num_trees();
		}
	}
	else {
		const char *c = get_desc()->get_name();
		file->rdwr_str(c);
	}

	// z-offset
	if(file->is_version_atleast(111, 1)) {
		uint8 zoff_ = zoff;
		file->rdwr_byte(zoff_);
		zoff = zoff_;
	}
	else {
		// correct z-offset
		if(file->is_loading()) {
			// this will trigger recalculation of offset in finish_rd()
			// we cant call calc_off() since this->pos is still invalid
			set_xoff(-128);
		}
	}
}


void baum_t::finish_rd()
{
	if(get_xoff()==-128) {
		calc_off(welt->lookup( get_pos())->get_grund_hang());
	}
}


image_id baum_t::get_image() const
{
	if(  env_t::hide_trees  ) {
		if(  env_t::hide_with_transparency  ) {
			// we need the real age for transparency or real image
			return IMG_EMPTY;
		}
		else {
			return tree_builder_t::get_tree_image(tree_id, 0, season);
		}
	}

	return tree_builder_t::get_tree_image(tree_id, get_age(), season);
	//	return get_desc()->get_image_id( season, baum_alter );
}


// image which transparent outline is used
image_id baum_t::get_outline_image() const
{
	return tree_builder_t::get_tree_image(tree_id, get_age(), season);
	//	return get_desc()->get_image_id( season, baum_alter );
}


// actually calculates only the season
void baum_t::calc_image()
{
	// summer autumn winter spring
	season = welt->get_season();
	if(  welt->get_snowline() <= get_pos().z  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate  ) {
		// snowy winter graphics
		season = 4;
	}
	else if(welt->get_snowline()<=get_pos().z+1  &&  season==0) {
		// snowline crossing in summer
		// so at least some weeks spring/autumn
		season = welt->get_last_month() <=5 ? 3 : 1;
	}
}



FLAGGED_PIXVAL baum_t::get_outline_colour() const
{
	return outline_color;
}


void baum_t::recalc_outline_color()
{
	outline_color = (env_t::hide_trees  &&  env_t::hide_with_transparency) ?
		(TRANSPARENT25_FLAG | OUTLINE_FLAG | color_idx_to_rgb(COL_BLACK)) : 0;
}


/* we should be as fast as possible, because trees are nearly the most common object on a map */
bool baum_t::check_season(const bool)
{
	// take care of birth/death and seasons
	const uint16 age = get_age();

	if(  age >= baum_t::SPAWN_PERIOD_START  &&  age < baum_t::SPAWN_PERIOD_START + baum_t::SPAWN_PERIOD_LENGTH  ) {
		// only in this month a tree can span new trees
		// only 1-3 trees will be planted....
		uint8 const c_plant_tree_max = 1 + simrand( welt->get_settings().get_max_no_of_trees_on_square() );
		uint retries = 0;

		for(  uint8 c_temp = 0;  c_temp < c_plant_tree_max  &&  retries < c_plant_tree_max;  c_temp++  ) {
			if(  !tree_builder_t::spawn_tree_near(this)  ) {
				retries++;
				c_temp--;
			}
		}

		// we make the tree four months older to avoid second spawning
		geburt -= baum_t::SPAWN_PERIOD_LENGTH;
	}

	// tree will die after 704 months (i.e. 58 years 8 months)
	if(  age >= baum_t::AGE_LIMIT  ) {
		mark_image_dirty( get_image(), 0 );
		return false;
	}

	// update seasonal image
	const uint8 old_season = season;
	calc_image();
	if(  season != old_season  ) {
		mark_image_dirty( get_image(), 0 );
	}

	return true;
}


void baum_t::rotate90()
{
	// cant use obj_t::rotate90 to rotate offsets as it rotates them only if xoff!=0
	sint8 old_yoff = get_yoff() + zoff;
	sint8 old_xoff = get_xoff();

	// rotate position
	obj_t::rotate90();

	// .. and the offsets
	set_xoff( -2 * old_yoff );
	set_yoff( old_xoff/2 - zoff);
}


void baum_t::recalc_off()
{
	// reconstruct position on tile
	const sint8 xoff = get_xoff() + 32;       // = x+y
	const sint8 yoff = 2*(get_yoff() + zoff); // = y-x
	sint8 x = (xoff - yoff) / 2;
	sint8 y = (xoff + yoff) / 2;

	calc_off(welt->lookup( get_pos())->get_grund_hang(), x, y);
}


// calculates tree position on a tile
// takes care of slopes
void baum_t::calc_off(slope_t::type slope, sint8 x_, sint8 y_)
{
	const sint16 random = (sint16)( get_pos().x + get_pos().y + get_pos().z + slope + tree_id + geburt );

	// point on tile (imaginary origin at sw corner, x axis: north, y axis: east
	sint16 x = x_==-128 ? (random + tree_id) & 31  : x_;
	sint16 y = y_==-128 ? (random + geburt) & 31 : y_;

	// the last bit has to be the same
	y ^= x&1;

	// bilinear interpolation of tile height
	const uint32 zoff_ =
		((corner_ne(slope) * x      * y       +
		  corner_nw(slope) * x      * (32-y)  +
		  corner_se(slope) * (32-x) * y       +
		  corner_sw(slope) * (32-x) * (32-y))
		* TILE_HEIGHT_STEP) / (32*32);

	// now zoff between 0 and TILE_HEIGHT_STEP-1
	zoff = zoff_ < (uint32)TILE_HEIGHT_STEP ? zoff_ : TILE_HEIGHT_STEP-1u;

	// xoff must be even
	set_xoff( x + y - 32 );
	set_yoff( (y - x)/2 - zoff);
}


void baum_t::show_info()
{
	if(env_t::tree_info) {
		obj_t::show_info();
	}
}


void baum_t::info(cbuffer_t &buf) const
{
	obj_t::info(buf);

	buf.append( translator::translate(get_desc()->get_name()) );
	buf.append( "\n" );

	const int age = (int)get_age();
	buf.printf( translator::translate("%i years %i months old."), age/12, (age%12) );

	if (char const* const maker = get_desc()->get_copyright()) {
		buf.append("\n\n");
		buf.printf(translator::translate("Constructed by %s"), maker);
	}
}


void baum_t::cleanup(player_t *player)
{
	player_t::book_construction_costs(player, welt->get_settings().cst_remove_tree, get_pos().get_2d(), ignore_wt);
	mark_image_dirty( get_image(), 0 );
}


uint16 baum_t::get_age() const
{
	return (welt->get_current_month() - (uint32)geburt) % (1<<12);
}
