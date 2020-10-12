/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <math.h>
#include <string>
#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simobj.h"
#include "../display/simimg.h"
#include "../player/simplay.h"
#include "../simtypes.h"

#include "../boden/grund.h"

#include "../descriptor/tree_desc.h"

#include "../obj/groundobj.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simrandom.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/freelist.h"


#include "baum.h"

static const uint8 tree_age_index[12] =
{
	0,1,2,3,3,3,3,3,3,4,4,4
};

FLAGGED_PIXVAL baum_t::outline_color = 0;

// quick lookup of an image, assuring always five seasons and five ages
// missing images just have identical entries
// seasons are: 0=summer, 1=autumn, 2=winter, 3=spring, 4=snow
// snow image is used if tree is above snow line, or for arctic climate
static image_id tree_id_to_image[256][5*5];


// distributes trees on a map
void baum_t::distribute_trees(int dichte, sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom )
{
	// now we can proceed to tree planting routine itself
	// best forests results are produced if forest size is tied to map size -
	// but there is some nonlinearity to ensure good forests on small maps
	settings_t const& s             = welt->get_settings();
	sint32     const  x             = welt->get_size().x;
	sint32     const  y             = welt->get_size().y;
	unsigned   const t_forest_size  = (unsigned)pow(((double)x * (double)y), 0.25) * s.get_forest_base_size() / 11 + (x + y) / (2 * s.get_forest_map_size_divisor());
	uint8      const c_forest_count = (unsigned)pow(((double)x * (double)y), 0.5)  / s.get_forest_count_divisor();

DBG_MESSAGE("verteile_baeume()","creating %i forest",c_forest_count);
	for (uint8 c1 = 0 ; c1 < c_forest_count ; c1++) {
		// to have same execution order for simrand
		koord const start = koord::koord_random(x, y);
		koord const size  = koord(t_forest_size,t_forest_size) + koord::koord_random(t_forest_size, t_forest_size);
		create_forest( start, size, xtop, ytop, xbottom, ybottom );
	}

	fill_trees( dichte, xtop, ytop, xbottom, ybottom );
}


/*************************** first the static function for the baum_t and tree_desc_t administration ***************/

/*
 * Diese Tabelle ermoeglicht das Auffinden dient zur Auswahl eines Baumtypen
 */
vector_tpl<const tree_desc_t *> baum_t::tree_list(0);

// index vector into tree_idn, accessible per climate
weighted_vector_tpl<uint32>* baum_t::tree_list_per_climate = NULL;

/*
 * Diese Tabelle ermoeglicht das Auffinden einer Description durch ihren Namen
 */
stringhashtable_tpl<const tree_desc_t *> baum_t::desc_table;


// total number of trees
// the same for a certain climate
int baum_t::get_count(climate cl)
{
	return tree_list_per_climate[cl].get_count();
}


/**
 * tree planting function - it takes care of checking suitability of area
 */
uint8 baum_t::plant_tree_on_coordinate(koord pos, const uint8 maximum_count, const uint8 count)
{
	grund_t * gr = welt->lookup_kartenboden(pos);
	if(  gr  ) {
		if(  get_count( welt->get_climate(pos) ) > 0  &&  gr->ist_natur()  &&  gr->get_top() < maximum_count  ) {
			obj_t *obj = gr->obj_bei(0);
			if(obj) {
				switch(obj->get_typ()) {
					case obj_t::wolke:
					case obj_t::air_vehicle:
					case obj_t::baum:
					case obj_t::leitung:
					case obj_t::label:
					case obj_t::zeiger:
						// ok to built here
						break;
					case obj_t::groundobj:
						if(((groundobj_t *)obj)->get_desc()->can_build_trees_here()) {
							break;
						}
						/* FALLTHROUGH */
						// leave these (and all other empty)
					default:
						return 0;
				}
			}
			const uint8 count_planted = min( maximum_count - gr->get_top(), count);
			for (uint8 i=0; i<count_planted; i++) {
				gr->obj_add( new baum_t(gr->get_pos()) ); //plants the tree(s)
			}
			return count_planted;
		}
	}
	return 0;
}


/**
 * tree planting function - it takes care of checking suitability of area
 */
