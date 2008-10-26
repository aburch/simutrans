/*
 * Strassen für Simutrans
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "strasse.h"
#include "../../simdebug.h"
#include "../../simworld.h"
#include "../../simimg.h"
#include "../grund.h"
#include "../../dataobj/loadsave.h"
#include "../../utils/cbuffer_t.h"
#include "../../besch/weg_besch.h"
#include "../../bauer/wegbauer.h"
#include "../../dataobj/translator.h"


const weg_besch_t *strasse_t::default_strasse=NULL;


void strasse_t::setze_gehweg(bool janein)
{
	weg_t::setze_gehweg(janein);
	if(janein  &&  gib_besch()  &&  gib_besch()->gib_topspeed()>50) {
		setze_max_speed(50);
	}
}



strasse_t::strasse_t(karte_t *welt, loadsave_t *file) : weg_t (welt)
{
	rdwr(file);
}



strasse_t::strasse_t(karte_t *welt) : weg_t (welt)
{
	setze_gehweg(false);
	setze_besch(default_strasse);
}



void strasse_t::rdwr(loadsave_t *file)
{
	weg_t::rdwr(file);

	if(file->get_version()<89000) {
		bool gehweg;
		file->rdwr_bool(gehweg, " \n");
		setze_gehweg(gehweg);
	}

	if(file->is_saving()) {
		const char *s = gib_besch()->gib_name();
		file->rdwr_str(s, "\n");
	}
	else {
		char bname[128];
		file->rd_str_into(bname, "\n");

		const weg_besch_t *besch = wegbauer_t::gib_besch(bname);
		int old_max_speed = gib_max_speed();
		if(besch==NULL) {
			besch = wegbauer_t::gib_besch(translator::compatibility_name(bname));
			if(besch==NULL) {
				besch = default_strasse;
			}
			dbg->warning("strasse_t::rdwr()", "Unknown street %s replaced by %s (old_max_speed %i)", bname, besch->gib_name(), old_max_speed );
		}
		setze_besch(besch);
		if(old_max_speed>0) {
			setze_max_speed(old_max_speed);
		}
		if(besch->gib_topspeed()>50  &&  hat_gehweg()) {
			setze_max_speed(50);
		}
	}
}
