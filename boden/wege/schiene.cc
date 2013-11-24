/*
 * Schienen für Simutrans
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../../simconvoi.h"
#include "../../simworld.h"
#include "../../vehicle/simvehikel.h"

#include "../../dataobj/loadsave.h"
#include "../../dataobj/translator.h"

#include "../../utils/cbuffer_t.h"

#include "../../besch/weg_besch.h"
#include "../../bauer/wegbauer.h"

#include "schiene.h"

const weg_besch_t *schiene_t::default_schiene=NULL;
bool schiene_t::show_reservations = false;


schiene_t::schiene_t() : weg_t()
{
	reserved = convoihandle_t();
	set_besch(schiene_t::default_schiene);
}


schiene_t::schiene_t(loadsave_t *file) : weg_t()
{
	reserved = convoihandle_t();
	rdwr(file);
}


void schiene_t::entferne(spieler_t *)
{
	// removes reservation
	if(reserved.is_bound()) {
		set_ribi(ribi_t::keine);
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
		reserved->zeige_info();
#endif
	}
}


/**
 * true, if this rail can be reserved
 * @author prissi
 */
bool schiene_t::reserve(convoihandle_t c, ribi_t::ribi dir  )
{
	if(can_reserve(c)) {
		reserved = c;
		/* for threeway and forway switches we may need to alter graphic, if
		 * direction is a diagonal (i.e. on the switching part)
		 * and there are switching graphics
		 */
		if(  ribi_t::is_threeway(get_ribi_unmasked())  &&  ribi_t::ist_kurve(dir)  &&  get_besch()->has_switch_bild()  ) {
			mark_image_dirty( get_bild(), 0 );
			mark_image_dirty( get_after_bild(), 0 );
			set_images(image_switch, get_ribi_unmasked(), is_snow(), (dir==ribi_t::nordost  ||  dir==ribi_t::suedwest) );
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
* @author prissi
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




/**
* releases previous reservation
* @author prissi
*/
bool schiene_t::unreserve(vehikel_t *)
{
	// is this tile empty?
	if(!reserved.is_bound()) {
		return true;
	}
//	if(!welt->lookup(get_pos())->suche_obj(v->get_typ())) {
		reserved = convoihandle_t();
		if(schiene_t::show_reservations) {
			set_flag( obj_t::dirty );
		}
		return true;
//	}
//	return false;
}


void schiene_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "schiene_t" );

	weg_t::rdwr(file);

	if(file->get_version()<99008) {
		sint32 blocknr=-1;
		file->rdwr_long(blocknr);
	}

	if(file->get_version()<89000) {
		uint8 dummy;
		file->rdwr_byte(dummy);
		set_electrify(dummy);
	}

	if(file->is_saving()) {
		const char *s = get_besch()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));

		int old_max_speed=get_max_speed();
		const weg_besch_t *besch = wegbauer_t::get_besch(bname);
		if(besch==NULL) {
			int old_max_speed=get_max_speed();
			besch = wegbauer_t::get_besch(translator::compatibility_name(bname));
			if(besch==NULL) {
				besch = default_schiene;
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("schiene_t::rdwr()", "Unknown rail %s replaced by %s (old_max_speed %i)", bname, besch->get_name(), old_max_speed );
		}
		set_besch(besch);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
		//DBG_MESSAGE("schiene_t::rdwr","track %s at (%i,%i) max_speed %i",bname,get_pos().x,get_pos().y,old_max_speed);
	}
}
