#include <string.h>

#include "tunnelboden.h"

#include "../simimg.h"
#include "../simworld.h"
#include "../player/simplay.h"
#include "../simskin.h"

#include "../bauer/tunnelbauer.h"

#include "../dataobj/loadsave.h"
#include "../dings/tunnel.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"
#include "../besch/tunnel_besch.h"



tunnelboden_t::tunnelboden_t(karte_t *welt, loadsave_t *file) : boden_t(welt, koord3d(0,0,0),0)
{
	rdwr(file);

	// some versions had tunnel without tunnel objects
	if (!find<tunnel_t>()) {
		// then we must spawn it here (a way MUST be always present, or the savegame is completely broken!)
		weg_t *weg=(weg_t *)obj_bei(0);
		obj_add(new tunnel_t(welt, gib_pos(), weg->gib_besitzer(), tunnelbauer_t::find_tunnel( (waytype_t)weg->gib_besch()->gib_wtyp(), 450, 0 ) ) );
		DBG_MESSAGE("tunnelboden_t::tunnelboden_t()","added tunnel to pos (%i,%i,%i)",gib_pos().x, gib_pos().y,gib_pos().z);
	}
}



void
tunnelboden_t::calc_bild_internal()
{
	if(!ist_tunnel()) {
		// only here, when undergound_mode is true
		clear_back_bild();
		if(ist_karten_boden()) {
			setze_bild( IMG_LEER ); // tunnel mound
		}
		else {
			// default tunnel ground images
			setze_bild(skinverwaltung_t::fussweg->gib_bild_nr(0));
		}
	}
	else if(ist_karten_boden()) {
		// calculate the slope of ground
		boden_t::calc_bild_internal();
		set_flag(draw_as_ding);
		koord pos = gib_pos().gib_2d()+koord(gib_grund_hang());
		grund_t *gr = welt->lookup_kartenboden(pos);
		if(gr) {
			gr->calc_bild();
		}

	}
	else {
		clear_back_bild();
		setze_bild(IMG_LEER);
	}
}



void
tunnelboden_t::rdwr(loadsave_t *file)
{
	grund_t::rdwr(file);

	if(file->get_version()<88009) {
		uint32 int_hang = slope;
		file->rdwr_long(int_hang, "\n");
		slope = int_hang;
	}

	// only 99.03 version save the tunnel here
	if(file->get_version()==99003) {
		char  buf[256];
		const tunnel_besch_t *besch = NULL;
		file->rdwr_str(buf,255);
		if (find<tunnel_t>() == NULL) {
			besch = tunnelbauer_t::gib_besch(buf);
			if(besch) {
				obj_add(new tunnel_t(welt, gib_pos(), obj_bei(0)->gib_besitzer(), besch));
			}
		}
	}
}
