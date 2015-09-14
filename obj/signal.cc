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
#include "../obj/gebaeude.h"
#include "../simsignalbox.h"
#include "../besch/haus_besch.h"

#include "signal.h"


signal_t::signal_t( loadsave_t *file) :
#ifdef INLINE_OBJ_TYPE
	roadsign_t(obj_t::signal, file)
#else
	roadsign_t(file)
#endif
{
	rdwr_signal(file);
	if(besch==NULL) {
		besch = roadsign_t::default_signal;
	}
}

signal_t::signal_t(player_t *player, koord3d pos, ribi_t::ribi dir,const roadsign_besch_t *besch, koord3d sb, bool preview) : roadsign_t(obj_t::signal, player, pos, dir, besch, preview)
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

	no_junctions_to_next_signal = false;

	if(besch->get_signal_group())
	{
		const grund_t* gr = welt->lookup(sb);
		if(gr)
		{
			gebaeude_t* gb = gr->get_building();
			if(gb && gb->get_tile()->get_besch()->get_utyp() == haus_besch_t::signalbox)
			{
				signalbox_t* sigb = (signalbox_t*)gb;
				signalbox = sb;
				sigb->add_signal(this); 
			}
		}
	}
}

signal_t::~signal_t()
{
	const grund_t* gr = welt->lookup(signalbox);
	if(gr)
	{
		gebaeude_t* gb = gr->get_building();
		if(gb && gb->get_tile()->get_besch()->get_utyp() == haus_besch_t::signalbox)
		{
			signalbox_t* sigb = (signalbox_t*)gb;
			sigb->remove_signal(this); 
		}
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

	buf.append("\n\n");

	buf.append(translator::translate("Controlled from"));
	buf.append(":\n");
	signal_t* sig = (signal_t*)this;
	koord3d sb = sig->get_signalbox();
	if(sb == koord3d::invalid)
	{
		buf.append(translator::translate("keine"));
	}
	else
	{
		const grund_t* gr = welt->lookup(sb);
		if(gr)
		{
			const gebaeude_t* gb = gr->get_building();
			if(gb)
			{
				buf.append(translator::translate(gb->get_name()));
				buf.append(" <");
				buf.append(sb.x);
				buf.append(",");
				buf.append(sb.y);
				buf.append(",");
				buf.append(sb.z);
				buf.append(">"); 
			}
			else
			{
				buf.append(translator::translate("keine"));
			}
		}
		else
		{
			buf.append(translator::translate("keine"));
		}
	}
}


void signal_t::calc_image()
{
	after_bild = IMG_LEER;
	image_id image = IMG_LEER;
	after_xoffset = 0;
	after_yoffset = 0;
	sint8 xoff = 0, yoff = 0;
	const bool left_swap = welt->get_settings().is_signals_left()  &&  besch->get_offset_left();

	grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		const hang_t::typ full_hang = gr->get_weg_hang();
		const sint8 hang_diff = hang_t::max_diff(full_hang);
		const ribi_t::ribi hang_dir = ribi_t::rueckwaerts( ribi_typ(full_hang) );
		set_flag(obj_t::dirty);
		const sint8 height_step = TILE_HEIGHT_STEP << hang_t::ist_doppel(gr->get_weg_hang());

		weg_t *sch = gr->get_weg(besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
		if(sch) 
		{
			uint16 number_of_signal_image_types = besch->get_aspects(); 
			if(besch->get_has_call_on())
			{
				number_of_signal_image_types += 1;
			}
			if(besch->get_has_selective_choose())
			{
				number_of_signal_image_types += besch->get_aspects() - 1;
			}
			uint16 offset = 0;
			ribi_t::ribi dir = sch->get_ribi_unmasked() & (~calc_mask());
			if(sch->is_electrified()  &&  besch->get_bild_anzahl() >= number_of_signal_image_types * 8) // 8: Four directions per aspect * 2 types (electrified and non-electrified) per aspect
			{
				offset = number_of_signal_image_types * 4;
			}

			// vertical offset of the signal positions
			if(full_hang==hang_t::flach) {
				yoff = -gr->get_weg_yoff();
				after_yoffset = yoff;
			}
			else {
				const ribi_t::ribi test_hang = left_swap ? ribi_t::rueckwaerts(hang_dir) : hang_dir;
				if(test_hang==ribi_t::ost ||  test_hang==ribi_t::nord) {
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
				const ribi_t::ribi tunnel_hang_dir = ribi_t::rueckwaerts( ribi_typ(gr->get_grund_hang()) );
				if(  tunnel_hang_dir==ribi_t::ost ||  tunnel_hang_dir==ribi_t::nord  ) {
					temp_dir &= ~ribi_t::suedwest;
				}
				else {
					temp_dir &= ~ribi_t::nordost;
				}
			}

			uint8 modified_state = state;
			const sint8 diff = 5 - besch->get_aspects(); 
			if(besch->get_has_call_on())
			{
				if(besch->get_has_selective_choose())
				{
					if(besch->get_aspects() < 5)
					{
						if(state > advance_caution)
						{
							modified_state = state - diff;
						}
					}
				}
				else
				{
					if(state > advance_caution)
					{
						modified_state -= (besch->get_aspects() - 1) + diff;
					}
				}
			}

			if(state == call_on && !besch->get_has_call_on())
			{
				modified_state = danger;
			}

			if(state == clear_no_choose && !besch->get_has_selective_choose())
			{
				modified_state = clear;
			}

			if(state == caution_no_choose && !besch->get_has_selective_choose())
			{
				modified_state = caution;
			}

			if(state == preliminary_caution_no_choose && !besch->get_has_selective_choose())
			{
				modified_state = preliminary_caution;
			}

			if(state == advance_caution_no_choose && !besch->get_has_selective_choose())
			{
				modified_state = advance_caution;
			}

			if(besch->is_pre_signal() && besch->get_aspects() == 2 && state == caution)
			{
				modified_state = danger;
			}

			if(besch->get_aspects() == 1)
			{
				modified_state = danger;
			}

			if(besch->get_aspects() == 2 && !besch->is_pre_signal() && !besch->is_choose_sign() && state > clear && !besch->get_has_call_on())
			{
				modified_state = clear;
			}

			if(besch->get_has_selective_choose() && besch->get_aspects() < 5 && state >= clear_no_choose)
			{
				modified_state -= diff; 
			}

			const schiene_t* sch1 = (schiene_t*)sch; 
			const ribi_t::ribi reserved_direction = sch1->get_reserved_direction();
			// signs for left side need other offsets and other front/back order
			if(  left_swap  ) {
				const sint16 XOFF = 2*besch->get_offset_left();
				const sint16 YOFF = besch->get_offset_left();

				if(temp_dir&ribi_t::ost) {
					uint8 direction_state = (reserved_direction & ribi_t::ost) ? modified_state * 4 : 0;
					image = besch->get_bild_nr(3+direction_state+offset);
					xoff += XOFF;
					yoff += -YOFF;
				}

				if(temp_dir&ribi_t::nord) {
					uint8 direction_state = (reserved_direction & ribi_t::nord) ? modified_state * 4 : 0;
					if(image!=IMG_LEER) {			
						after_bild = besch->get_bild_nr(0+direction_state+offset);
						after_xoffset += -XOFF;
						after_yoffset += -YOFF;
					}
					else {
						image = besch->get_bild_nr(0+direction_state+offset);
						xoff += -XOFF;
						yoff += -YOFF;
					}
				}

				if(temp_dir&ribi_t::west) {
					uint8 direction_state = (reserved_direction & ribi_t::west) ? modified_state * 4 : 0;
					after_bild = besch->get_bild_nr(2+direction_state+offset);
					after_xoffset += -XOFF;
					after_yoffset += YOFF;
				}

				if(temp_dir&ribi_t::sued) {
					uint8 direction_state = (reserved_direction & ribi_t::sued) ? modified_state * 4 : 0;
					if(after_bild!=IMG_LEER) {
						image = besch->get_bild_nr(1+direction_state+offset);
						xoff += XOFF;
						yoff += YOFF;
					}
					else {
						after_bild = besch->get_bild_nr(1+direction_state+offset);
						after_xoffset += XOFF;
						after_yoffset += YOFF;
					}
				}
			}
			else {
				if(temp_dir&ribi_t::ost) {
					uint8 direction_state = (reserved_direction & ribi_t::ost) ? modified_state * 4 : 0;
					after_bild = besch->get_bild_nr(3+direction_state+offset);
				}

				if(temp_dir&ribi_t::nord) {
					uint8 direction_state = (reserved_direction & ribi_t::nord) ? modified_state * 4 : 0;
					if(after_bild==IMG_LEER) {
						after_bild = besch->get_bild_nr(0+direction_state+offset);
					}
					else {
						image = besch->get_bild_nr(0+direction_state+offset);
					}
				}

				if(temp_dir&ribi_t::west) {
					uint8 direction_state = (reserved_direction & ribi_t::west) ? modified_state * 4 : 0;
					image = besch->get_bild_nr(2+direction_state+offset);
				}

				if(temp_dir&ribi_t::sued) {
					uint8 direction_state = (reserved_direction & ribi_t::sued) ? modified_state * 4 : 0;
					if(image==IMG_LEER) {
						image = besch->get_bild_nr(1+direction_state+offset);
					}
					else {
						after_bild = besch->get_bild_nr(1+direction_state+offset);
					}
				}
			}
		}
	}
	set_xoff( xoff );
	set_yoff( yoff );
	set_bild(image);
}

void signal_t::rdwr_signal(loadsave_t *file)
{
#ifdef SPECIAL_RESCUE_12_5
	if(file->get_experimental_version() >= 12 && file->is_saving())
#else
	if(file->get_experimental_version() >= 12)
#endif
	{
		signalbox.rdwr(file);

		uint8 state_full = state;
		file->rdwr_byte(state_full); 
		state = state_full;
		
		bool ignore_choose_full = ignore_choose;
		file->rdwr_bool(ignore_choose_full);
		ignore_choose = ignore_choose_full; 
#ifdef SPECIAL_RESCUE_12_6
		if(file->is_saving())
#endif
		// TODO: Enable this
		//file->rdwr_bool(no_junctions_to_next_signal);
	}
}

void signal_t::rotate90()
{
	signalbox.rotate90(welt->get_size().y-1); 
	roadsign_t* rs = (roadsign_t*) this;
	dir = ribi_t::rotate90( dir );
	obj_t::rotate90();
}