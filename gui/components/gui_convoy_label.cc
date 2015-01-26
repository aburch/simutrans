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
#include "../../display/simgraph.h"
#include "../../convoy.h"
#include "../../bauer/warenbauer.h"
#include "../../dataobj/translator.h"
#include "../../player/simplay.h"
#include "../../vehicle/simvehicle.h"


gui_convoy_label_t::gui_convoy_label_t(convoihandle_t cnv, bool show_number_of_convoys, bool show_max_speed):
	gui_label_t(NULL,COL_BLACK,centered),
	show_number(show_number_of_convoys), show_max_speed(show_max_speed), cnv(cnv)
{
}


scr_size gui_convoy_label_t::get_image_size() const
{
	scr_coord_val tamx=0;
	scr_coord_val tamy=0;
	if (cnv.is_bound() && cnv->get_vehikel_anzahl()>0) {
		for(unsigned i=0; i<cnv->get_vehikel_anzahl();i++) {
			KOORD_VAL x, y, w, h;
			const image_id image=cnv->get_vehikel(i)->get_base_image();
			display_get_base_image_offset(image, &x, &y, &w, &h );
			tamx += (w*2)/3;
			tamy = max(tamy,h+26);
		}
	}
	return scr_size(tamx,tamy);
}


scr_size gui_convoy_label_t::get_size() const
{
	int tamx=0, tamy=0;
	if (get_text_pointer()!=NULL) {
		tamx=display_calc_proportional_string_len_width(get_text_pointer(), 35535) + 2;
		tamy+=LINESPACE+separation;
	}
	tamy+=(show_number?LINESPACE:0)+(show_max_speed?LINESPACE:0);
	scr_size img_tam=get_image_size();
	return scr_size(max(tamx,img_tam.w),tamy+img_tam.h);
}


void gui_convoy_label_t::draw(scr_coord offset)
{
	if (get_text_pointer()!=NULL) {
		gui_label_t::draw(offset);
		offset += scr_size(0,LINESPACE+separation);
	}
	int left=pos.x+offset.x;
	scr_size tam=get_image_size();
	if (get_align()==centered) {
		left-=tam.w/2;
	} else if (get_align()==right) {
		left-=tam.w;
	}
	if (cnv.is_bound() && cnv->get_vehikel_anzahl()>0) {
		for(unsigned i=0; i<cnv->get_vehikel_anzahl();i++) {
			KOORD_VAL x, y, w, h;
			const image_id image=cnv->get_vehikel(i)->get_base_image();
			display_get_base_image_offset(image, &x, &y, &w, &h );
			display_base_img(image,left-x,pos.y+offset.y+13-y-h/2,cnv->get_owner()->get_player_nr(),false,true);
			left += (w*2)/3;
		}
	}
	offset.y+=get_image_size().h;
	if (show_number || show_max_speed)
	{
		convoi_t &convoy = *cnv.get_rep();			
		char tmp[128];
		if (show_number) {
			sprintf(tmp, "%s %d (%s %i)",
				translator::translate("Fahrzeuge:"), cnv->get_vehikel_anzahl(),
				translator::translate("Station tiles:"), convoy.get_vehicle_summary().tiles);
			display_proportional( offset.x + 4, offset.y , tmp, ALIGN_LEFT, COL_BLACK, true );
			offset.y+=LINESPACE;
		}
		if (show_max_speed) {
			const sint32 min_speed = convoy.calc_max_speed(convoy.get_weight_summary());
			const sint32 max_speed = convoy.calc_max_speed(weight_summary_t(convoy.get_vehicle_summary().weight, convoy.get_current_friction()));
			sprintf(tmp,  min_speed == max_speed ? "%s %d km/h" : "%s %d %s %d km/h", 
				translator::translate("Max. speed:"), min_speed, 
				translator::translate("..."), max_speed );

			display_proportional( offset.x + 4, offset.y , tmp, ALIGN_LEFT, COL_BLACK, true );
			offset.y+=LINESPACE;
		}
	}
}
