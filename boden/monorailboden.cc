
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
    clear_flag(grund_t::is_bridge);
}


monorailboden_t::monorailboden_t(karte_t *welt, koord3d pos) : grund_t(welt, pos)
{
    slope = grund_t::gib_grund_hang();
    clear_flag(grund_t::is_bridge);
}


void
monorailboden_t::rdwr(loadsave_t *file)
{
	grund_t::rdwr(file);

	// save slope locally
	if(file->get_version()>88005) {
		file->rdwr_byte( slope, " " );
	}
	else {
		slope = grund_t::gib_grund_hang();
	}
}

void monorailboden_t::calc_bild()
{
	setze_bild( IMG_LEER );
	clear_back_bild();
	if(gib_weg_nr(0)) {
		ding_t *dt = suche_obj(ding_t::oberleitung);
		if(dt) {
			dt->calc_bild();
		}
		gib_weg_nr(0)->calc_bild(gib_pos());
	}
}


bool
monorailboden_t::zeige_info()
{
	if(gib_halt().is_bound()) {
		gib_halt()->zeige_info();
		return true;
	}
	else {
		if(hat_wege()) {	// if this is true, then all land info is shown
DBG_MESSAGE("monorailboden_t::zeige_info()","with ribis %x",grund_t::gib_weg_ribi_unmasked(weg_t::monorail));
			// there is some info!
			if(!grund_infos->get(this)) {
				grund_infos->put(this, new grund_info_t(welt, this));
			}
			create_win(-1, -1, grund_infos->get(this), w_autodelete);
			return true;
		}
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
