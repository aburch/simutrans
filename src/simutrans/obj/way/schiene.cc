/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../../simconvoi.h"
#include "../../world/simworld.h"
#include "../../vehicle/vehicle.h"

#include "../../dataobj/environment.h"
#include "../../dataobj/loadsave.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/pakset_manager.h"
#include "../../utils/cbuffer.h"

#include "../../descriptor/way_desc.h"
#include "../../builder/wegbauer.h"
#include "../../player/simplay.h"

#include "schiene.h"

const way_desc_t *schiene_t::default_schiene=NULL;
bool schiene_t::show_reservations = false;


schiene_t::schiene_t() : weg_t()
{
	reserved2 = reserved = convoihandle_t();
	enter = enter2 = 0;

	if (schiene_t::default_schiene) {
		set_desc(schiene_t::default_schiene);
	}
	else {
		dbg->fatal("schiene_t::schiene_t()", "No rail way available!");
	}
}


schiene_t::schiene_t(loadsave_t *file) : weg_t()
{
	reserved2 = reserved = convoihandle_t();
	enter = enter2 = 0;

	rdwr(file);
}


void schiene_t::cleanup(player_t *)
{
	// removes reservation
	set_ribi(ribi_t::none);
	if (reserved.is_bound()) {
		reserved->suche_neue_route();
	}
	if (reserved2.is_bound()) {
		reserved2->suche_neue_route();
	}
}


void schiene_t::info(cbuffer_t & buf) const
{
	weg_t::info(buf);

	if(reserved.is_bound()) {
		const char* reserve_text = translator::translate("\nis reserved by:");
		// ignore linebreak
		if (reserve_text[0] == '\n') {
			reserve_text++;
		}
		buf.append(reserve_text);
		buf.append(reserved->get_name());
		buf.append("\n");
#ifdef DEBUG_PBS
		reserved->open_info_window();
#endif
	}
}


/**
 * true, if this rail can be reserved
 */
bool schiene_t::reserve(convoihandle_t c, uint32 index )
{
	if (is_close_diagonal()) {
		// get entry direction (if index is 0, we have to use next tile to guess)
		ribi_t::ribi from = (index == 0) ? ribi_type(get_pos(), c->get_route()->at(index+1)) : ribi_type(c->get_route()->at(index - 1), get_pos());
		bool use2 = (is_close_diagonal() == 2 && (from & ribi_t::northeast)) || (is_close_diagonal() == 1 && (from & ribi_t::southeast));
		convoihandle_t& r = use2 ? reserved2 : reserved;
		convoihandle_t& r_other = use2 ? reserved : reserved2;
		if (r_other == c) {
			// may happen during rotations that we are rereved on the other track ...
			r_other = convoihandle_t();
			(use2 ? enter : enter2) = 0;
		}
		if (r.is_bound() && r != c) {
			return false;
		}
		r = c;
		(use2 ? enter2 : enter) = from;
		if (schiene_t::show_reservations) {
			set_flag(obj_t::dirty);
		}
		return true;
	}
	else if(can_reserve(c)) {
		reserved = c;
		/* for threeway and fourway switches we may need to alter graphic, if
		 * direction is a diagonal (i.e. on the switching part)
		 * and there are switching graphics
		 */
		if(  index > 1  &&  ribi_t::is_threeway(get_ribi_unmasked())  &&  get_desc()->has_switch_image()  ) {
			route_t const& rt = *c->get_route();
			ribi_t::ribi dir = ribi_type(rt[index - 1], get_pos());
			if (ribi_t::is_bend(dir)) {
				mark_image_dirty(get_image(), 0);
				mark_image_dirty(get_front_image(), 0);
				set_switched((dir == ribi_t::northeast || dir == ribi_t::southwest));
				set_images(image_switch, get_ribi_unmasked(), is_snow(), get_switched());
				set_flag(obj_t::dirty);
			}
		}
		if(schiene_t::show_reservations) {
			set_flag( obj_t::dirty );
		}
		return true;
		
	}
	// reserve anyway ...
	return false;
}


/**
 * releases previous reservation: On normal tiles => release all
 */
bool schiene_t::unreserve(vehicle_t *v)
{
	if (is_close_diagonal()) {
		if (v && v->get_convoi()) {
			return unreserve(v->get_convoi()->self);
		}
	}
	return unreserve(reserved);
}


