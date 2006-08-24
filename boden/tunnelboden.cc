
#include "tunnelboden.h"

#include "../simimg.h"
#include "../simworld.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"

#include "../besch/grund_besch.h"



tunnelboden_t::tunnelboden_t(karte_t *welt, loadsave_t *file) : boden_t(welt, koord3d(0,0,0),0)
{
	rdwr(file);
}


tunnelboden_t::tunnelboden_t(karte_t *welt, koord3d pos, hang_t::typ hang_typ) : boden_t(welt, pos, hang_typ)
{
}


void tunnelboden_t::calc_bild()
{
	if(!ist_im_tunnel()) {
		// calculate the slope of ground
		boden_t::calc_bild();
		set_flag(draw_as_ding);
	}
	else {
		clear_back_bild();
		setze_bild(IMG_LEER);
	}
	if(wege[0]) {
		wege[0]->setze_bild(IMG_LEER);
	}
	if(wege[1]) {
		wege[1]->setze_bild(IMG_LEER);
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
}



void * tunnelboden_t::operator new(size_t /*s*/)
{
	return (tunnelboden_t *)freelist_t::gimme_node(sizeof(tunnelboden_t));
}


void tunnelboden_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(tunnelboden_t),p);
}
