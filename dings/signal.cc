/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../boden/wege/schiene.h"
#include "../boden/grund.h"
#include "../simimg.h"
#include "../dataobj/ribi.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"

#include "signal.h"


signal_t::signal_t( karte_t *welt, loadsave_t *file) :
	roadsign_t(welt,file)
{
	if(besch==NULL) {
		besch = roadsign_t::default_signal;
	}
	zustand = rot;
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void signal_t::info(cbuffer_t & buf) const
{
	// well, needs to be done
	ding_t::info(buf);

	buf.append(translator::translate(besch->get_name()));
	buf.append("\n");

	buf.append(translator::translate("\ndirection:"));
	buf.append(get_dir());
}


void signal_t::calc_bild()
{
	after_bild = IMG_LEER;
	image_id bild = IMG_LEER;

	grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		set_flag(ding_t::dirty);

		weg_t *sch = gr->get_weg(besch->get_wtyp());
		if(sch) {
			uint16 offset=0;
			ribi_t::ribi dir = sch->get_ribi();
			if(sch->is_electrified()  &&  (besch->get_bild_anzahl()/8)>1) {
				offset = besch->is_pre_signal() ? 12 : 8;
			}

			// vertical offset of the signal positions
			hang_t::typ hang = gr->get_weg_hang();
			if(hang==hang_t::flach) {
				set_yoff( -gr->get_weg_yoff() );
				after_offset = 0;
			}
			else {
				if(hang==hang_t::west ||  hang==hang_t::sued) {
					set_yoff( 0 );
					after_offset = -TILE_HEIGHT_STEP;
				}
				else {
					set_yoff( -TILE_HEIGHT_STEP );
					after_offset = +TILE_HEIGHT_STEP;
				}
			}

			// and now calculate the images
			if(dir&ribi_t::ost) {
				after_bild = besch->get_bild_nr(3+zustand*4+offset);
			}

			if(dir&ribi_t::nord) {
				if(after_bild==IMG_LEER) {
					after_bild = besch->get_bild_nr(0+zustand*4+offset);
				}
				else {
					bild = besch->get_bild_nr(0+zustand*4+offset);
				}
			}

			if(dir&ribi_t::west) {
				bild = besch->get_bild_nr(2+zustand*4+offset);
			}

			if(dir&ribi_t::sued) {
				if(bild==IMG_LEER) {
					bild = besch->get_bild_nr(1+zustand*4+offset);
				}
				else {
					after_bild = besch->get_bild_nr(1+zustand*4+offset);
				}
			}
		}
	}
	set_bild(bild);
}