/**
* releases previous reservation
* only true, if there was something to release
*/
bool schiene_t::unreserve(convoihandle_t c)
{
	// is this tile reserved by us?
	if(reserved==c) {
		reserved = convoihandle_t();
		enter = 0;
		if(schiene_t::show_reservations) {
			set_flag( obj_t::dirty );
		}
		return true;
	}
	if (reserved2 == c) {
		reserved2 = convoihandle_t();
		enter2 = 0;
		if (schiene_t::show_reservations) {
			set_flag(obj_t::dirty);
		}
		return true;
	}
	return false;
}



void schiene_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "schiene_t" );

	weg_t::rdwr(file);

	if(file->is_version_less(99, 8)) {
		sint32 blocknr=-1;
		file->rdwr_long(blocknr);
	}

	if(file->is_version_less(89, 0)) {
		uint8 dummy;
		file->rdwr_byte(dummy);
		set_electrify(dummy);
	}

	if(file->is_saving()) {
		const char *s = get_desc()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));

		int old_max_speed = get_max_speed();
		const way_desc_t *desc = way_builder_t::get_desc(bname);

		if(desc==NULL) {
			desc = way_builder_t::get_desc(translator::compatibility_name(bname));

			if(desc==NULL) {
				desc = default_schiene;
				if (!desc) {
					dbg->fatal("schiene_t::rdwr", "Trying to load train tracks but pakset has none!");
				}
				pakset_manager_t::add_missing_paks( bname, MISSING_WAY );
			}
		}

		set_desc(desc);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
//		DBG_MESSAGE("schiene_t::rdwr","track %s at (%s) max_speed %i", bname, get_pos().get_str(), old_max_speed);
	}
}


void schiene_t::rotate90()
{
	weg_t::rotate90();
	if (is_close_diagonal()) {
		// it is unfourtunately not straight forward to swap the reservations ...
		bool swap = false;
		// bool use2 = (is_close_diagonal() == 2 && (from & ribi_t::northeast)) || (is_close_diagonal() == 1 && (from & ribi_t::southeast));
		if (enter) {
			swap = (is_close_diagonal() == 2 && (enter & ribi_t::northeast)) || (is_close_diagonal() == 1 && (enter & ribi_t::southeast));
		}
		else if (enter2) {
			swap = (is_close_diagonal() == 2 && (enter2 & ribi_t::northeast)) || (is_close_diagonal() == 1 && (enter2 & ribi_t::southeast));
		}
		if (swap) {
			// needs to swap convoi handles
			convoihandle_t tmp = reserved2;
			reserved2 = reserved;
			reserved = tmp;
			ribi_t::ribi t = enter2;
			enter2 = enter;
			enter = t;
		}
	}
}


FLAGGED_PIXVAL schiene_t::get_outline_colour() const
{
	if (env_t::show_single_ways  &&  ribi_t::is_single(get_ribi_unmasked())) {
		return TRANSPARENT75_FLAG | OUTLINE_FLAG | gfx->palette_lookup(COL_YELLOW);
	}
	if (show_reservations  &&  reserved.is_bound()) {
		return TRANSPARENT75_FLAG | OUTLINE_FLAG | gfx->palette_lookup(COL_RED);
	}
	if (show_reservations  &&  is_close_diagonal()  &&  get_desc()->has_close_diagonal_image() && reserved2.is_bound()) {
		return TRANSPARENT75_FLAG | OUTLINE_FLAG | gfx->palette_lookup(COL_RED);
	}
	return 0;
}


void schiene_t::display(int xpos, int ypos CLIP_NUM_DEF) const
{
	if (is_close_diagonal() && !get_desc()->has_close_diagonal_image()) {
		image_id image = get_desc()->get_diagonal_image_id(is_close_diagonal() == 1 ? ribi_t::northwest : ribi_t::southwest, is_snow());
		if (get_owner_nr() != PLAYER_UNOWNED) {
			if (obj_t::show_owner) {
				gfx->draw_blend(image, xpos, ypos, get_owner_nr(), gfx->palette_lookup(get_owner()->get_player_color1() + 2) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, get_flag(dirty)  CLIP_NUM_PAR);
			}
			else {
				gfx->draw_color(image, xpos, ypos, get_owner_nr(), true, get_flag(dirty)  CLIP_NUM_PAR);
			}
		}
		else {
			gfx->draw_normal(image, xpos, ypos, 0, true, get_flag(dirty)  CLIP_NUM_PAR);
		}

		if (show_reservations  &&  reserved2.is_bound()) {
			// transparent outline for second way
			gfx->draw_blend(image, xpos, ypos, get_owner_nr(), TRANSPARENT75_FLAG | OUTLINE_FLAG | gfx->palette_lookup(COL_RED), 0, get_flag(dirty)  CLIP_NUM_PAR);
		}
	}

	obj_t::display(xpos, ypos CLIP_NUM_PAR);
}
