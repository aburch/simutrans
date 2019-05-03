/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Convoi info stats, like loading status bar
 */

#include "gui_convoiinfo.h"
#include "../../simworld.h"
#include "../../vehicle/simvehicle.h"
#include "../../simconvoi.h"
#include "../../simcolor.h"
#include "../../display/simgraph.h"
#include "../../display/viewport.h"
#include "../../player/simplay.h"
#include "../../simline.h"

#include "../../dataobj/translator.h"

#include "../../utils/simstring.h"


class gui_convoi_images_t : public gui_component_t
{
	convoihandle_t cnv;
public:
	gui_convoi_images_t(convoihandle_t cnv) { this->cnv = cnv; }

	scr_size get_min_size() const OVERRIDE
	{
		return draw_vehicles( scr_coord(0,0), false);
	}

	scr_size draw_vehicles(scr_coord offset, bool display_images) const
	{
		scr_coord p = offset + get_pos();
		p.y += get_size().h/2;
		// we will use their images offsets and width to shift them to their correct position
		// this should work with any vehicle size ...
		scr_size s(0,0);
		for(unsigned i=0; i<cnv->get_vehicle_count();i++) {
			scr_coord_val x, y, w, h;
			const image_id image = cnv->get_vehikel(i)->get_loaded_image();
			display_get_base_image_offset(image, &x, &y, &w, &h );
			if (display_images) {
				display_base_img(image, p.x + s.w - x, p.y - y - h/2, cnv->get_owner()->get_player_nr(), false, true);
			}
			s.w += (w*2)/3;
			s.h = max(s.h, h);
		}
		return s;
	}

	scr_size get_max_size() const OVERRIDE { return get_min_size(); }

	void draw( scr_coord offset ) OVERRIDE
	{
		draw_vehicles( offset, true);
	}
};


gui_convoiinfo_t::gui_convoiinfo_t(convoihandle_t cnv)
{
	this->cnv = cnv;

	set_table_layout(2,1);
	set_alignment(ALIGN_LEFT | ALIGN_TOP);

	add_table(1,3);
	{
		add_component(&label_name);
		add_component(&label_line);
		add_table(2,1);
		{
			new_component<gui_label_t>("Gewinn");
			add_component(&label_profit);
		}
		end_table();
	}
	end_table();

	add_table(1,2);
	{
		new_component<gui_convoi_images_t>(cnv);
		add_component(&filled_bar);
	}
	end_table();

	filled_bar.add_color_value(&cnv->get_loading_limit(), color_idx_to_rgb(COL_YELLOW));
	filled_bar.add_color_value(&cnv->get_loading_level(), color_idx_to_rgb(COL_GREEN));
	update_label();
}

/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_convoiinfo_t::infowin_event(const event_t *ev)
{
	if(cnv.is_bound()) {
		if(IS_LEFTRELEASE(ev)) {
			cnv->open_info_window();
			return true;
		}
		else if(IS_RIGHTRELEASE(ev)) {
			karte_ptr_t welt;
			welt->get_viewport()->change_world_position(cnv->get_pos());
			return true;
		}
	}
	return false;
}

const char* gui_convoiinfo_t::get_text() const
{
	return cnv->get_name();
}

void gui_convoiinfo_t::update_label()
{
	label_profit.buf().append_money(cnv->get_jahresgewinn() / 100.0);
	label_profit.update();
	label_line.set_visible(true);

	if (cnv->in_depot()) {
		label_line.buf().append(translator::translate("(in depot)"));
	}
	else if (cnv->get_line().is_bound()) {
		label_line.buf().printf("%s %s", translator::translate("Line"), cnv->get_line()->get_name());
	}
	else {
		label_line.buf();
		label_line.set_visible(false);
	}
	label_line.update();

	label_name.buf().append(cnv->get_name());
	label_name.update();
	label_name.set_color(cnv->get_status_color());

	set_size(get_size());
}

/**
 * Draw the component
 * @author Hj. Malthaner
 */
void gui_convoiinfo_t::draw(scr_coord offset)
{
	update_label();
	gui_aligned_container_t::draw(offset);
}
