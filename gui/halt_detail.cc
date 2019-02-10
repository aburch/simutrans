/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simworld.h"
#include "../simware.h"
#include "../simfab.h"
#include "../simhalt.h"
#include "../simline.h"
#include "../simconvoi.h"

#include "../descriptor/goods_desc.h"
#include "../bauer/goods_manager.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"

#include "../player/simplay.h"

#include "halt_detail.h"
#include "components/gui_label.h"

/**
 * Button to open line window
 */
class gui_line_button_t : public button_t, public action_listener_t
{
	static karte_ptr_t welt;
	linehandle_t line;
public:
	gui_line_button_t(linehandle_t line) : button_t()
	{
		this->line = line;
		init(button_t::posbutton, NULL);
		add_listener(this);
	}

	bool action_triggered(gui_action_creator_t*, value_t)
	{
		player_t *player = welt->get_active_player();
		if(  player == line->get_owner()  ) {
			player->simlinemgmt.show_lineinfo(player, line);
		}
		return true;
	}

	void draw(scr_coord offset)
	{
		if (line->get_owner() == welt->get_active_player()) {
			button_t::draw(offset);
		}
	}
};

karte_ptr_t gui_line_button_t::welt;

/**
 * Button to open convoi window
 */
class gui_convoi_button_t : public button_t, public action_listener_t
{
	convoihandle_t convoi;
public:
	gui_convoi_button_t(convoihandle_t convoi) : button_t()
	{
		this->convoi = convoi;
		init(button_t::posbutton, NULL);
		add_listener(this);
	}

	bool action_triggered(gui_action_creator_t*, value_t)
	{
		convoi->open_info_window();
		return true;
	}
};


halt_detail_t::halt_detail_t(halthandle_t halt_) :
	gui_frame_t("", NULL),
	halt(halt_),
	scrolly(&cont)
{
	set_table_layout(1,0);

	if (halt.is_bound()) {
		init(halt);
	}
}


void halt_detail_t::init(halthandle_t halt_)
{
	halt = halt_;
	set_name(halt->get_name());
	set_owner(halt->get_owner());

	// fill buffer with halt detail
	halt_detail_info();

	scrolly.set_show_scroll_x(true);
	add_component(&scrolly);

	set_resizemode(diagonal_resize);
	reset_min_windowsize();

	set_windowsize( scr_size(4*D_BUTTON_WIDTH, 10*LINESPACE) );
}


bool compare_connection(haltestelle_t::connection_t const& a, haltestelle_t::connection_t const& b)
{
	return strcmp(a.halt->get_name(), b.halt->get_name()) <=0;
}


// insert empty row between blocks, size 1st column
void halt_detail_t::insert_empty_row()
{
	cont.new_component<gui_label_t>("     ");
	cont.new_component<gui_empty_t>();
}


// insert label "nothing"
void halt_detail_t::insert_show_nothing()
{
	cont.new_component<gui_empty_t>();
	cont.new_component<gui_label_t>("keine");
}


