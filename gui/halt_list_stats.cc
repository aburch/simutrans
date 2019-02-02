/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "halt_list_stats.h"
#include "../simhalt.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../player/simplay.h"
#include "../simworld.h"
#include "../display/simimg.h"

#include "../dataobj/translator.h"

#include "../descriptor/skin_desc.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "gui_frame.h"
#include "halt_info.h" // gui_halt_type_images_t


static karte_ptr_t welt;
/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool halt_list_stats_t::infowin_event(const event_t *ev)
{
	if(halt.is_bound()) {
		if(IS_LEFTRELEASE(ev)) {
			if (event_get_last_control_shift() != 2) {
				halt->open_info_window();
			}
			return true;
		}
		if(IS_RIGHTRELEASE(ev)) {
			welt->get_viewport()->change_world_position(halt->get_basis_pos3d());
			return true;
		}
	}
	return false;
}


halt_list_stats_t::halt_list_stats_t(halthandle_t h)
{
	halt = h;
	set_table_layout(2,2);

	add_component(&indicator);
	indicator.set_max_size(scr_size(D_INDICATOR_WIDTH,D_INDICATOR_HEIGHT));

	add_table(2,1);
	{
		add_component(&label_name);
		label_name.buf().append(halt->get_name());
		label_name.update();

		img_types = new_component<gui_halt_type_images_t>(halt);
	}
	end_table();

	gui_aligned_container_t *table = add_table(3,1);
	{
		add_component(&img_enabled[0]);
		img_enabled[0].set_image(skinverwaltung_t::passengers->get_image_id(0), true);
		add_component(&img_enabled[1]);
		img_enabled[1].set_image(skinverwaltung_t::mail->get_image_id(0), true);
		add_component(&img_enabled[2]);
		img_enabled[2].set_image(skinverwaltung_t::goods->get_image_id(0), true);

		img_enabled[0].set_rigid(true);
		img_enabled[1].set_rigid(true);
		img_enabled[2].set_rigid(true);
	}
	end_table();
	indicator.set_max_size(scr_size(table->get_min_size().w,D_INDICATOR_HEIGHT));

	add_component(&label_cargo);
	halt->get_short_freight_info( label_cargo.buf() );
	label_cargo.update();
}


const char* halt_list_stats_t::get_text() const
{
	return halt->get_name();
}

/**
 * Draw the component
 * @author Markus Weber
 */
void halt_list_stats_t::draw(scr_coord offset)
{
	indicator.set_color(halt->get_status_farbe());
	img_enabled[0].set_visible(halt->get_pax_enabled());
	img_enabled[1].set_visible(halt->get_mail_enabled());
	img_enabled[2].set_visible(halt->get_ware_enabled());

	label_name.buf().append(halt->get_name());
	label_name.update();

	halt->get_short_freight_info( label_cargo.buf() );
	label_cargo.update();

	set_size(get_size());
	gui_aligned_container_t::draw(offset);
}
