/*
* Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <math.h>

#include "gui_convoy_label.h"

#include "../../simconst.h"
#include "../../simgraph.h"
#include "../../bauer/warenbauer.h"
#include "../../dataobj/translator.h"
#include "../../player/simplay.h"
#include "../../vehicle/simvehikel.h"


gui_convoy_label_t::gui_convoy_label_t(convoihandle_t cnv, bool show_number_of_convoys, bool show_max_speed):
	gui_label_t(NULL,COL_BLACK,centered),
	show_number(show_number_of_convoys), show_max_speed(show_max_speed), cnv(cnv)
{
}


koord gui_convoy_label_t::get_image_size() const
{
	int tamx=0;
	int tamy=0;
	if (cnv.is_bound() && cnv->get_vehikel_anzahl()>0) {
		for(unsigned i=0; i<cnv->get_vehikel_anzahl();i++) {
			KOORD_VAL x, y, w, h;
			const image_id bild=cnv->get_vehikel(i)->get_basis_bild();
			display_get_base_image_offset(bild, &x, &y, &w, &h );
			tamx += (w*2)/3;
			tamy = max(tamy,h+26);
		}
	}
	return koord(tamx,tamy);
}


koord gui_convoy_label_t::get_size() const
{
	int tamx=0, tamy=0;
	if (get_text_pointer()!=NULL) {
		tamx=display_calc_proportional_string_len_width(get_text_pointer(), 35535) + 2;
		tamy+=LINESPACE+separation;
	}
	tamy+=(show_number?LINESPACE:0)+(show_max_speed?LINESPACE:0);
	koord img_tam=get_image_size();
	return koord(max(tamx,img_tam.x),tamy+img_tam.y);
}


void gui_convoy_label_t::zeichnen(koord offset)
{
	if (get_text_pointer()!=NULL) {
		gui_label_t::zeichnen(offset);
		offset=offset+koord(0,LINESPACE+separation);
	}
	int left=pos.x+offset.x;
	koord tam=get_image_size();
	if (get_align()==centered) {
		left-=tam.x/2;
	} else if (get_align()==right) {
		left-=tam.x;
	}
	if (cnv.is_bound() && cnv->get_vehikel_anzahl()>0) {
		for(unsigned i=0; i<cnv->get_vehikel_anzahl();i++) {
			KOORD_VAL x, y, w, h;
			const image_id bild=cnv->get_vehikel(i)->get_basis_bild();
			display_get_base_image_offset(bild, &x, &y, &w, &h );
			display_base_img(bild,left-x,pos.y+offset.y+13-y-h/2,cnv->get_besitzer()->get_player_nr(),false,true);
			left += (w*2)/3;
		}
	}
	offset.y+=get_image_size().y;
	char tmp[128];
	if (show_number) {
		sprintf(tmp, "%s %d (%s %i)",
			translator::translate("Fahrzeuge:"), cnv->get_vehikel_anzahl(),
			translator::translate("Station tiles:"), (cnv->get_length()+TILE_STEPS-1)/TILE_STEPS );
		display_proportional( offset.x + 4, offset.y , tmp, ALIGN_LEFT, COL_BLACK, true );
		offset.y+=LINESPACE;
	}
	if (show_max_speed) {
		uint16 max_speed=0;
		uint16 min_speed=0;
		if(cnv->get_vehikel_anzahl() > 0) {
//			int length=0;
			uint32 total_power=0;
			uint32 total_max_weight=0;
			uint32 total_min_weight=0;
			for( unsigned i=0;  i<cnv->get_vehikel_anzahl();  i++) {
				const vehikel_besch_t *besch = cnv->get_vehikel(i)->get_besch();

//				length += besch->get_length();
				total_power += besch->get_leistung()*besch->get_gear()/64;
				total_min_weight += besch->get_gewicht();
				total_max_weight += besch->get_gewicht();

				uint32 max_weight =0;
				uint32 min_weight =100000;
				for(uint32 j=0; j<warenbauer_t::get_waren_anzahl(); j++) {
					const ware_besch_t *ware = warenbauer_t::get_info(j);

					if(besch->get_ware()->get_catg_index()==ware->get_catg_index()) {
						max_weight = max(max_weight, ware->get_weight_per_unit());
						min_weight = min(min_weight, ware->get_weight_per_unit());
					}
				}
				total_max_weight += (max_weight*besch->get_zuladung()+499)/1000;
				total_min_weight += (min_weight*besch->get_zuladung()+499)/1000;
			}
			max_speed = min(speed_to_kmh(cnv->get_min_top_speed()), (uint32) sqrt((((double)total_power/total_min_weight)-1)*2500));
			min_speed = min(speed_to_kmh(cnv->get_min_top_speed()), (uint32) sqrt((((double)total_power/total_max_weight)-1)*2500));
		}
		sprintf(tmp,  "%s %d(%d)km/h", translator::translate("Max. speed:"), min_speed, max_speed );
		display_proportional( offset.x + 4, offset.y , tmp, ALIGN_LEFT, COL_BLACK, true );
		offset.y+=LINESPACE;
	}
}