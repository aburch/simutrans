/*
* Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <math.h>

#include "gui_convoy_label.h"

#include "../../simconst.h"
#include "../../simconvoi.h"
#include "../../simgraph.h"
#include "../../convoy.h"
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
	if (show_number || show_max_speed)
	{
		existing_convoy_t convoy(*cnv.get_rep());			
		char tmp[128];
		if (show_number) {
			sprintf(tmp, "%s %d (%s %i)",
				translator::translate("Fahrzeuge:"), cnv->get_vehikel_anzahl(),
				translator::translate("Station tiles:"), convoy.get_vehicle_summary().get_tile_length());
			display_proportional( offset.x + 4, offset.y , tmp, ALIGN_LEFT, COL_BLACK, true );
			offset.y+=LINESPACE;
		}
		if (show_max_speed) {
			const uint32 min_speed = convoy.calc_max_speed(convoy.get_weight_summary());
			const uint32 max_speed = convoy.calc_max_speed(weight_summary_t(convoy.get_vehicle_summary().weight, convoy.get_current_friction()));
			sprintf(tmp,  min_speed == max_speed ? "%s %d km/h" : "%s %d %s %d km/h", 
				translator::translate("Max. speed:"), min_speed, 
				translator::translate("..."), max_speed );

			display_proportional( offset.x + 4, offset.y , tmp, ALIGN_LEFT, COL_BLACK, true );
			offset.y+=LINESPACE;
		}
	}
}
