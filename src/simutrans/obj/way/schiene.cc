/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../../simconvoi.h"
#include "../../world/simworld.h"
#include "../../vehicle/vehicle.h"

#include "../../dataobj/loadsave.h"
#include "../../dataobj/translator.h"

#include "../../utils/cbuffer.h"

#include "../../descriptor/way_desc.h"
#include "../../builder/wegbauer.h"

#include "schiene.h"

const way_desc_t *schiene_t::default_schiene=NULL;
bool schiene_t::show_reservations = false;


schiene_t::schiene_t() : weg_t()
{
	reserved = convoihandle_t();

	if (schiene_t::default_schiene) {
		set_desc(schiene_t::default_schiene);
	}
	else {
		dbg->fatal("schiene_t::schiene_t()", "No rail way available!");
	}
}


schiene_t::schiene_t(loadsave_t *file) : weg_t()
{
	reserved = convoihandle_t();
	rdwr(file);
}


void schiene_t::cleanup(player_t *)
{
	// removes reservation
	if(reserved.is_bound()) {
		set_ribi(ribi_t::none);
		reserved->suche_neue_route();
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
bool schiene_t::reserve(convoihandle_t c, ribi_t::ribi dir  )
{
	if(can_reserve(c)) {
		reserved = c;
		/* for threeway and fourway switches we may need to alter graphic, if
		 * direction is a diagonal (i.e. on the switching part)
		 * and there are switching graphics
		 */
		if(  ribi_t::is_threeway(get_ribi_unmasked())  &&  ribi_t::is_bend(dir)  &&  get_desc()->has_switch_image()  ) {
			mark_image_dirty( get_image(), 0 );
			mark_image_dirty( get_front_image(), 0 );
			set_images(image_switch, get_ribi_unmasked(), is_snow(), (dir==ribi_t::northeast  ||  dir==ribi_t::southwest) );
			set_flag( obj_t::dirty );
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
* releases previous reservation
* only true, if there was something to release
*/
bool schiene_t::unreserve(convoihandle_t c)
{
	// is this tile reserved by us?
	if(reserved.is_bound()  &&  reserved==c) {
		reserved = convoihandle_t();
		if(schiene_t::show_reservations) {
			set_flag( obj_t::dirty );
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
			int old_max_speed=get_max_speed();
			desc = way_builder_t::get_desc(translator::compatibility_name(bname));
			if(desc==NULL) {
				desc = default_schiene;
				if (!desc) {
					dbg->fatal("schiene_t::rdwr", "Trying to load train tracks but pakset has none!");
				}
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("schiene_t::rdwr()", "Unknown rail '%s' replaced by '%s' (old_max_speed %i)", bname, desc->get_name(), old_max_speed );
		}

		set_desc(desc);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
//		DBG_MESSAGE("schiene_t::rdwr","track %s at (%s) max_speed %i", bname, get_pos().get_str(), old_max_speed);
	}
}


FLAGGED_PIXVAL schiene_t::get_outline_colour() const
{
	if (!show_reservations || !reserved.is_bound()) {
		return 0;
	}

	return TRANSPARENT75_FLAG | OUTLINE_FLAG | color_idx_to_rgb(COL_RED);
}