bool baum_t::plant_tree_on_coordinate(koord pos, const tree_desc_t *desc, const bool check_climate, const bool random_age )
{
	// none there
	if(  desc_table.empty()  ) {
		return false;
	}
	grund_t *gr = welt->lookup_kartenboden(pos);
	if(  gr  ) {
		if(  gr->ist_natur()  &&  gr->get_top() < welt->get_settings().get_max_no_of_trees_on_square()  &&  (!check_climate  ||  desc->is_allowed_climate( welt->get_climate(pos) ))  ) {
			if(  gr->get_top() > 0  ) {
				switch(gr->obj_bei(0)->get_typ()) {
					case obj_t::wolke:
					case obj_t::air_vehicle:
					case obj_t::baum:
					case obj_t::leitung:
					case obj_t::label:
					case obj_t::zeiger:
						// ok to built here
						break;
					case obj_t::groundobj:
						if(((groundobj_t *)(gr->obj_bei(0)))->get_desc()->can_build_trees_here()) {
							break;
						}
						/* FALLTHROUGH */
						// leave these (and all other empty)
					default:
						return false;
				}
			}
			baum_t *b = new baum_t(gr->get_pos(), desc); //plants the tree
			if(  random_age  ) {
				b->geburt = welt->get_current_month() - simrand(703);
				b->calc_off( welt->lookup( b->get_pos() )->get_grund_hang() );
			}
			gr->obj_add( b );
			return true; //tree was planted - currently unused value is not checked
		}
	}
	return false;
}


uint32 baum_t::create_forest(koord new_center, koord wh, sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom )
{
	// none there
	if(  desc_table.empty()  ) {
		return 0;
	}
	const sint16 xpos_f = new_center.x;
	const sint16 ypos_f = new_center.y;
	uint32 number_of_new_trees = 0;
	for( sint16 j = 0; j < wh.x; j++) {
		for( sint16 i = 0; i < wh.y; i++) {

			const sint32 x_tree_pos = (j-(wh.x>>1));
			const sint32 y_tree_pos = (i-(wh.y>>1));

			if( xtop > x_tree_pos  ||  x_tree_pos >= xbottom ) {
				continue;
			}
			if( ytop > y_tree_pos  ||  y_tree_pos >= ybottom ) {
				continue;
			}

			const uint64 distance = 1 + sqrt_i64( ((uint64)x_tree_pos*x_tree_pos*(wh.y*wh.y) + (uint64)y_tree_pos*y_tree_pos*(wh.x*wh.x)));
			const uint32 tree_probability = (uint32)( ( 8 * (uint32)((wh.x*wh.x)+(wh.y*wh.y)) ) / distance );

			if (tree_probability < 38) {
				continue;
			}

			uint8 number_to_plant = 0;
			uint8 const max_trees_here = min(welt->get_settings().get_max_no_of_trees_on_square(), (tree_probability - 38 + 1) / 2);
			for (uint8 c2 = 0 ; c2<max_trees_here; c2++) {
				const uint32 rating = simrand(10) + 38 + c2*2;
				if (rating < tree_probability ) {
					number_to_plant++;
				}
			}

			number_of_new_trees += baum_t::plant_tree_on_coordinate(koord((sint16)(xpos_f + x_tree_pos), (sint16)(ypos_f + y_tree_pos)), welt->get_settings().get_max_no_of_trees_on_square(), number_to_plant);
		}
	}
	return number_of_new_trees;
}


void baum_t::fill_trees(int dichte, sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom )
{
	// none there
	if(  desc_table.empty()  ) {
		return;
	}
DBG_MESSAGE("verteile_baeume()","distributing single trees");
	koord pos;
	for(  pos.y=ytop;  pos.y<ybottom;  pos.y++  ) {
		for(  pos.x=xtop;  pos.x<xbottom;  pos.x++  ) {
			grund_t *gr = welt->lookup_kartenboden(pos);
			if(gr->get_top() == 0  &&  gr->get_typ() == grund_t::boden)  {
				// plant spare trees, (those with low preffered density) or in an entirely tree climate
				uint16 cl = 1 << welt->get_climate(pos);
				settings_t const& s = welt->get_settings();
				if ((cl & s.get_no_tree_climates()) == 0 && ((cl & s.get_tree_climates()) != 0 || simrand(s.get_forest_inverse_spare_tree_density() * dichte) < 100)) {
					plant_tree_on_coordinate(pos, 1, 1);
				}
			}
		}
	}
}


static bool compare_tree_desc(const tree_desc_t* a, const tree_desc_t* b)
{
	// same level - we do an artificial but unique sorting by (untranslated) name
	return strcmp(a->get_name(), b->get_name())<0;
}


