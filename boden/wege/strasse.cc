/*
 * Strassen für Simutrans
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "strasse.h"
#include "../../simworld.h"
#include "../../dataobj/loadsave.h"
#include "../../besch/weg_besch.h"
#include "../../bauer/wegbauer.h"
#include "../../dataobj/translator.h"


const weg_besch_t *strasse_t::default_strasse=NULL;


void strasse_t::set_gehweg(bool janein)
{
	weg_t::set_gehweg(janein);
	if(janein  &&  get_besch()  &&  get_besch()->get_topspeed()>50) {
		set_max_speed(50);
	}
}



strasse_t::strasse_t(loadsave_t *file) : weg_t()
{
	rdwr(file);
}


strasse_t::strasse_t() : weg_t()
{
	set_gehweg(false);
	set_besch(default_strasse);
}



void strasse_t::rdwr(loadsave_t *file)
{
	xml_tag_t s( file, "strasse_t" );

	weg_t::rdwr(file);

	if(file->get_version()<89000) {
		bool gehweg;
		file->rdwr_bool(gehweg);
		set_gehweg(gehweg);
	}

	if(file->is_saving()) {
		const char *s = get_besch()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));

		const weg_besch_t *besch = wegbauer_t::get_besch(bname);
		int old_max_speed = get_max_speed();
		if(besch==NULL) {
			besch = wegbauer_t::get_besch(translator::compatibility_name(bname));
			if(besch==NULL) {
				besch = default_strasse;
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("strasse_t::rdwr()", "Unknown street %s replaced by %s (old_max_speed %i)", bname, besch->get_name(), old_max_speed );
		}
		set_besch(besch);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
		if(besch->get_topspeed()>50  &&  hat_gehweg()) {
			set_max_speed(50);
		}
	}
}
