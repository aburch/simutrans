/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simworld.h"
#include "../display/simimg.h"
#include "../display/viewport.h"
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

#include "../utils/simstring.h"

#include "halt_detail.h"


halt_detail_t::halt_detail_t(halthandle_t halt_) :
	gui_frame_t(halt_->get_name(), halt_->get_owner()),
	halt(halt_),
	scrolly(&cont),
	txt_info(&buf)
{
	cont.add_component(&txt_info);

	// fill buffer with halt detail
	halt_detail_info();
	txt_info.set_pos(scr_coord(D_MARGIN_LEFT,D_MARGIN_TOP));

	scrolly.set_pos(scr_coord(0, 0));
	scrolly.set_show_scroll_x(true);
	add_component(&scrolly);

	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+4+22*(LINESPACE)+D_SCROLLBAR_HEIGHT+2));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+4+3*(LINESPACE)+D_SCROLLBAR_HEIGHT+2));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));

	cached_active_player=NULL;
}



halt_detail_t::~halt_detail_t()
{
	while(!posbuttons.empty()) {
		button_t *b = posbuttons.remove_first();
		cont.remove_component( b );
		delete b;
	}
	while(!linelabels.empty()) {
		gui_label_t *l = linelabels.remove_first();
		cont.remove_component( l );
		delete l;
	}
	while(!linebuttons.empty()) {
		button_t *b = linebuttons.remove_first();
		cont.remove_component( b );
		delete b;
	}
	while(!convoylabels.empty()) {
		gui_label_t *l = convoylabels.remove_first();
		cont.remove_component( l );
		delete l;
	}
	while(!convoybuttons.empty()) {
		button_t *b = convoybuttons.remove_first();
		cont.remove_component( b );
		delete b;
	}
	while(!label_names.empty()) {
		free(label_names.remove_first());
	}
}