bool baum_t::successfully_loaded()
{
	if(  desc_table.empty()  ) {
		DBG_MESSAGE("baum_t", "No trees found - feature disabled");
	}

	FOR(stringhashtable_tpl<tree_desc_t const*>, const& i, desc_table) {
		tree_list.insert_ordered(i.value, compare_tree_desc);
		if(  tree_list.get_count()==255  ) {
			dbg->error( "baum_t::successfully_loaded()", "Maximum tree count exceeded! (max 255 instead of %i)", desc_table.get_count() );
			break;
		}
	}
	tree_list.append( NULL );

	delete [] tree_list_per_climate;
	tree_list_per_climate = new weighted_vector_tpl<uint32>[MAX_CLIMATES];

	// clear cache
	memset( tree_id_to_image, -1, sizeof(tree_id_to_image) );
	// now register all trees for all fitting climates
	for(  uint32 typ=0;  typ<tree_list.get_count()-1;  typ++  ) {
		// add this tree to climates
		for(  uint8 j=0;  j<MAX_CLIMATES;  j++  ) {
			if(  tree_list[typ]->is_allowed_climate((climate)j)  ) {
				tree_list_per_climate[j].append(typ, tree_list[typ]->get_distribution_weight());
			}
		}
		// create cache images
		for(  uint8 season = 0;  season < 5;  season++  ) {
			uint8 use_season = 0;
			const sint16 seasons = tree_list[typ]->get_seasons();
			if(  seasons > 1  ) {
				use_season = season;
				// three possibilities
				if(  seasons < 4  ) {
					// only summer and winter => season 4 with winter image
					use_season = (season == 4);
				}
				else if(  seasons == 4  ) {
					// all there, but the snowy special image missing
					if(  season == 4  ) {
						// take spring image (gave best results with pak64, pak.german) ////// but season 2 is winter????
						use_season = 2;
					}
				}
			}
			for(  uint8 age = 0;  age < 5;  age++  ) {
				tree_id_to_image[typ][season * 5 + age] = tree_list[typ]->get_image_id( use_season, age );
			}
		}
	}
	return true;
}


bool baum_t::register_desc(tree_desc_t *desc)
{
	// avoid duplicates with same name
	if(  desc_table.remove(desc->get_name())  ) {
		dbg->doubled( "baum_t", desc->get_name() );
	}
	desc_table.put(desc->get_name(), desc );
	return true;
}


