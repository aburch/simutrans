/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simworld.h"
#include "../simimg.h"
#include "../simware.h"
#include "../simfab.h"
#include "../simhalt.h"
#include "../simline.h"
#include "../simconvoi.h"

#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"

#include "../player/simplay.h"

#include "halt_detail.h"


halt_detail_t::halt_detail_t(halthandle_t halt_) :
	gui_frame_t(halt_->get_name(), halt_->get_besitzer()),
	halt(halt_),
	scrolly(&cont),
	txt_info(&buf)
{
	cont.add_komponente(&txt_info);

	// fill buffer with halt detail
	halt_detail_info();
	txt_info.set_pos(koord(D_MARGIN_LEFT,D_MARGIN_TOP));

	scrolly.set_pos(koord(0, 0));
	scrolly.set_show_scroll_x(true);
	add_komponente(&scrolly);

	set_fenstergroesse(koord(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+4+22*(LINESPACE)+button_t::gui_scrollbar_size.y+2));
	set_min_windowsize(koord(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+4+3*(LINESPACE)+button_t::gui_scrollbar_size.y+2));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));

	cached_active_player=NULL;
}



halt_detail_t::~halt_detail_t()
{
	while(!posbuttons.empty()) {
		button_t *b = posbuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
	while(!linelabels.empty()) {
		gui_label_t *l = linelabels.remove_first();
		cont.remove_komponente( l );
		delete l;
	}
	while(!linebuttons.empty()) {
		button_t *b = linebuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
	while(!convoylabels.empty()) {
		gui_label_t *l = convoylabels.remove_first();
		cont.remove_komponente( l );
		delete l;
	}
	while(!convoybuttons.empty()) {
		button_t *b = convoybuttons.remove_first();
		cont.remove_komponente( b );
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
		cont.remove_komponente( b );
		delete b;
	}
	while(!linelabels.empty()) {
		gui_label_t *l = linelabels.remove_first();
		cont.remove_komponente( l );
		delete l;
	}
	while(!linebuttons.empty()) {
		button_t *b = linebuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
	while(!convoylabels.empty()) {
		gui_label_t *l = convoylabels.remove_first();
		cont.remove_komponente( l );
		delete l;
	}
	while(!convoybuttons.empty()) {
		button_t *b = convoybuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
	while(!label_names.empty()) {
		free(label_names.remove_first());
	}
	buf.clear();

	const slist_tpl<fabrik_t *> & fab_list = halt->get_fab_list();
	slist_tpl<const ware_besch_t *> nimmt_an;

	sint16 offset_y = LINESPACE;
	buf.append(translator::translate("Fabrikanschluss"));
	buf.append("\n");
	offset_y += D_MARGIN_TOP;

	if (!fab_list.empty()) {
		FOR(slist_tpl<fabrik_t*>, const fab, fab_list) {
			const koord pos = fab->get_pos().get_2d();

			// target button ...
			button_t *pb = new button_t();
			pb->init( button_t::posbutton, NULL, koord(D_MARGIN_LEFT, offset_y) );
			pb->set_targetpos( pos );
			pb->add_listener( this );
			posbuttons.append( pb );
			cont.add_komponente( pb );

			buf.printf("   %s (%d, %d)\n", translator::translate(fab->get_name()), pos.x, pos.y);
			offset_y += LINESPACE;

			FOR(array_tpl<ware_production_t>, const& i, fab->get_eingang()) {
				ware_besch_t const* const ware = i.get_typ();
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
		for(uint32 i=0; i<warenbauer_t::get_waren_anzahl(); i++) {
			const ware_besch_t *ware = warenbauer_t::get_info(i);
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
			if (halt->get_welt()->get_active_player()==halt->registered_lines[i]->get_besitzer()) {
				button_t *b = new button_t();
				b->init( button_t::posbutton, NULL, koord(D_MARGIN_LEFT, offset_y) );
				b->set_targetpos( koord(-1,i) );
				b->add_listener( this );
				linebuttons.append( b );
				cont.add_komponente( b );
			}

			// Line labels with color of player
			label_names.append( strdup(halt->registered_lines[i]->get_name()) );
			gui_label_t *l = new gui_label_t( label_names.back(), PLAYER_FLAG|(halt->registered_lines[i]->get_besitzer()->get_player_color1()+0) );
			l->set_pos( koord(D_MARGIN_LEFT+D_BUTTON_HEIGHT+D_H_SPACE, offset_y) );
			linelabels.append( l );
			cont.add_komponente( l );
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
			b->init( button_t::posbutton, NULL, koord(D_MARGIN_LEFT, offset_y) );
			b->set_targetpos( koord(-2, i) );
			b->add_listener( this );
			convoybuttons.append( b );
			cont.add_komponente( b );

			// Line labels with color of player
			label_names.append( strdup(halt->registered_convoys[i]->get_name()) );
			gui_label_t *l = new gui_label_t( label_names.back(), PLAYER_FLAG|(halt->registered_convoys[i]->get_besitzer()->get_player_color1()+0) );
			l->set_pos( koord(D_MARGIN_LEFT+D_BUTTON_HEIGHT+D_H_SPACE, offset_y) );
			convoylabels.append( l );
			cont.add_komponente( l );
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

	for (uint i=0; i<warenbauer_t::get_max_catg_index(); i++){
		vector_tpl<haltestelle_t::connection_t> const& connections = halt->get_connections(i);
		if(  !connections.empty()  ) {
			buf.append("\n");
			offset_y += LINESPACE;

			buf.append(" ·");
			const ware_besch_t* info = warenbauer_t::get_info_catg_index(i);
			// If it is a special freight, we display the name of the good, otherwise the name of the category.
			buf.append( translator::translate(info->get_catg()==0?info->get_name():info->get_catg_name()) );
#if DEBUG>=4
			if(  halt->is_transfer(i)  ) {
				buf.append("*");
			}
#endif
			buf.append(":\n");
			offset_y += LINESPACE;

			FOR(vector_tpl<haltestelle_t::connection_t>, const& conn, connections) {
				if(  conn.halt.is_bound()  ) {

					has_stops = true;

					buf.printf("   %s <%u>", conn.halt->get_name(), conn.weight);

					// target button ...
					button_t *pb = new button_t();
					pb->init( button_t::posbutton, NULL, koord(D_MARGIN_LEFT, offset_y) );
					pb->set_targetpos( conn.halt->get_basis_pos() );
					pb->add_listener( this );
					posbuttons.append( pb );
					cont.add_komponente( pb );
				}

				buf.append("\n");
				offset_y += LINESPACE;
			}
		}
	}

	if (!has_stops) {
		buf.printf(" %s\n", translator::translate("keine"));
	}

	txt_info.recalc_size();
	cont.set_groesse( txt_info.get_groesse() );

	// ok, we have now this counter for pending updates
	destination_counter = halt->get_reconnect_counter();
	cached_line_count = halt->registered_lines.get_count();
	cached_convoy_count = halt->registered_convoys.get_count();
}



bool halt_detail_t::action_triggered( gui_action_creator_t *, value_t extra)
{
	if(  extra.i & ~1  ) {
		koord k = *(const koord *)extra.p;
		if(  k.x>=0  ) {
			// goto button pressed
			halt->get_welt()->change_world_position( koord3d(k,halt->get_welt()->max_hgt(k)) );
		}
		else if(  k.x==-1  ) {
			// Line button pressed.
			uint16 j=k.y;
			if(  j < halt->registered_lines.get_count()  ) {
				linehandle_t line=halt->registered_lines[j];
				spieler_t *sp=halt->get_welt()->get_active_player();
				if(  sp==line->get_besitzer()  ) {
					//TODO:
					// Change player => change marked lines
					sp->simlinemgmt.show_lineinfo(sp,line);
					halt->get_welt()->set_dirty();
				}
			}
		}
		else if(  k.x==-2  ) {
			// Knightly : lineless convoy button pressed
			uint16 j = k.y;
			if(  j<halt->registered_convoys.get_count()  ) {
				convoihandle_t convoy = halt->registered_convoys[j];
				convoy->zeige_info();
			}
		}
	}
	return true;
}



void halt_detail_t::zeichnen(koord pos, koord gr)
{
	if(halt.is_bound()) {
		if(  halt->get_reconnect_counter()!=destination_counter  ||  cached_active_player!=halt->get_welt()->get_active_player()
				||  halt->registered_lines.get_count()!=cached_line_count  ||  halt->registered_convoys.get_count()!=cached_convoy_count  ) {
			// fill buffer with halt detail
			halt_detail_info();
			cached_active_player=halt->get_welt()->get_active_player();
		}
	}
	gui_frame_t::zeichnen( pos, gr );
}



void halt_detail_t::set_fenstergroesse(koord groesse)
{
	gui_frame_t::set_fenstergroesse(groesse);
	scrolly.set_groesse(get_client_windowsize()-scrolly.get_pos());
}



halt_detail_t::halt_detail_t(karte_t *):
	gui_frame_t("", NULL),
	scrolly(&cont),
	txt_info(&buf)
{
	// just a dummy
}



void halt_detail_t::rdwr(loadsave_t *file)
{
	koord3d halt_pos;
	koord gr = get_fenstergroesse();
	sint32 xoff = scrolly.get_scroll_x();
	sint32 yoff = scrolly.get_scroll_y();
	if(  file->is_saving()  ) {
		halt_pos = halt->get_basis_pos3d();
	}
	halt_pos.rdwr( file );
	gr.rdwr( file );
	file->rdwr_long( xoff );
	file->rdwr_long( yoff );
	if(  file->is_loading()  ) {
		halt = haltestelle_t::get_welt()->lookup( halt_pos )->get_halt();
		// now we can open the window ...
		koord const& pos = win_get_pos(this);
		halt_detail_t *w = new halt_detail_t(halt);
		create_win(pos.x, pos.y, w, w_info, magic_halt_detail + halt.get_id());
		w->set_fenstergroesse( gr );
		w->scrolly.set_scroll_position( xoff, yoff );
		destroy_win( this );
	}
}
