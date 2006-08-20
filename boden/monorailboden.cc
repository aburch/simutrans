
#include "../simdebug.h"
#include "monorailboden.h"

#include "../gui/karte.h"
#include "../gui/ground_info.h"

#include "../simimg.h"
#include "../simworld.h"
#include "../simtools.h"
#include "../simhalt.h"
#include "../besch/grund_besch.h"


#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"


monorailboden_t::monorailboden_t(karte_t *welt, loadsave_t *file) : grund_t(welt)
{
    rdwr(file);
    set_flag(grund_t::is_bridge);
}


monorailboden_t::monorailboden_t(karte_t *welt, koord3d pos) : grund_t(welt, pos)
{
    set_flag(grund_t::is_bridge);
}


void
monorailboden_t::rdwr(loadsave_t *file)
{
    grund_t::rdwr(file);
}

void monorailboden_t::calc_bild()
{
	if(hat_wege()) {
		setze_weg_bild(gib_weg(weg_t::schiene)->calc_bild(gib_pos()));
		setze_bild( IMG_LEER );
	}
	else {
		setze_weg_bild( IMG_LEER );
	}
	setze_weg2_bild( IMG_LEER );
}


bool
monorailboden_t::zeige_info()
{
	if(gib_halt().is_bound()) {
		gib_halt()->zeige_info();
		return true;
	}
	else {
		if(!grund_infos->get(this)) {
			grund_infos->put(this, new grund_info_t(welt, this));
		}
		create_win(-1, -1, grund_infos->get(this), w_autodelete);
		return true;
	}
	return false;
}


void * monorailboden_t::operator new(size_t /*s*/)
{
	return (monorailboden_t *)freelist_t::gimme_node(sizeof(monorailboden_t));
}


void monorailboden_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(monorailboden_t),p);
}
