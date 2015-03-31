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


signal_t::signal_t( loadsave_t *file) :
#ifdef INLINE_OBJ_TYPE
	roadsign_t(obj_t::signal, file)
#else
	roadsign_t(file)
#endif
{
	if(besch==NULL) {
		besch = roadsign_t::default_signal;
	}
	if(besch->is_pre_signal())
	{
		// Distant signals do not display a danger aspect.
		state = caution;
	}
	else
	{
		state = danger;
	}
}

signal_t::signal_t(player_t *player, koord3d pos, ribi_t::ribi dir,const roadsign_besch_t *besch, bool preview) : roadsign_t(obj_t::signal, player, pos, dir, besch, preview)
{
	if(besch->is_pre_signal())
	{
		// Distant signals do not display a danger aspect.
		state = caution;
	}
	else
	{
		state = danger;
	}
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * "return a description string for the object , such as the in a
 * Observation window is displayed." (Google)
 * @author Hj. Malthaner
 */
void signal_t::info(cbuffer_t & buf, bool dummy) const
{
	// well, needs to be done
	obj_t::info(buf);

	buf.printf("%s\n%s%u", translator::translate(besch->get_name()), translator::translate("\ndirection:"), get_dir());
}


void signal_t::calc_image()
{
	after_bild = IMG_LEER;
	image_id image = IMG_LEER;
	const bool TEST_pre_signal =  besch->is_pre_signal();
	after_xoffset = 0;
	after_yoffset = 0;
	sint8 xoff = 0, yoff = 0;
	const bool left_swap = welt->get_settings().is_signals_left();
	grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		set_flag(obj_t::dirty);
		const sint8 height_step = TILE_HEIGHT_STEP << hang_t::ist_doppel(gr->get_weg_hang());

		weg_t *sch = gr->get_weg(besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
		if(sch) {
			uint16 offset=0;
			ribi_t::ribi dir = sch->get_ribi_unmasked() & (~calc_mask());
			if(sch->is_electrified()  &&  (besch->get_bild_anzahl()/8)>1) {
				offset = besch->is_pre_signal() ? 12 : 8;
			}

			// vertical offset of the signal positions
			hang_t::typ hang = gr->get_weg_hang();
			if(hang==hang_t::flach) {
				yoff = -gr->get_weg_yoff();
				after_yoffset = yoff;
			}
			else {
				if(  left_swap  ) {
					hang = hang_t::gegenueber(hang);
				}

				// hang_t::(ost, nord, ...) does not work for double slopes, convert to single
                 hang = hang >> hang_t::ist_doppel(hang);

				if(hang==hang_t::ost ||  hang==hang_t::nord) {
					yoff = -height_step;
					after_yoffset = 0;
				}
				else {
					yoff = 0;
					 after_yoffset = -height_step;
				}
			}

			// and now calculate the images:
			// we need to hide the "second" image on tunnel entries
			ribi_t::ribi temp_dir = dir;
			if(  gr->get_typ()==grund_t::tunnelboden  &&  gr->ist_karten_boden()  &&
				(grund_t::underground_mode==grund_t::ugm_none  ||  (grund_t::underground_mode==grund_t::ugm_level  &&  gr->get_hoehe()<grund_t::underground_level))   ) {
				// entering tunnel here: hide the image further in if not undergroud/sliced
				hang = gr->get_grund_hang();
				if(  hang==hang_t::ost  ||  hang==hang_t::nord  ) {
					temp_dir &= ~ribi_t::suedwest;
				}
				else {
					temp_dir &= ~ribi_t::nordost;
				}
			}

			// signs for left side need other offsets and other front/back order
			if(  left_swap  ) {
				const sint16 XOFF = 24;
				const sint16 YOFF = 12;

				if(temp_dir&ribi_t::ost) {
					image = besch->get_bild_nr(3+state*4+offset);
					xoff += XOFF;
					yoff += -YOFF;
				}

				if(temp_dir&ribi_t::nord) {
					if(image!=IMG_LEER) {
						after_bild = besch->get_bild_nr(0+state*4+offset);
						after_xoffset += -XOFF;
						after_yoffset += -YOFF;
					}
					else {
						image = besch->get_bild_nr(0+state*4+offset);
						xoff += -XOFF;
						yoff += -YOFF;
					}
				}

				if(temp_dir&ribi_t::west) {
					after_bild = besch->get_bild_nr(2+state*4+offset);
					after_xoffset += -XOFF;
					after_yoffset += YOFF;
				}

				if(temp_dir&ribi_t::sued) {
					if(after_bild!=IMG_LEER) {
						image = besch->get_bild_nr(1+state*4+offset);
						xoff += XOFF;
						yoff += YOFF;
					}
					else {
						after_bild = besch->get_bild_nr(1+state*4+offset);
						after_xoffset += XOFF;
						after_yoffset += YOFF;
					}
				}
			}
			else {
				if(temp_dir&ribi_t::ost) {
					after_bild = besch->get_bild_nr(3+state*4+offset);
				}

				if(temp_dir&ribi_t::nord) {
					if(after_bild==IMG_LEER) {
						after_bild = besch->get_bild_nr(0+state*4+offset);
					}
					else {
						image = besch->get_bild_nr(0+state*4+offset);
					}
				}

				if(temp_dir&ribi_t::west) {
					image = besch->get_bild_nr(2+state*4+offset);
				}

				if(temp_dir&ribi_t::sued) {
					if(image==IMG_LEER) {
						image = besch->get_bild_nr(1+state*4+offset);
					}
					else {
						after_bild = besch->get_bild_nr(1+state*4+offset);
					}
				}
			}
		}
	}
	set_xoff( xoff );
	set_yoff( yoff );
	set_bild(image);
}
