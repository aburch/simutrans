
#include <string.h>

#include "tunnelboden.h"

#include "../simimg.h"
#include "../simworld.h"
#include "../simplay.h"
#include "../simskin.h"

#include "../bauer/tunnelbauer.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"

#include "../dings/tunnel.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"
#include "../besch/tunnel_besch.h"



tunnelboden_t::tunnelboden_t(karte_t *welt, loadsave_t *file) : boden_t(welt, koord3d(0,0,0),0)
{
	rdwr(file);
}


tunnelboden_t::tunnelboden_t(karte_t *welt, koord3d pos, hang_t::typ hang_typ) : boden_t(welt, pos, hang_typ)
{
}


void tunnelboden_t::calc_bild()
{
	if(!ist_tunnel()) {
		// only here, when undergound_mode is true
		clear_back_bild();
		grund_t::calc_bild();
		setze_bild(skinverwaltung_t::fussweg->gib_bild_nr(0));
		if(ist_karten_boden()) {
			setze_bild(obj_bei(0)->gib_bild());
			((weg_t *)obj_bei(0))->setze_bild(0,IMG_LEER);
		}
	}
	else if(ist_karten_boden()) {
		// calculate the slope of ground
		boden_t::calc_bild();
		set_flag(draw_as_ding);
		if(flags&has_way1) {
			((weg_t *)obj_bei(0))->setze_bild(0,IMG_LEER);
		}
		if(flags&has_way2) {
			((weg_t *)obj_bei(1))->setze_bild(0,IMG_LEER);
		}
	}
	else {
		clear_back_bild();
		setze_bild(IMG_LEER);
		if(flags&has_way1) {
			((weg_t *)obj_bei(0))->setze_bild(0,IMG_LEER);
		}
		if(flags&has_way2) {
			((weg_t *)obj_bei(1))->setze_bild(0,IMG_LEER);
		}
	}
}

void
tunnelboden_t::rdwr(loadsave_t *file)
{
	grund_t::rdwr(file);

	if(file->get_version()<88009) {
		int int_hang = slope;
		file->rdwr_long(int_hang, "\n");
		slope = int_hang;
	}
	if(file->get_version()==99003) {
		char  buf[256];
		const tunnel_besch_t *besch = NULL;
		file->rdwr_str(buf,255);
		if(this->suche_obj(ding_t::tunnel)==NULL) {
			besch = tunnelbauer_t::gib_besch(buf);
			if(besch) {
				obj_add(new tunnel_t(welt, gib_pos(), gib_besitzer(), besch));
			}
		}
	}
}



void * tunnelboden_t::operator new(size_t /*s*/)
{
//	assert(s==sizeof(tunnelboden_t));
	return static_cast<tunnelboden_t *>(freelist_t::gimme_node(sizeof(tunnelboden_t)));
}


void tunnelboden_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(tunnelboden_t),p);
}