void halt_detail_t::halt_detail_info()
{
	if (!halt.is_bound()) {
		return;
	}

	while(!posbuttons.empty()) {
		button_t *b = posbuttons.remove_first();
		cont.remove_component( b );
		delete b;
	}
	while(!linelabels.empty()) {
		gui_label_t *l = linelabels.remove_first();
		cont.remove_component( l );
		delete l;
	}
	while(!linebuttons.empty()) {
		button_t *b = linebuttons.remove_first();
		cont.remove_component( b );
		delete b;
	}
	while(!convoylabels.empty()) {
		gui_label_t *l = convoylabels.remove_first();
		cont.remove_component( l );
		delete l;
	}
	while(!convoybuttons.empty()) {
		button_t *b = convoybuttons.remove_first();
		cont.remove_component( b );
		delete b;
	}
	while(!label_names.empty()) {
		free(label_names.remove_first());
	}
	buf.clear();

	const slist_tpl<fabrik_t *> & fab_list = halt->get_fab_list();
	slist_tpl<const goods_desc_t *> nimmt_an;

	sint16 offset_y = LINESPACE;

	if(halt->get_pax_enabled() || halt->get_mail_enabled())
	{
		buf.append(translator::translate("Transfer time: "));
		char transfer_time_as_clock[32];
		welt->sprintf_time_tenths(transfer_time_as_clock, sizeof(transfer_time_as_clock), (halt->get_transfer_time() ));
		buf.append(transfer_time_as_clock);
		if(!halt->get_ware_enabled())
		{
			buf.append("\n\n");
			offset_y += (D_MARGIN_TOP * 2);
		}
		else
		{
			buf.append("; ");
		}
	}

	if(halt->get_ware_enabled())
	{
		buf.append(translator::translate("Transshipment time: "));
		char transshipment_time_as_clock[32];
		welt->sprintf_time_tenths(transshipment_time_as_clock, sizeof(transshipment_time_as_clock), (halt->get_transshipment_time() ));
		buf.append(transshipment_time_as_clock);
		buf.append("\n\n");
		offset_y += (D_MARGIN_TOP * 2);
	}

	buf.append(translator::translate("Fabrikanschluss"));
	buf.append("\n");
	offset_y += D_MARGIN_TOP;

	if (!fab_list.empty()) {
		FOR(slist_tpl<fabrik_t*>, const fab, fab_list) {
			const koord pos = fab->get_pos().get_2d();

			// target button ...
			button_t *pb = new button_t();
			pb->init( button_t::posbutton, NULL, scr_coord(D_MARGIN_LEFT, offset_y) );
			pb->set_targetpos( pos );
			pb->add_listener( this );
			posbuttons.append( pb );
			cont.add_component( pb );

			buf.printf("   %s (%d, %d)\n", translator::translate(fab->get_name()), pos.x, pos.y);
			offset_y += LINESPACE;

			FOR(array_tpl<ware_production_t>, const& i, fab->get_input()) {
				goods_desc_t const* const ware = i.get_typ();
				if(!nimmt_an.is_contained(ware)) {
					nimmt_an.append(ware);
				}
			}
		}
	}
	else {
		buf.append(" ");
		buf.append(translator::translate("keine"));
		buf.append("\n");
		offset_y += LINESPACE;
	}

	buf.append("\n");
	offset_y += LINESPACE;

	buf.append(translator::translate("Angenommene Waren"));
	buf.append("\n");
	offset_y += LINESPACE;

	if (!nimmt_an.empty()  &&  halt->get_ware_enabled()) {
		for(uint32 i=0; i<goods_manager_t::get_count(); i++) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			if(nimmt_an.is_contained(ware)) {

				buf.append(" - ");
				buf.append(translator::translate(ware->get_name()));
				buf.append("\n");
				offset_y += LINESPACE;
			}
		}
	}
	else {
		buf.append(" ");
		buf.append(translator::translate("keine"));
		buf.append("\n");
		offset_y += LINESPACE;
	}

	// add lines that serve this stop
	buf.append("\n");
	offset_y += LINESPACE;

	buf.append(translator::translate("Lines serving this stop"));
	buf.append("\n");
	offset_y += LINESPACE;

	if(  !halt->registered_lines.empty()  ) {
		for (unsigned int i = 0; i<halt->registered_lines.get_count(); i++) {
			// Line buttons only if owner ...
			if (welt->get_active_player()==halt->registered_lines[i]->get_owner()) {
				button_t *b = new button_t();
				b->init( button_t::posbutton, NULL, scr_coord(D_MARGIN_LEFT, offset_y) );
				b->set_targetpos( koord(-1,i) );
				b->add_listener( this );
				linebuttons.append( b );
				cont.add_component( b );
			}

			// Line labels with color of player
			label_names.append( strdup(halt->registered_lines[i]->get_name()) );
			gui_label_t *l = new gui_label_t( label_names.back(), PLAYER_FLAG|(halt->registered_lines[i]->get_owner()->get_player_color1()+0) );
			l->set_pos( scr_coord(D_MARGIN_LEFT+D_BUTTON_HEIGHT+D_H_SPACE, offset_y) );
			linelabels.append( l );
			cont.add_component( l );
			buf.append("\n");
			offset_y += LINESPACE;
		}
	}
	else {
		buf.append(" ");
		buf.append( translator::translate("keine") );
		buf.append("\n");
		offset_y += LINESPACE;
	}

	// Knightly : add lineless convoys which serve this stop
	buf.append("\n");
	offset_y += LINESPACE;

	buf.append( translator::translate("Lineless convoys serving this stop") );
	buf.append("\n");
	offset_y += LINESPACE;

	if(  !halt->registered_convoys.empty()  ) {
		for(  uint32 i=0;  i<halt->registered_convoys.get_count();  ++i  ) {
			// Convoy buttons
			button_t *b = new button_t();
			b->init( button_t::posbutton, NULL, scr_coord(D_MARGIN_LEFT, offset_y) );
			b->set_targetpos( koord(-2, i) );
			b->add_listener( this );
			convoybuttons.append( b );
			cont.add_component( b );

			// Line labels with color of player
			label_names.append( strdup(halt->registered_convoys[i]->get_name()) );
			gui_label_t *l = new gui_label_t( label_names.back(), PLAYER_FLAG|(halt->registered_convoys[i]->get_owner()->get_player_color1()+0) );
			l->set_pos( scr_coord(D_MARGIN_LEFT+D_BUTTON_HEIGHT+D_H_SPACE, offset_y) );
			convoylabels.append( l );
			cont.add_component( l );
			buf.append("\n");
			offset_y += LINESPACE;
		}
	}
	else {
		buf.append(" ");
		buf.append( translator::translate("keine") );
		buf.append("\n");
		offset_y += LINESPACE;
	}

	buf.append("\n");
	offset_y += LINESPACE;

	buf.append(translator::translate("Direkt erreichbare Haltestellen"));
	buf.append("\n");
	offset_y += LINESPACE;

	bool has_stops = false;

	const uint8 max_classes = max(goods_manager_t::passengers->get_number_of_classes(), goods_manager_t::mail->get_number_of_classes());

	for (uint i=0; i<goods_manager_t::get_max_catg_index(); i++)
	{
		typedef quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*> connexions_map_single_remote;

		// TODO: Add UI to show different connexions for multiple classes
		uint8 g_class = 0;
		if(i == goods_manager_t::INDEX_PAS)
		{
			g_class = goods_manager_t::passengers->get_number_of_classes() - 1;
		}
		else if(i == goods_manager_t::INDEX_MAIL)
		{
			g_class = goods_manager_t::mail->get_number_of_classes() - 1;
		}

		connexions_map_single_remote *connexions = halt->get_connexions(i, g_class, max_classes);

		if(!connexions->empty())
		{
			buf.append("\n");
			offset_y += LINESPACE;
			buf.append(" · ");
			const goods_desc_t* info = goods_manager_t::get_info_catg_index(i);
			// If it is a special freight, we display the name of the good, otherwise the name of the category.
			buf.append( translator::translate(info->get_catg()==0?info->get_name():info->get_catg_name()) );
#if MSG_LEVEL>=4
			if(  halt->is_transfer(i, g_class, max_classes)  ) {
				buf.append("*");
			}
#endif
			buf.append(":\n");
			offset_y += LINESPACE;

			FOR(connexions_map_single_remote, &iter, *connexions)
			{
				halthandle_t a_halt = iter.key;
				haltestelle_t::connexion* cnx = iter.value;
				if(a_halt.is_bound()) 
				{
					has_stops = true;
					buf.append("   ");
					buf.append(a_halt->get_name());
					
					const uint32 tiles_to_halt = shortest_distance(halt->get_next_pos(a_halt->get_basis_pos()), a_halt->get_next_pos(halt->get_basis_pos()));
					const double km_per_tile = welt->get_settings().get_meters_per_tile() / 1000.0;
					const double km_to_halt = (double)tiles_to_halt * km_per_tile;

					// Distance indication
					buf.append(" (");
					char number[10];
					number_to_string(number, km_to_halt, 2);
					buf.append(number);
					buf.append("km)");

					// target button ...
					button_t *pb = new button_t();
					pb->init( button_t::posbutton, NULL, scr_coord(D_MARGIN_LEFT, offset_y) );
					pb->set_targetpos( a_halt->get_basis_pos() );
					pb->add_listener( this );
					posbuttons.append( pb );
					cont.add_component( pb );

					buf.append("\n");
					buf.append("(");
					char travelling_time_as_clock[32];
					welt->sprintf_time_tenths(travelling_time_as_clock, sizeof(travelling_time_as_clock), cnx->journey_time );
					buf.append(travelling_time_as_clock);
					buf.append(translator::translate(" mins. travelling"));
					buf.append(", ");
					if(halt->is_within_walking_distance_of(a_halt) && !cnx->best_convoy.is_bound() && !cnx->best_line.is_bound())
					{
						buf.append(translator::translate("on foot)"));
					}
					else if(cnx->waiting_time > 0)
					{
						char waiting_time_as_clock[32];
						welt->sprintf_time_tenths(waiting_time_as_clock, sizeof(waiting_time_as_clock),  cnx->waiting_time );
						buf.append(waiting_time_as_clock);
						buf.append(translator::translate(" mins. waiting)"));
					}
					else
					{
						// Test for the default waiting value
						buf.append(translator::translate("waiting time unknown)"));
					}
					buf.append("\n\n");

					offset_y += 3 * LINESPACE;
				}
			}
		}
	}

	if (!has_stops) {
		buf.printf(" %s\n", translator::translate("keine"));
	}

	txt_info.recalc_size();
	cont.set_size( txt_info.get_size() );

	// ok, we have now this counter for pending updates
	cached_line_count = halt->registered_lines.get_count();
	cached_convoy_count = halt->registered_convoys.get_count();
}



