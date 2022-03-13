/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "fabrik_info.h"

#include "components/gui_label.h"

#include "help_frame.h"
#include "factory_chart.h"

#include "../simfab.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../world/simcity.h"
#include "simwin.h"
#include "../tool/simmenu.h"
#include "../player/simplay.h"
#include "../world/simworld.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../utils/simstring.h"

/**
 * Helper class: one row of entries in consumer / supplier table
 */
class gui_factory_connection_t : public gui_aligned_container_t
{
	const fabrik_t *fab;
	koord target;
	bool supplier;
	gui_image_t* image;

	bool is_active() const
	{
		if (supplier) {
			if(  const fabrik_t *src = fabrik_t::get_fab(target)  ) {
				return src->is_active_lieferziel(fab->get_pos().get_2d());
			}
		}
		else {
			return fab->is_active_lieferziel(target);
		}
		return false;
	}

public:
	gui_factory_connection_t(const fabrik_t* f, koord t, bool s) : fab(f), target(t), supplier(s)
	{
		set_table_layout(3,1);
		button_t* b = new_component<button_t>();
		b->set_typ(button_t::posbutton_automatic);
		b->set_targetpos(target);
		image = new_component<gui_image_t>(skinverwaltung_t::goods->get_image_id(0));
		image->enable_offset_removal(true);

		gui_label_buf_t* l = new_component<gui_label_buf_t>();
		if (const fabrik_t *other = fabrik_t::get_fab(target) ) {
			l->buf().printf("%s (%d,%d)", other->get_name(), target.x, target.y);
			l->update();
		}
	}

	void draw(scr_coord offset) OVERRIDE
	{
		image->set_transparent(is_active() ? 0 : TRANSPARENT50_FLAG);
		gui_aligned_container_t::draw(offset);
	}
};


fabrik_info_t::fabrik_info_t(fabrik_t* fab_, const gebaeude_t* gb) :
	gui_frame_t(""),
	fab(fab_),
	chart(NULL),
	view(scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width() * 7) / 8))),
	prod(&prod_buf),
	txt(&info_buf),
	scroll_info(&container_info)
{
	if (fab) {
		init(fab, gb);
	}
}


void fabrik_info_t::init(fabrik_t* fab_, const gebaeude_t* gb)
{
	fab = fab_;
	// window name
	tstrncpy( fabname, fab->get_name(), lengthof(fabname) );
	gui_frame_t::set_name(fab->get_name());
	set_owner(fab->get_owner());

	set_table_layout(1,0);

	// input name
	input.set_text( fabname, lengthof(fabname) );
	input.add_listener(this);
	add_component(&input);
	highlight_suppliers.set_text("Suppliers");
	highlight_suppliers.set_typ(button_t::roundbox_state);
	highlight_suppliers.add_listener(this);
	highlight_consumers.set_text("Abnehmer");
	highlight_consumers.set_typ(button_t::roundbox_state);
	highlight_consumers.add_listener(this);

	// top part: production number & details, boost indicators, factory view
	add_table(6,0)->set_alignment(ALIGN_LEFT | ALIGN_TOP);
	{
		// production details per input/output
		fab->info_prod( prod_buf );
		prod.recalc_size();
		add_component( &prod );

		new_component<gui_fill_t>();

		// indicator for possible boost by electricity, passengers, mail
		if (fab->get_desc()->get_electric_boost() ) {
			boost_electric.set_image(skinverwaltung_t::electricity->get_image_id(0), true);
			add_component(&boost_electric);

		}
		if (fab->get_desc()->get_pax_boost() ) {
			boost_passenger.set_image(skinverwaltung_t::passengers->get_image_id(0), true);
			add_component(&boost_passenger);
		}
		if (fab->get_desc()->get_mail_boost() ) {
			boost_mail.set_image(skinverwaltung_t::mail->get_image_id(0), true);
			add_component(&boost_mail);
		}

		// world view object with boost overlays
		add_table(1,0)->set_spacing( scr_size(0,0));
		{
			add_component(&view);
			view.set_obj(gb);
			add_component(&indicator_color);
			indicator_color.set_max_size(view.get_max_size());
		}
		end_table();
	}
	end_table();

	// tab panel: connections, chart panels, details
	add_component(&switch_mode);
	switch_mode.add_tab(&scroll_info, translator::translate("Connections"));

	// connection information

	container_info.set_table_layout(1,0);
	container_info.add_component(&all_suppliers);
	container_info.add_component(&all_consumers);
	container_info.add_component(&all_cities);
	container_info.add_component(&txt);
	fab->info_conn(info_buf);

	// initialize to zero, update_info will do the rest
	old_suppliers_count = 0;
	old_consumers_count = 0;
	old_stops_count = 0;
	old_cities_count = 0;

	update_info();

	// take-over chart tabs into our
	chart.set_factory(fab);
	switch_mode.take_tabs(chart.get_tab_panel());

	// factory description in tab
	{
		bool add_tab = false;
		details_buf.clear();

		// factory details
		char key[256];
		sprintf(key, "factory_%s_details", fab->get_desc()->get_name());
		const char * value = translator::translate(key);
		if(value && *value != 'f') {
			details_buf.append(value);
			add_tab = true;
		}

		if (char const* const maker = fab->get_desc()->get_copyright()) {
			details_buf.append("<p>");
			details_buf.printf(translator::translate("Constructed by %s"), maker);
			add_tab = true;
		}

		if (add_tab) {
			switch_mode.add_tab(&container_details, translator::translate("Details"));
			container_details.set_table_layout(1,0);
			gui_flowtext_t* f = container_details.new_component<gui_flowtext_t>();
			f->set_text( (const char*)details_buf);
		}
	}
	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
	set_resizemode(gui_frame_t::diagonal_resize);
}


