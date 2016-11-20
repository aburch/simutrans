/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simobj.h"
#include "../boden/wege/schiene.h"
#include "../boden/grund.h"
#include "../display/simimg.h"
#include "../dataobj/ribi.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../utils/cbuffer_t.h"

#include "signal.h"


signal_t::signal_t(loadsave_t *file) :
	roadsign_t(file)
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
	obj_t::info(buf);

	buf.printf("%s\n%s%u", translator::translate(besch->get_name()), translator::translate("\ndirection:"), get_dir());
}


void signal_t::calc_image()
{
	after_bild = IMG_LEER;
	image_id bild = IMG_LEER;

	after_xoffset = 0;
	after_yoffset = 0;
	sint8 xoff = 0, yoff = 0;
	const bool left_swap = welt->get_settings().is_signals_left()  &&  besch->get_offset_left();

	grund_t *gr = welt->lookup(get_pos());
	if(gr) {

		const slope_t::type full_hang = gr->get_weg_hang();
		const sint8 hang_diff = slope_t::max_diff(full_hang);
		const ribi_t::ribi hang_dir = ribi_t::backward( ribi_type(full_hang) );

		set_flag(obj_t::dirty);

		weg_t *sch = gr->get_weg(besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
		if(sch) {
			uint16 offset=0;
			ribi_t::ribi dir = sch->get_ribi_unmasked() & (~calc_mask());
			if(sch->is_electrified()  &&  (besch->get_count()/8)>1) {
				offset = besch->is_pre_signal() ? 12 : 8;
			}

			// vertical offset of the signal positions
			if(full_hang==slope_t::flat) {
				yoff = -gr->get_weg_yoff();
				after_yoffset = yoff;
			}
			else {
				const ribi_t::ribi test_hang = left_swap ? ribi_t::backward(hang_dir) : hang_dir;
				if(test_hang==ribi_t::east ||  test_hang==ribi_t::north) {
					yoff = -TILE_HEIGHT_STEP*hang_diff;
					after_yoffset = 0;
				}
				else {
					yoff = 0;
					after_yoffset = -TILE_HEIGHT_STEP*hang_diff;
				}
			}

			// and now calculate the images:
			// we need to hide the "second" image on tunnel entries
			ribi_t::ribi temp_dir = dir;
			if(  gr->get_typ()==grund_t::tunnelboden  &&  gr->ist_karten_boden()  &&
				(grund_t::underground_mode==grund_t::ugm_none  ||  (grund_t::underground_mode==grund_t::ugm_level  &&  gr->get_hoehe()<grund_t::underground_level))   ) {
				// entering tunnel here: hide the image further in if not undergroud/sliced
				const ribi_t::ribi tunnel_hang_dir = ribi_t::backward( ribi_type(gr->get_grund_hang()) );
				if(  tunnel_hang_dir==ribi_t::east ||  tunnel_hang_dir==ribi_t::north  ) {
					temp_dir &= ~ribi_t::southwest;
				}
				else {
					temp_dir &= ~ribi_t::northeast;
				}
			}

			// signs for left side need other offsets and other front/back order
			if(  left_swap  ) {
				const sint16 XOFF = 2*besch->get_offset_left();
				const sint16 YOFF = besch->get_offset_left();

				if(temp_dir&ribi_t::east) {
					bild = besch->get_bild_nr(3+zustand*4+offset);
					xoff += XOFF;
					yoff += -YOFF;
				}

				if(temp_dir&ribi_t::north) {
					if(bild!=IMG_LEER) {
						after_bild = besch->get_bild_nr(0+zustand*4+offset);
						after_xoffset += -XOFF;
						after_yoffset += -YOFF;
					}
					else {
						bild = besch->get_bild_nr(0+zustand*4+offset);
						xoff += -XOFF;
						yoff += -YOFF;
					}
				}

				if(temp_dir&ribi_t::west) {
					after_bild = besch->get_bild_nr(2+zustand*4+offset);
					after_xoffset += -XOFF;
					after_yoffset += YOFF;
				}

				if(temp_dir&ribi_t::south) {
					if(after_bild!=IMG_LEER) {
						bild = besch->get_bild_nr(1+zustand*4+offset);
						xoff += XOFF;
						yoff += YOFF;
					}
					else {
						after_bild = besch->get_bild_nr(1+zustand*4+offset);
						after_xoffset += XOFF;
						after_yoffset += YOFF;
					}
				}
			}
			else {
				if(temp_dir&ribi_t::east) {
					after_bild = besch->get_bild_nr(3+zustand*4+offset);
				}

				if(temp_dir&ribi_t::north) {
					if(after_bild==IMG_LEER) {
						after_bild = besch->get_bild_nr(0+zustand*4+offset);
					}
					else {
						bild = besch->get_bild_nr(0+zustand*4+offset);
					}
				}

				if(temp_dir&ribi_t::west) {
					bild = besch->get_bild_nr(2+zustand*4+offset);
				}

				if(temp_dir&ribi_t::south) {
					if(bild==IMG_LEER) {
						bild = besch->get_bild_nr(1+zustand*4+offset);
					}
					else {
						after_bild = besch->get_bild_nr(1+zustand*4+offset);
					}
				}
			}
		}
	}
	set_xoff( xoff );
	set_yoff( yoff );
	set_bild(bild);
}