void halt_detail_t::halt_detail_info()
{
	if (!halt.is_bound()) {
		return;
	}

	cont.remove_all();

	const slist_tpl<fabrik_t *> & fab_list = halt->get_fab_list();
	slist_tpl<const goods_desc_t *> nimmt_an;

	cont.set_table_layout(2,0);
	cont.new_component_span<gui_label_t>("Fabrikanschluss", 2);

	if (!fab_list.empty()) {
		FOR(slist_tpl<fabrik_t*>, const fab, fab_list) {
			const koord pos = fab->get_pos().get_2d();

			// target button ...
			button_t *pb = cont.new_component<button_t>();
			pb->init( button_t::posbutton_automatic, NULL);
			pb->set_targetpos( pos );

			// .. name
			gui_label_buf_t *lb = cont.new_component<gui_label_buf_t>();
			lb->buf().printf("%s (%d, %d)\n", translator::translate(fab->get_name()), pos.x, pos.y);
			lb->update();

			FOR(array_tpl<ware_production_t>, const& i, fab->get_input()) {
				goods_desc_t const* const ware = i.get_typ();
				if(!nimmt_an.is_contained(ware)) {
					nimmt_an.append(ware);
				}
			}
		}
	}
	else {
		insert_show_nothing();
	}

	insert_empty_row();
	cont.new_component_span<gui_label_t>("Angenommene Waren", 2);

	if (!nimmt_an.empty()  &&  halt->get_ware_enabled()) {
		for(uint32 i=0; i<goods_manager_t::get_count(); i++) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			if(nimmt_an.is_contained(ware)) {

				cont.new_component<gui_empty_t>();
				cont.new_component<gui_label_t>(ware->get_name());
			}
		}
	}
	else {
		insert_show_nothing();
	}

	insert_empty_row();
	// add lines that serve this stop
	cont.new_component_span<gui_label_t>("Lines serving this stop", 2);

	if(  !halt->registered_lines.empty()  ) {
		for (unsigned int i = 0; i<halt->registered_lines.get_count(); i++) {

			linehandle_t line = halt->registered_lines[i];
			cont.new_component<gui_line_button_t>(line);

			// Line labels with color of player
			gui_label_buf_t *lb = cont.new_component<gui_label_buf_t>(PLAYER_FLAG | color_idx_to_rgb(line->get_owner()->get_player_color1()) );
			lb->buf().append( line->get_name() );
			lb->update();
		}
	}
	else {
		insert_show_nothing();
	}

	insert_empty_row();
	// Knightly : add lineless convoys which serve this stop
	cont.new_component_span<gui_label_t>("Lineless convoys serving this stop", 2);
	if(  !halt->registered_convoys.empty()  ) {
		for(  uint32 i=0;  i<halt->registered_convoys.get_count();  ++i  ) {

			convoihandle_t cnv = halt->registered_convoys[i];
			// Convoy buttons
			cont.new_component<gui_convoi_button_t>(cnv);

			// Line labels with color of player
			gui_label_buf_t *lb = cont.new_component<gui_label_buf_t>(PLAYER_FLAG | color_idx_to_rgb(cnv->get_owner()->get_player_color1()) );
			lb->buf().append( cnv->get_name() );
			lb->update();
		}
	}
	else {
		insert_show_nothing();
	}

	insert_empty_row();
	cont.new_component_span<gui_label_t>("Direkt erreichbare Haltestellen", 2);

	bool has_stops = false;

	for (uint i=0; i<goods_manager_t::get_max_catg_index(); i++){
		vector_tpl<haltestelle_t::connection_t> const& connections = halt->get_connections(i);
		if(  !connections.empty()  ) {

			gui_label_buf_t *lb = cont.new_component_span<gui_label_buf_t>(2);
			lb->buf().append(" ·");
			const goods_desc_t* info = goods_manager_t::get_info_catg_index(i);
			// If it is a special freight, we display the name of the good, otherwise the name of the category.
			lb->buf().append(translator::translate(info->get_catg()==0 ? info->get_name() : info->get_catg_name() ) );
#if MSG_LEVEL>=4
			if(  halt->is_transfer(i)  ) {
				lb->buf().append("*");
			}
#endif
			lb->buf().append(":\n");
			lb->update();

			vector_tpl<haltestelle_t::connection_t> sorted;
			FOR(vector_tpl<haltestelle_t::connection_t>, const& conn, connections) {
				if(  conn.halt.is_bound()  ) {
					sorted.insert_unique_ordered(conn, compare_connection);
				}
			}
			FOR(vector_tpl<haltestelle_t::connection_t>, const& conn, sorted) {

				has_stops = true;

				button_t *pb = cont.new_component<button_t>();
				pb->init( button_t::posbutton_automatic, NULL);
				pb->set_targetpos( conn.halt->get_basis_pos() );

				gui_label_buf_t *lb = cont.new_component<gui_label_buf_t>();
				lb->buf().printf("%s <%u>", conn.halt->get_name(), conn.weight);
				lb->update();
			}
		}
	}

	if (!has_stops) {
		insert_show_nothing();
	}

	reset_min_windowsize();

	// ok, we have now this counter for pending updates
	destination_counter = halt->get_reconnect_counter();
	cached_line_count = halt->registered_lines.get_count();
	cached_convoy_count = halt->registered_convoys.get_count();
}


void halt_detail_t::draw(scr_coord pos, scr_size size)
{
	if(halt.is_bound()) {
		if(  halt->get_reconnect_counter()!=destination_counter
				||  halt->registered_lines.get_count()!=cached_line_count  ||  halt->registered_convoys.get_count()!=cached_convoy_count  ) {
			// fill buffer with halt detail
			halt_detail_info();
		}
	}
	gui_frame_t::draw( pos, size );
}


void halt_detail_t::rdwr(loadsave_t *file)
{
	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );
	// halt
	koord3d halt_pos;
	if(  file->is_saving()  ) {
		halt_pos = halt->get_basis_pos3d();
	}
	halt_pos.rdwr( file );
	if(  file->is_loading()  ) {
		halt = welt->lookup( halt_pos )->get_halt();
		if (halt.is_bound()) {
			init(halt);
			reset_min_windowsize();
			set_windowsize(size);
		}
	}
	scrolly.rdwr(file);

	if (!halt.is_bound()) {
		destroy_win( this );
	}
}