bool halt_detail_t::action_triggered( gui_action_creator_t *, value_t extra)
{
	if(  extra.i & ~1  ) {
		koord k = *(const koord *)extra.p;
		if(  k.x>=0  ) {
			// goto button pressed
			welt->get_viewport()->change_world_position( koord3d(k,welt->max_hgt(k)) );
		}
		else if(  k.x==-1  ) {
			// Line button pressed.
			uint16 j=k.y;
			if(  j < halt->registered_lines.get_count()  ) {
				linehandle_t line=halt->registered_lines[j];
				player_t *player=welt->get_active_player();
				if(  player==line->get_owner()  ) {
					// Change player => change marked lines
					player->simlinemgmt.show_lineinfo(player,line);
					welt->set_dirty();
				}
			}
		}
		else if(  k.x==-2  ) {
			// Knightly : lineless convoy button pressed
			uint16 j = k.y;
			if(  j<halt->registered_convoys.get_count()  ) {
				convoihandle_t convoy = halt->registered_convoys[j];
				convoy->show_info();
			}
		}
	}
	return true;
}



void halt_detail_t::draw(scr_coord pos, scr_size size)
{
	if(halt.is_bound()) {
		if( cached_active_player != welt->get_active_player()	||  halt->registered_lines.get_count()!=cached_line_count  || 
			halt->registered_convoys.get_count()!=cached_convoy_count  ||
		    welt->get_ticks() - update_time > 10000) {
			// fill buffer with halt detail
			halt_detail_info();
			cached_active_player=welt->get_active_player();
			update_time = welt->get_ticks();
		}
	}
	gui_frame_t::draw( pos, size );
}



void halt_detail_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
	scrolly.set_size(get_client_windowsize()-scrolly.get_pos());
}



halt_detail_t::halt_detail_t():
	gui_frame_t("", NULL),
	scrolly(&cont),
	txt_info(&buf)
{
	// just a dummy
}



void halt_detail_t::rdwr(loadsave_t *file)
{
	koord3d halt_pos;
	scr_size size = get_windowsize();
	sint32 xoff = scrolly.get_scroll_x();
	sint32 yoff = scrolly.get_scroll_y();
	if(  file->is_saving()  ) {
		halt_pos = halt->get_basis_pos3d();
	}
	halt_pos.rdwr( file );
	size.rdwr( file );
	file->rdwr_long( xoff );
	file->rdwr_long( yoff );
	if(  file->is_loading()  ) {
		halt = welt->lookup( halt_pos )->get_halt();
		// now we can open the window ...
		scr_coord const& pos = win_get_pos(this);
		halt_detail_t *w = new halt_detail_t(halt);
		create_win(pos.x, pos.y, w, w_info, magic_halt_detail + halt.get_id());
		w->set_windowsize( size );
		w->scrolly.set_scroll_position( xoff, yoff );
		destroy_win( this );
	}
}
