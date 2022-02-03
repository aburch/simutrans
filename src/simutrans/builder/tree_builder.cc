/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "tree_builder.h"

#include "../dataobj/settings.h"
#include "../obj/baum.h"
#include "../obj/groundobj.h"
#include "../world/simworld.h"
#include "../utils/simrandom.h"

#include <cmath>


static karte_ptr_t welt;


static const uint8 tree_age_index[(baum_t::AGE_LIMIT >> 6) + 1] =
{
	0,1,2,3,3,3,3,3,3,4,4,4
};


vector_tpl<const tree_desc_t *> tree_builder_t::tree_list(0);
weighted_vector_tpl<uint8> tree_builder_t::tree_list_per_climate[MAX_CLIMATES];
stringhashtable_tpl<const tree_desc_t *> tree_builder_t::desc_table;
vector_tpl<plainstring> tree_builder_t::loaded_tree_names;


/// Quick lookup of an image, assuring always five seasons and five ages.
/// Missing images just have identical entries.
/// Seasons are: 0=summer, 1=autumn, 2=winter, 3=spring, 4=snow
/// Snow image is used if tree is above snow line, or for arctic climate
static image_id tree_id_to_image[256][5*5];


image_id tree_builder_t::get_tree_image(uint8 tree_id, uint32 age, uint8 season)
{
	const uint8 tree_age_idx = tree_age_index[min(age>>6, 11u)];
	assert(tree_age_idx < 5);
	return tree_id_to_image[ tree_id ][ season*5 + tree_age_idx ];
}


const tree_desc_t *tree_builder_t::find_tree(const char *tree_name)
{
	return tree_list.empty() ? NULL : desc_table.get(tree_name);
}


uint8 tree_builder_t::plant_tree_on_coordinate(koord pos, const uint8 maximum_count, const uint8 count)
{
	grund_t *gr = welt->lookup_kartenboden(pos);
	if(  gr  ) {
		if(  has_trees_for_climate( welt->get_climate(pos) )  &&  gr->ist_natur()  &&  gr->get_top() < maximum_count  ) {
			obj_t *obj = gr->obj_bei(0);
			if(obj) {
				switch(obj->get_typ()) {
					case obj_t::wolke:
					case obj_t::air_vehicle:
					case obj_t::baum:
					case obj_t::leitung:
					case obj_t::label:
					case obj_t::zeiger:
						// ok to build here
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


bool tree_builder_t::plant_tree_on_coordinate(koord pos, const tree_desc_t *desc, const bool check_climate, const bool random_age)
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
				b->geburt = welt->get_current_month() - simrand(baum_t::AGE_LIMIT-1);
				b->calc_off( welt->lookup( b->get_pos() )->get_grund_hang() );
			}

			gr->obj_add( b );
			return true; //tree was planted - currently unused value is not checked
		}
	}

	return false;
}


uint32 tree_builder_t::create_forest(koord new_center, koord wh, sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom)
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

			if( xtop > (xpos_f + x_tree_pos)  ||  (xpos_f + x_tree_pos) >= xbottom ) {
				continue;
			}
			if( ytop > (ypos_f + y_tree_pos)  ||  (ypos_f + y_tree_pos) >= ybottom ) {
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

			const koord tree_pos = koord((sint16)(xpos_f + x_tree_pos), (sint16)(ypos_f + y_tree_pos));
			const uint8 max_trees_per_square = welt->get_settings().get_max_no_of_trees_on_square();

			number_of_new_trees += plant_tree_on_coordinate(tree_pos, max_trees_per_square, number_to_plant);
		}
	}

	return number_of_new_trees;
}


void tree_builder_t::fill_trees(int dichte, sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom )
{
	// none there
	if(  desc_table.empty()  ) {
		return;
	}

	DBG_MESSAGE("tree_builder_t::fill_trees", "distributing single trees");
	koord pos;

	for(  pos.y=ytop;  pos.y<ybottom;  pos.y++  ) {
		for(  pos.x=xtop;  pos.x<xbottom;  pos.x++  ) {
			grund_t *gr = welt->lookup_kartenboden(pos);
			if(gr->get_top() == 0  &&  gr->get_typ() == grund_t::boden)  {
				// plant spare trees, (those with low preffered density) or in an entirely tree climate
				const uint16 cl = 1 << welt->get_climate(pos);
				const settings_t &s = welt->get_settings();

				if ((cl & s.get_no_tree_climates()) == 0 && ((cl & s.get_tree_climates()) != 0 || simrand(s.get_forest_inverse_spare_tree_density() * dichte) < 100)) {
					plant_tree_on_coordinate(pos, 1, 1);
				}
			}
		}
	}
}


static bool compare_tree_desc(const tree_desc_t *a, const tree_desc_t *b)
{
	// same level - we do an artificial but unique sorting by (untranslated) name
	return strcmp(a->get_name(), b->get_name())<0;
}


