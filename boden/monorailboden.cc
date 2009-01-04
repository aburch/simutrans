#include "../simdebug.h"

#include "../gui/ground_info.h"
#include "../tpl/ptrhashtable_tpl.h"

#include "../simimg.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../besch/grund_besch.h"

#include "../dataobj/loadsave.h"
#include "monorailboden.h"


monorailboden_t::monorailboden_t(karte_t *welt, koord3d pos,hang_t::typ slope) : grund_t(welt, pos)
{
	this->slope = slope;
}


void
monorailboden_t::rdwr(loadsave_t *file)
{
	grund_t::rdwr(file);

	if(file->get_version()<88009) {
		// save slope locally
		if(file->get_version()>88005) {
			uint8 sl;
			file->rdwr_byte( sl, " " );
			slope = sl;
		}
		else {
			slope = grund_t::get_grund_hang();
		}
	}
}


void monorailboden_t::calc_bild_internal()
{
	set_bild( IMG_LEER );
	clear_back_bild();
	if(get_weg_nr(0)) {
		if(grund_t::underground_mode) {
			get_weg_nr(0)->set_bild(IMG_LEER);
		}
		else {
			get_weg_nr(0)->calc_bild();
		}
	}
}
