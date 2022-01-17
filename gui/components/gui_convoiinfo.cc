/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Convoi info stats, like loading status bar
 */

#include "gui_convoiinfo.h"
#include "../../simworld.h"
#include "../../vehicle/vehicle.h"
#include "../../simconvoi.h"
#include "../../simcolor.h"
#include "../../simhalt.h"
#include "../../display/simgraph.h"
#include "../../display/viewport.h"
#include "../../player/simplay.h"
#include "../../simline.h"

#include "../../dataobj/translator.h"
#include "../../dataobj/schedule.h"

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
		unsigned count = cnv.is_bound() ? cnv->get_vehicle_count() : 0;
		for(unsigned i=0; i<count; i++) {
			scr_coord_val x, y, w, h;
			const image_id image = cnv->get_vehicle(i)->get_loaded_image();
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

	set_table_layout(2,2);
	set_alignment(ALIGN_LEFT | ALIGN_TOP);


	add_table(1,4);
	{
		add_component(&label_name);

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
		filled_bar.add_color_value(&cnv->get_loading_limit(), color_idx_to_rgb(COL_YELLOW));
		filled_bar.add_color_value(&cnv->get_loading_level(), color_idx_to_rgb(COL_GREEN));
		add_component(&filled_bar);
	}
	end_table();

	add_component( &label_line );

	container_next_halt = add_table(2,1);
	{
		pos_next_halt.init( button_t::posbutton_automatic, "" );
		add_component( &pos_next_halt );
		add_component( &label_next_halt );
	}
	end_table();

	update_label();
}

/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 */
bool gui_convoiinfo_t::infowin_event(const event_t *ev)
{
	if(cnv.is_bound()) {
		// check whether some child must handle this!
		event_t ev2 = *ev;
		ev2.move_origin(container_next_halt->get_pos());

		if( container_next_halt->infowin_event( &ev2 ) ) {
			return true;
		}
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
	label_line.buf().clear();

	if (cnv->in_depot()) {
		label_line.set_visible( false );
		pos_next_halt.set_targetpos3d(cnv->get_home_depot());
		label_next_halt.set_text("(in depot)");
	}
	else {
		label_line.set_visible(true);
		if( cnv->get_line().is_bound() ) {
			label_line.buf().printf( "%s: %s", translator::translate( "Line" ), cnv->get_line()->get_name() );
		}
		else {
			label_line.buf();
			label_line.set_visible( false );
		}
		label_line.update();

		if (!cnv->get_route()->empty()) {
			halthandle_t h;
			const koord3d end = cnv->get_route()->back();

			if(  grund_t *gr = world()->lookup( end )  ) {
				h = gr->get_halt();
				// oil riggs and fish swarms can load anywhere in ther coverage area
				if(  !h.is_bound()  &&  gr->is_water()  &&  cnv->get_schedule()  &&  cnv->get_schedule()->get_waytype()==water_wt   ) {
					planquadrat_t *pl = world()->access( end.get_2d() );
					if(  pl->get_haltlist_count() > 0  ) {
						h = pl->get_haltlist()[0];
					}
				}
			}
			pos_next_halt.set_targetpos3d( end );
			label_next_halt.set_text_pointer(h.is_bound()?h->get_name():translator::translate("wegpunkt"));
		}
	}

	label_name.set_text_pointer(cnv->get_name());
	label_name.set_color(cnv->get_status_color());

	set_size(get_size());
}

/**
 * Draw the component
 */
void gui_convoiinfo_t::draw(scr_coord offset)
{
	update_label();
	gui_aligned_container_t::draw(offset);
}
