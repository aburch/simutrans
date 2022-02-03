/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../world/simworld.h"
#include "../simskin.h"

#include "../obj/baum.h"

#include "../dataobj/environment.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"

#include "boden.h"

#include "../descriptor/ground_desc.h"
#include "../descriptor/skin_desc.h"


boden_t::boden_t(loadsave_t *file, koord pos ) : grund_t( koord3d(pos,0) )
{
	grund_t::rdwr( file );

	// restoring trees (disadvantage: losing offsets but much smaller savegame footprint)
	if(  file->is_version_atleast(110, 1)  ) {
		sint16 id = file->rd_obj_id();
		while(  id!=-1  ) {
			if (id != (uint8)id) {
				dbg->warning("boden_t::boden_t", "Invalid tree id %hd, using %hhu", id, (uint8)id);
			}

			uint16 age;
			if (file->is_version_atleast(122, 2)) {
				file->rdwr_short(age);
				age &= 0xFFF;
			}
			else {
				sint32 val;
				file->rdwr_long(val);
				age = (uint32)val & 0xFFF;
			}

			// check if we still have this tree
			const tree_desc_t *desc = NULL;
			const char *desc_name = tree_builder_t::get_loaded_desc_name((uint8)id);

			if (desc_name) {
				desc = tree_builder_t::get_desc_by_name(desc_name);
				if (!desc) {
					desc = tree_builder_t::get_desc_by_name(translator::compatibility_name(desc_name));
				}
			}
			else {
				desc = tree_builder_t::get_desc_by_id((uint8)id);
			}

			if (desc) {
				baum_t *tree = new baum_t( get_pos(), (uint8)id, age, slope );
				objlist.add( tree );
			}
			else {
				dbg->warning( "boden_t::boden_t()", "Could not restore tree type %hhu at (%s)", (uint8)id, pos.get_str() );
			}

			// check for next tree
			id = file->rd_obj_id();
		}
	}
}


boden_t::boden_t(koord3d pos, slope_t::type sl) : grund_t(pos)
{
	slope = sl;
}


// only for more compact saving of trees
void boden_t::rdwr(loadsave_t *file)
{
	grund_t::rdwr(file);

	if(  file->is_version_atleast(110, 1)  ) {
		// a server sends the smallest possible savegames to clients, i.e. saves only types and age of trees
		if(  (env_t::server  ||  file->is_version_atleast(122, 2))  &&  !hat_wege()  ) {
			for(  uint8 i=0;  i<objlist.get_top();  i++  ) {
				obj_t *obj = objlist.bei(i);
				if(  obj->get_typ()==obj_t::baum  ) {
					baum_t *tree = (baum_t *)obj;
					file->wr_obj_id( tree->get_desc_id() );

					if (file->is_version_atleast(122, 2)) {
						uint16 age = tree->get_age();
						file->rdwr_short( age );
					}
					else {
						uint32 age = tree->get_age();
						file->rdwr_long( age );
					}
				}
			}
		}
		file->wr_obj_id( -1 );
	}
}


const char *boden_t::get_name() const
{
	if(  ist_uebergang()  ) {
		return "Kreuzung";
	}
	else if(  hat_wege()  ) {
		return get_weg_nr(0)->get_name();
	}
	else {
		return "Boden";
	}
}


void boden_t::calc_image_internal(const bool calc_only_snowline_change)
{
	const slope_t::type slope_this = get_disp_slope();

	const weg_t *const weg = get_weg( road_wt );
	if(  weg  &&  weg->hat_gehweg()  ) {
		// single or double slope
		const uint8 imageid = (!slope_this  ||  is_one_high(slope_this)) ? ground_desc_t::slopetable[slope_this] : ground_desc_t::slopetable[slope_this >> 1] + 12;

		if(  (get_hoehe() >= welt->get_snowline()  ||  welt->get_climate(pos.get_2d()) == arctic_climate)  &&  skinverwaltung_t::fussweg->get_image_id(imageid + 1) != IMG_EMPTY  ) {
			// snow images
			set_image( skinverwaltung_t::fussweg->get_image_id(imageid + 1) );
		}
		else if(  slope_this != 0  &&  get_hoehe() == welt->get_snowline() - 1  &&  skinverwaltung_t::fussweg->get_image_id(imageid + 2) != IMG_EMPTY  ) {
			// transition images
			set_image( skinverwaltung_t::fussweg->get_image_id(imageid + 2) );
		}
		else {
			set_image( skinverwaltung_t::fussweg->get_image_id(imageid) );
		}
	}
	else {
		set_image( ground_desc_t::get_ground_tile(this) );
	}

	if(  !calc_only_snowline_change  ) {
		grund_t::calc_back_image( get_disp_height(), slope_this );
	}
}