fabrik_info_t::~fabrik_info_t()
{
	rename_factory();
	fabname[0] = 0;
	if (highlight_consumers.pressed) {
		highlight(fab->get_lieferziele(), false);
	}
	if (highlight_suppliers.pressed) {
		highlight(fab->get_suppliers(), false);
	}
}


void fabrik_info_t::rename_factory()
{
	if(  fabname[0]  &&  welt->get_fab_list().is_contained(fab)  &&  strcmp(fabname, fab->get_name())  ) {
		// text changed and factory still exists => call tool
		cbuffer_t buf;
		buf.printf( "f%s,%s", fab->get_pos().get_str(), fabname );
		tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
		tool->set_default_param( buf );
		welt->set_tool( tool, welt->get_public_player());
		// since init always returns false, it is safe to delete immediately
		delete tool;
	}
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 */
void fabrik_info_t::draw(scr_coord pos, scr_size size)
{
	update_components();

	// boost stuff
	boost_electric.set_transparent(fab->get_prodfactor_electric()>0 ? 0 : TRANSPARENT50_FLAG | OUTLINE_FLAG | color_idx_to_rgb(COL_BLACK));
	boost_passenger.set_transparent(fab->get_prodfactor_pax()>0 ? 0 : TRANSPARENT50_FLAG | OUTLINE_FLAG | color_idx_to_rgb(COL_BLACK));
	boost_mail.set_transparent(fab->get_prodfactor_mail()>0 ? 0 : TRANSPARENT50_FLAG | OUTLINE_FLAG | color_idx_to_rgb(COL_BLACK));

	indicator_color.set_color( color_idx_to_rgb(fabrik_t::status_to_color[fab->get_status()]) );

	chart.update();

	gui_frame_t::draw(pos, size);
}


bool fabrik_info_t::is_weltpos()
{
	return ( welt->get_viewport()->is_on_center( get_weltpos(false) ) );
}


/**
 * This method is called if an action is triggered
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 */
bool fabrik_info_t::action_triggered( gui_action_creator_t *comp, value_t)
{
	if(  comp == &input  ) {
		rename_factory();
	}
	else if (comp == &highlight_consumers)
	{
		highlight_consumers.pressed ^= 1;
		highlight(fab->get_lieferziele(), highlight_consumers.pressed);
	}
	else if (comp == &highlight_suppliers)
	{
		highlight_suppliers.pressed ^= 1;
		highlight(fab->get_suppliers(), highlight_suppliers.pressed);
	}
	return true;
}


void fabrik_info_t::map_rotate90(sint16)
{
	// force update
	old_suppliers_count ++;
	old_consumers_count ++;
	old_stops_count ++;
	old_cities_count ++;
	update_components();
}

// update name and buffers
void fabrik_info_t::update_info()
{
	tstrncpy( fabname, fab->get_name(), lengthof(fabname) );
	gui_frame_t::set_name(fab->get_name());
	input.set_text( fabname, lengthof(fabname) );

	update_components();
}

// update all buffers
void fabrik_info_t::update_components()
{
	// update texts
	fab->info_prod( prod_buf );
	fab->info_conn( info_buf );

	// consumers
	if(  fab->get_lieferziele().get_count() != old_consumers_count ) {
		all_consumers.remove_all();
		all_consumers.set_table_layout(1,0);
		all_consumers.set_margin(scr_size(0,0), scr_size(0,D_V_SPACE));
		all_consumers.add_component(&highlight_consumers);


		for(koord k : fab->get_lieferziele() ) {
			all_consumers.new_component<gui_factory_connection_t>(fab, k, false);
		}
		old_consumers_count = fab->get_lieferziele().get_count();
	}
	// suppliers
	if(  fab->get_suppliers().get_count() != old_suppliers_count ) {
		all_suppliers.remove_all();
		all_suppliers.set_table_layout(1,0);
		all_suppliers.set_margin(scr_size(0,0), scr_size(0,D_V_SPACE));
		all_suppliers.add_component(&highlight_suppliers);

		for(koord k : fab->get_suppliers() ) {
			if(  const fabrik_t *src = fabrik_t::get_fab(k)  ) {
				all_suppliers.new_component<gui_factory_connection_t>(fab, src->get_pos().get_2d(), true);
			}
		}
		old_suppliers_count = fab->get_suppliers().get_count();
	}
	// cities
	if(  fab->get_target_cities().get_count() != old_cities_count  ) {
		all_cities.remove_all();
		all_cities.set_table_layout(6,0);
		all_cities.set_margin(scr_size(0,0), scr_size(0,D_V_SPACE));
		all_cities.new_component_span<gui_label_t>(fab->is_end_consumer() ? "Customers live in:" : "Arbeiter aus:", 6);
		// no new class for entries to get better alignment for columns
		for(stadt_t* const c : fab->get_target_cities()) {

			button_t* b = all_cities.new_component<button_t>();
			b->set_typ(button_t::posbutton_automatic);
			b->set_targetpos(c->get_center());
			// name
			gui_label_buf_t *l = all_cities.new_component<gui_label_buf_t>();
			l->buf().printf("%s", c->get_name());
			l->update();

			stadt_t::factory_entry_t const* const pax_entry  = c->get_target_factories_for_pax().get_entry(fab);
			stadt_t::factory_entry_t const* const mail_entry = c->get_target_factories_for_mail().get_entry(fab);
			assert( pax_entry && mail_entry );
			// passengers
			l = all_cities.new_component<gui_label_buf_t>();
			l->buf().printf("%i", pax_entry->supply);
			l->update();
			l->set_align(gui_label_t::right);
			all_cities.new_component<gui_image_t>(skinverwaltung_t::passengers->get_image_id(0))->enable_offset_removal(true);
			// mail
			l = all_cities.new_component<gui_label_buf_t>();
			l->buf().printf("%i", mail_entry->supply);
			l->update();
			l->set_align(gui_label_t::right);
			all_cities.new_component<gui_image_t>(skinverwaltung_t::mail->get_image_id(0))->enable_offset_removal(true);
		}
		old_cities_count = fab->get_target_cities().get_count();
	}
	container_info.set_size(container_info.get_min_size());
	set_dirty();
}

/***************** Saveload stuff from here *****************/
void fabrik_info_t::rdwr( loadsave_t *file )
{
	// the factory first
	koord fabpos;
	if(  file->is_saving()  ) {
		fabpos = fab->get_pos().get_2d();
	}
	fabpos.rdwr( file );
	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	if(  file->is_loading()  ) {
		fab = fabrik_t::get_fab(fabpos );
		gebaeude_t* gb = welt->lookup_kartenboden( fabpos )->find<gebaeude_t>();

		if (fab != NULL  &&  gb != NULL) {
			init(fab, gb);
		}
		win_set_magic(this, (ptrdiff_t)fab);
	}
	chart.rdwr(file);
	scroll_info.rdwr(file);
	switch_mode.rdwr(file);

	if(  file->is_loading()  ) {
		reset_min_windowsize();
		set_windowsize(size);
	}
}

void fabrik_info_t::highlight(vector_tpl<koord> fab_koords, bool marking)
{
	fab_koords.append(fab->get_pos().get_2d());

	for (uint i = 0; i < fab_koords.get_count(); i++) {
		vector_tpl<grund_t*> fab_tiles;
		if(grund_t *gr = welt->lookup_kartenboden(fab_koords[i])) {
			if(gebaeude_t *gb=gr->find<gebaeude_t>() ) {
				gb->get_tile_list(fab_tiles);
				for(grund_t* gr : fab_tiles ) {
					// no need for check, we just did before ...
					gebaeude_t* gb = gr->find<gebaeude_t>();
					if( marking ) {
						gb->set_flag( obj_t::highlight );
					}
					else {
						gb->clear_flag( obj_t::highlight );
					}
					gr->set_flag( grund_t::dirty );
				}
			}
		}
	}
}