// calculates tree position on a tile
// takes care of slopes
void baum_t::calc_off(uint8 slope, sint8 x_, sint8 y_)
{
	sint16 random = (sint16)( get_pos().x + get_pos().y + get_pos().z + slope + (sint16)(intptr_t)this );
	// point on tile (imaginary origin at sw corner, x axis: north, y axis: east
	sint16 x = x_==-128 ? (random + tree_id) & 31  : x_;
	sint16 y = y_==-128 ? (random + get_age()) & 31 : y_;

	// the last bit has to be the same
	y = y ^ (x&1);

	// bilinear interpolation of tile height
	uint32 zoff_ = ((corner_ne(slope)*x*y + corner_nw(slope)*x*(32-y)
	                 + corner_se(slope)*(32-x)*y + corner_sw(slope)*(32-x)*(32-y)) * TILE_HEIGHT_STEP) / (32*32);
	// now zoff between 0 and TILE_HEIGHT_STEP-1
	zoff = zoff_ < (uint32)TILE_HEIGHT_STEP ? zoff_ : TILE_HEIGHT_STEP-1u;

	// xoff must be even
	set_xoff( x + y - 32 );
	set_yoff( (y - x)/2 - zoff);
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


image_id baum_t::get_image() const
{
	if(  env_t::hide_trees  ) {
		if(  env_t::hide_with_transparency  ) {
			// we need the real age for transparency or real image
			return IMG_EMPTY;
		}
		else {
			return tree_id_to_image[ tree_id ][ season*5 ];
		}
	}
	const uint8 baum_alter = tree_age_index[min(get_age()>>6, 11u)];
	return tree_id_to_image[ tree_id ][ season*5 + baum_alter ];
//	return get_desc()->get_image_id( season, baum_alter );
}


// image which transparent outline is used
image_id baum_t::get_outline_image() const
{
	const uint8 baum_alter = tree_age_index[min(get_age()>>6, 11u)];
	return tree_id_to_image[ tree_id ][ season*5 + baum_alter ];
//	return get_desc()->get_image_id( season, baum_alter );
}


uint32 baum_t::get_age() const
{
	sint32 age = welt->get_current_month() - geburt;
	if (age<0) {
		// correct underflow, geburt is 16bit
		age += 1 << 16;
	}
	return age;
}


/**
 * also checks for distribution values
 */
uint16 baum_t::random_tree_for_climate_intern(climate cl)
{
	// now weight their distribution
	weighted_vector_tpl<uint32> const& t = tree_list_per_climate[cl];
	return t.empty() ? 0xFFFF : pick_any_weighted(t);
}


baum_t::baum_t(loadsave_t *file) : obj_t()
{
	season = 0;
	geburt = welt->get_current_month();
	tree_id = 0;
	rdwr(file);
}


baum_t::baum_t(koord3d pos) : obj_t(pos)
{
	// generate aged trees
	// might underflow
	geburt = welt->get_current_month() - simrand(703);
	tree_id = (uint8)random_tree_for_climate_intern( welt->get_climate( pos.get_2d() ) );
	season = 0;
	calc_off( welt->lookup( get_pos())->get_grund_hang() );
	calc_image();
}


baum_t::baum_t(koord3d pos, uint8 type, sint32 age, uint8 slope ) : obj_t(pos)
{
	geburt = welt->get_current_month()-age; // might underflow
	tree_id = type;
	season = 0;
	calc_off( slope );
	calc_image();
}


baum_t::baum_t(koord3d pos, const tree_desc_t *desc) : obj_t(pos)
{
	geburt = welt->get_current_month();
	tree_id = tree_list.index_of(desc);
	season = 0;
	calc_off( welt->lookup( get_pos())->get_grund_hang() );
	calc_image();
}


bool baum_t::saee_baum()
{
	// spawn a new tree in an area 3x3 tiles around
	// the area for normal new tree planting is slightly more restricted, square of 9x9 was too much

	// to have same execution order for simrand
	const sint16 sx = simrand(5)-2;
	const sint16 sy = simrand(5)-2;
	const koord k = get_pos().get_2d() + koord(sx,sy);

	return plant_tree_on_coordinate(k, tree_list[tree_id], true, false);
}


/* we should be as fast as possible, because trees are nearly the most common object on a map */
bool baum_t::check_season(const bool)
{
	// take care of birth/death and seasons
	sint32 age = (welt->get_current_month() - geburt);

	// attention: integer underflow (geburt is 16bit, month 32bit);
	while(  age < 0  ) {
		age += 1 << 16;
	}

	if(  age >= 512  &&  age <= 515  ) {
		// only in this month a tree can span new trees
		// only 1-3 trees will be planted....
		uint8 const c_plant_tree_max = 1 + simrand( welt->get_settings().get_max_no_of_trees_on_square() );
		uint retrys = 0;
		for(  uint8 c_temp = 0;  c_temp < c_plant_tree_max  &&  retrys < c_plant_tree_max;  c_temp++  ) {
			if(  !saee_baum()  ) {
				retrys++;
				c_temp--;
			}
		}
		// we make the tree four months older to avoid second spawning
		geburt = geburt - 4;
	}

	// tree will die after 704 month (i.e. 58 years 8 month)
	if(  age >= 704  ) {
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


void baum_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "baum_t" );

	obj_t::rdwr(file);

	sint32 alter = (welt->get_current_month() - geburt)<<18;
	file->rdwr_long(alter);

	// after loading, calculate new
	geburt = welt->get_current_month() - (alter>>18);

	if(file->is_loading()) {
		char buf[128];
		file->rdwr_str(buf, lengthof(buf));
		const tree_desc_t *desc = desc_table.get( buf );
		if(  !tree_list.is_contained(desc)  ) {
			desc = desc_table.get( translator::compatibility_name(buf) );
		}
		if(  tree_list.is_contained(desc)  ) {
			tree_id = tree_list.index_of( desc );
		}
		else {
			// not a tree
			tree_id = tree_list.get_count()-1;
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


void baum_t::show_info()
{
	if(env_t::tree_info) {
		obj_t::show_info();
	}
}


void baum_t::info(cbuffer_t & buf) const
{
	obj_t::info(buf);

	buf.append( translator::translate(get_desc()->get_name()) );
	buf.append( "\n" );
	uint32 age = get_age();
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


void *baum_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(baum_t));
}


void baum_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(baum_t),p);
}