bool tree_builder_t::successfully_loaded()
{
	if(  desc_table.empty()  ) {
		DBG_MESSAGE("tree_builder_t::successfully_loaded", "No trees found - feature disabled");
	}

	FOR(stringhashtable_tpl<tree_desc_t const*>, const& i, desc_table) {
		tree_list.insert_ordered(i.value, compare_tree_desc);
		if(  tree_list.get_count()==255  ) {
			dbg->error( "tree_builder_t::successfully_loaded", "Maximum tree count exceeded! (%u > 255)", desc_table.get_count() );
			break;
		}
	}

	tree_list.append( NULL );

	// clear cache
	memset( tree_id_to_image, -1, sizeof(tree_id_to_image) );

	// now register all trees for all fitting climates
	for(  uint32 typ=0;  typ<tree_list.get_count()-1;  typ++  ) {
		// add this tree to climates
		for(  uint8 j=0;  j<MAX_CLIMATES;  j++  ) {
			if(  tree_list[typ]->is_allowed_climate((climate)j)  ) {
				assert(typ <= 255);
				tree_list_per_climate[j].append((uint8)typ, tree_list[typ]->get_distribution_weight());
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

			for(  uint8 age_idx = 0;  age_idx < 5;  age_idx++  ) {
				tree_id_to_image[typ][season * 5 + age_idx] = tree_list[typ]->get_image_id( use_season, age_idx );
			}
		}
	}

	return true;
}


bool tree_builder_t::register_desc(const tree_desc_t *desc)
{
	// avoid duplicates with same name
	if(  desc_table.remove(desc->get_name())  ) {
		dbg->doubled( "baum_t", desc->get_name() );
	}

	desc_table.put(desc->get_name(), desc );
	return true;
}


void tree_builder_t::distribute_trees(int dichte, sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom)
{
	// now we can proceed to tree planting routine itself
	// best forests results are produced if forest size is tied to map size -
	// but there is some nonlinearity to ensure good forests on small maps
	settings_t const& s             = welt->get_settings();
	sint32     const  x             = welt->get_size().x;
	sint32     const  y             = welt->get_size().y;
	unsigned   const t_forest_size  = (uint32)pow(((double)x * (double)y), 0.25) * s.get_forest_base_size() / 11 + (x + y) / (2 * s.get_forest_map_size_divisor());
	uint32     const c_forest_count = (uint32)pow(((double)x * (double)y), 0.5)  / s.get_forest_count_divisor();

	DBG_MESSAGE("tree_builder_t::distribute_trees", "Creating %i forests", c_forest_count);

	for (uint32 c1 = 0; c1 < c_forest_count ; c1++) {
		// to have same execution order for simrand
		koord const start = koord::koord_random(x, y);
		koord const size  = koord(t_forest_size,t_forest_size) + koord::koord_random(t_forest_size, t_forest_size);

		create_forest( start, size, xtop, ytop, xbottom, ybottom );
	}

	fill_trees( dichte, xtop, ytop, xbottom, ybottom );
}


const tree_desc_t *tree_builder_t::random_tree_for_climate(climate cl)
{
	const uint16 b = random_tree_id_for_climate(cl);
	return b!=0xFFFF ? tree_list[b] : NULL;
}


uint16 tree_builder_t::random_tree_id_for_climate(climate cl)
{
	// now weight their distribution
	weighted_vector_tpl<uint8> const &t = tree_list_per_climate[cl];
	return t.empty() ? 0xFFFF : pick_any_weighted(t);
}


bool tree_builder_t::spawn_tree_near(const baum_t *tree, int radius)
{
	// to have same execution order for simrand
	const sint16 sx = simrand(2*radius + 1)-radius;
	const sint16 sy = simrand(2*radius + 1)-radius;
	const koord k = tree->get_pos().get_2d() + koord(sx,sy);

	return plant_tree_on_coordinate(k, tree->get_desc(), true, false);
}


void tree_builder_t::rdwr_tree_ids(loadsave_t *file)
{
	xml_tag_t tag(file, "tree_ids");

	uint8 num_trees = tree_list.get_count()-1;
	file->rdwr_byte(num_trees);

	if (file->is_loading()) {
		loaded_tree_names.clear();
		plainstring str;

		for (uint8 i = 0; i < num_trees; ++i) {
			file->rdwr_str(str);
			loaded_tree_names.append(str);
		}
	}
	else {
		for (uint8 i = 0; i < num_trees; ++i) {
			plainstring str = tree_list[i]->get_name();
			file->rdwr_str(str);
		}
	}
}


const char *tree_builder_t::get_loaded_desc_name(uint8 loaded_id)
{
	return (loaded_id < loaded_tree_names.get_count()) ? loaded_tree_names[loaded_id].c_str() : NULL;
}
