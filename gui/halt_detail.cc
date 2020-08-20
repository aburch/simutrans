/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
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


#define HALT_CAPACITY_BAR_WIDTH 100
#define GOODS_SYMBOL_CELL_WIDTH 14
#define GOODS_WAITING_CELL_WIDTH 60
#define GOODS_WAITING_BAR_BASE_WIDTH 160
#define GOODS_LEAVING_BAR_HEIGHT 2
#define BARCOL_TRANSFER_IN (10)

sint16 halt_detail_t::tabstate = -1;

halt_detail_t::halt_detail_t(halthandle_t halt_) :
	gui_frame_t(halt_->get_name(), halt_->get_owner()),
	halt(halt_),
	scrolly(&cont),
	scrolly_pas(&pas),
	scrolly_goods(&cont_goods),
	pas(halt_),
	goods(halt_),
	nearby_factory(halt_),
	txt_info(&buf),
	line_number(halt_)
{
	if (halt.is_bound()) {
		init(halt);
	}
}


void halt_detail_t::init(halthandle_t halt_)
{
	line_number.set_pos(scr_coord(0, D_V_SPACE));
	add_component(&line_number);

	cont.add_component(&txt_info);
	txt_info.set_pos(scr_coord(D_MARGIN_LEFT,D_MARGIN_TOP));

	tabs.set_pos(scr_coord(0, LINESPACE*4 + D_MARGIN_TOP + D_V_SPACE));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT + tabs.get_pos().y + D_TAB_HEADER_HEIGHT));

	old_factory_count = 0;
	old_catg_count = 0;
	cached_active_player=NULL;
	cashed_size_y = 0;
	show_pas_info = false;
	show_freight_info = false;

	scrolly.set_pos(scr_coord(0, 0));
	scrolly.set_show_scroll_x(true);

	lb_nearby_factory.set_text(translator::translate("Fabrikanschluss")); // (en)Connected industries

	// fill buffer with halt detail
	goods.recalc_size();
	nearby_factory.recalc_size();
	update_components();

	tabs.add_listener(this);
	add_component(&tabs);

	cont_goods.set_pos(scr_coord(0, 0));
	cont_goods.add_component(&goods);
	cont_goods.add_component(&lb_nearby_factory);
	cont_goods.add_component(&nearby_factory);

	// Adjust window to optimal size
	set_tab_opened();

	tabs.set_size(get_client_windowsize() - tabs.get_pos() - scr_size(0, 1));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
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


koord3d halt_detail_t::get_weltpos(bool)
{
	return halt->get_basis_pos3d();
}


bool halt_detail_t::is_weltpos()
{
	return (welt->get_viewport()->is_on_center(get_weltpos(false)));
}

// update all buffers
void halt_detail_t::update_components()
{
	bool reset_tab = false;
	int old_tab = tabs.get_active_tab_index();
	if (!show_pas_info && (halt->get_pax_enabled() || halt->get_mail_enabled())) {
		show_pas_info = true;
		reset_tab = true;
		old_tab++;
	}
	if (!show_freight_info && halt->get_ware_enabled())
	{
		show_freight_info = true;
		reset_tab = true;
		old_tab++;
	}
	if (reset_tab) {
		tabs.clear();
		if (show_pas_info) {
			scrolly_pas.set_size(pas.get_size());
			tabs.add_tab(&scrolly_pas, translator::translate("Passengers/mail"));
		}
		if (show_freight_info) {
			tabs.add_tab(&scrolly_goods, translator::translate("Freight"));
		}
		tabs.add_tab(&scrolly, translator::translate("Services"));
		if (tabstate != -1) {
			tabs.set_active_tab_index(old_tab);
			tabstate = old_tab;
		}
		else {
			tabs.set_active_tab_index(0);
			tabstate = 0;
		}
	}

	// tab3
	if (halt.is_bound()) {
		if (cached_active_player != welt->get_active_player() || halt->registered_lines.get_count() != cached_line_count ||
			halt->registered_convoys.get_count() != cached_convoy_count ||
			welt->get_ticks() - update_time > 10000) {
			// fill buffer with halt detail
			halt_detail_info();
			cached_active_player = welt->get_active_player();
			update_time = welt->get_ticks();
		}
	}

	// tab1
	if (cashed_size_y != pas.get_size().h) {
		cashed_size_y = pas.get_size().h;
		if (tabstate==0) {
			set_tab_opened();
		}
	}

	// tab2
	scr_coord_val y = 0; // calc for layout
	if (goods.active_freight_catg != old_catg_count)
	{
		goods.recalc_size();
		old_catg_count = goods.active_freight_catg;
	}
	y += goods.get_size().h;

	lb_nearby_factory.set_pos(scr_coord(D_H_SPACE, y));
	y += LINESPACE+2;
	nearby_factory.set_pos(scr_coord(0, y));
	if (halt->get_fab_list().get_count() != old_factory_count)
	{
		nearby_factory.recalc_size();
		old_factory_count = halt->get_fab_list().get_count();
	}
	y += nearby_factory.get_size().h;

	if (y != cont_goods.get_size().h) {
		cont_goods.set_size(scr_size(400, y));
	}

	set_dirty();
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

	sint16 offset_x = 0;
	sint16 offset_y = D_MARGIN_TOP;

	// add lines that serve this stop
	buf.append(translator::translate("Lines serving this stop"));
	buf.append("\n");
	offset_y += LINESPACE;

	if(  !halt->registered_lines.empty()  ) {
		for (uint8 lt = 1; lt < simline_t::MAX_LINE_TYPE; lt++) {
			uint waytype_line_cnt = 0;
			for (unsigned int i = 0; i<halt->registered_lines.get_count(); i++) {
				if (halt->registered_lines[i]->get_linetype() != lt) {
					continue;
				}
				int offset_x = D_MARGIN_LEFT;
				if (!waytype_line_cnt) {
					buf.append("\n");
					offset_y += LINESPACE;
					buf.append(" · ");
					buf.append(translator::translate(halt->registered_lines[i]->get_linetype_name()));
					buf.append("\n");
					offset_y += LINESPACE;
				}

				// Line buttons only if owner ...
				if (welt->get_active_player() == halt->registered_lines[i]->get_owner()) {
					button_t *b = new button_t();
					b->init(button_t::posbutton, NULL, scr_coord(offset_x, offset_y));
					b->set_targetpos(koord(-1, i));
					b->add_listener(this);
					linebuttons.append(b);
					cont.add_component(b);
				}
				offset_x += D_BUTTON_HEIGHT+D_H_SPACE;
				// Line labels with color of player
				label_names.append( strdup(halt->registered_lines[i]->get_name()) );
				gui_label_t *l = new gui_label_t( label_names.back(), PLAYER_FLAG|(halt->registered_lines[i]->get_owner()->get_player_color1()+0) );
				l->set_pos( scr_coord(offset_x, offset_y) );
				linelabels.append( l );
				cont.add_component( l );
				buf.append("\n");
				offset_y += LINESPACE;
				waytype_line_cnt++;
			}


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

#if MSG_LEVEL>=4
	const uint8 max_classes = max(goods_manager_t::passengers->get_number_of_classes(), goods_manager_t::mail->get_number_of_classes());
#endif

	for (uint i=0; i<goods_manager_t::get_max_catg_index(); i++)
	{
		typedef quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*> connexions_map_single_remote;

		// TODO: Add UI to show different connexions for multiple classes
		uint8 g_class = goods_manager_t::get_classes_catg_index(i)-1;

		connexions_map_single_remote *connexions = halt->get_connexions(i, g_class);

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



bool halt_detail_t::action_triggered( gui_action_creator_t *comp, value_t extra)
{
	if( comp != &tabs && extra.i & ~1  ) {
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
	else if (tabstate != tabs.get_active_tab_index() || get_windowsize().h == get_min_windowsize().h) {
		set_tab_opened();
	}
	return true;
}


bool halt_detail_t::infowin_event(const event_t *ev)
{
	if (ev->ev_class == EVENT_KEYBOARD && ev->ev_code == SIM_KEY_DOWN) {
		set_tab_opened();
	}
	if (ev->ev_class == EVENT_KEYBOARD && ev->ev_code == SIM_KEY_UP) {
		set_windowsize(get_min_windowsize());
	}

	return gui_frame_t::infowin_event(ev);
}


void halt_detail_t::set_tab_opened()
{
	update_components();
	tabstate = tabs.get_active_tab_index();
	uint selected_tab = tabstate;
	if (!show_pas_info) {
		selected_tab++;
	}
	else if (!show_freight_info && selected_tab) {
		selected_tab++;
	}

	int margin_above_tab = tabs.get_pos().y + D_TAB_HEADER_HEIGHT + D_MARGINS_Y + D_V_SPACE;
	switch (selected_tab)
	{
		case 0: // pas
		default:
			set_windowsize(scr_size(get_windowsize().w, min(display_get_height() - margin_above_tab, margin_above_tab + cashed_size_y)));
			break;
		case 1: // goods
			set_windowsize(scr_size(get_windowsize().w, min(display_get_height() - margin_above_tab, margin_above_tab + cont_goods.get_size().h)));
			break;
		case 2: // info
			set_windowsize(scr_size(get_windowsize().w, min(display_get_height() - margin_above_tab, margin_above_tab + txt_info.get_size().h)));
			break;
	}
}


void halt_detail_t::draw(scr_coord pos, scr_size size)
{
	update_components();
	gui_frame_t::draw( pos, size );

	int yoff = D_TITLEBAR_HEIGHT + D_MARGIN_TOP + LINESPACE;
	int left = D_MARGIN_LEFT;

	image_id symbol;
	uint32 wainting_sum, transship_in_sum, leaving_sum;
	bool is_operating;
	bool overcrowded;
	char transfer_time_as_clock[32];
	const uint8 max_classes = max(goods_manager_t::passengers->get_number_of_classes(), goods_manager_t::mail->get_number_of_classes());
	for (uint8 i=0; i<3; i++) {
		is_operating = false;
		overcrowded = false;
		wainting_sum = 0;
		transship_in_sum = 0;
		leaving_sum = 0;
		switch (i) {
			case 0:
				if (!halt->get_pax_enabled()) {
					continue;
				}
				symbol = skinverwaltung_t::passengers->get_image_id(0);
				wainting_sum = halt->get_ware_summe(goods_manager_t::get_info(goods_manager_t::INDEX_PAS));
				is_operating = halt->gibt_ab(goods_manager_t::get_info(goods_manager_t::INDEX_PAS));
				overcrowded = halt->is_overcrowded(goods_manager_t::INDEX_PAS);
				transship_in_sum = halt->get_transferring_goods_sum(goods_manager_t::get_info(goods_manager_t::INDEX_PAS)) - halt->get_leaving_goods_sum(goods_manager_t::get_info(goods_manager_t::INDEX_PAS));
				break;
			case 1:
				if (!halt->get_mail_enabled()) {
					continue;
				}
				symbol = skinverwaltung_t::mail->get_image_id(0);
				wainting_sum = halt->get_ware_summe(goods_manager_t::get_info(goods_manager_t::INDEX_MAIL));
				is_operating = halt->gibt_ab(goods_manager_t::get_info(goods_manager_t::INDEX_MAIL));
				overcrowded = halt->is_overcrowded(goods_manager_t::INDEX_MAIL);
				transship_in_sum = halt->get_transferring_goods_sum(goods_manager_t::get_info(goods_manager_t::INDEX_MAIL)) - halt->get_leaving_goods_sum(goods_manager_t::get_info(goods_manager_t::INDEX_MAIL));
				break;
			case 2:
				if (!halt->get_ware_enabled()) {
					continue;
				}
				symbol = skinverwaltung_t::goods->get_image_id(0);
				for (uint g1 = 0; g1 < goods_manager_t::get_max_catg_index(); g1++) {
					if (g1 == goods_manager_t::INDEX_PAS || g1 == goods_manager_t::INDEX_MAIL)
					{
						continue;
					}
					const goods_desc_t *wtyp = goods_manager_t::get_info(g1);
					if (!is_operating)
					{
						is_operating = halt->gibt_ab(wtyp);
					}
					switch (g1) {
					case 0:
						wainting_sum += halt->get_ware_summe(wtyp);
						break;
					default:
						const uint8  count = goods_manager_t::get_count();
						for (uint32 g2 = 3; g2 < count; g2++) {
							goods_desc_t const* const wtyp2 = goods_manager_t::get_info(g2);
							if (wtyp2->get_catg_index() != g1) {
								continue;
							}
							wainting_sum += halt->get_ware_summe(wtyp2);
							transship_in_sum += halt->get_transferring_goods_sum(wtyp2, 0);
							leaving_sum += halt->get_leaving_goods_sum(wtyp2, 0);
						}
						break;
					}
				}
				overcrowded = ((wainting_sum + transship_in_sum) > halt->get_capacity(i));
				transship_in_sum -= leaving_sum;
				break;
			default:
				continue;
				//break;
		}
		left = D_MARGIN_LEFT;
		cbuffer_t capacity_buf;
		capacity_buf.clear();
		capacity_buf.printf("%5u", wainting_sum);
		capacity_buf.printf("/%u", halt->get_capacity(i));

		// [capacity type]
		display_color_img(symbol, pos.x + left, pos.y + yoff, 0, false, false);
		left += 13;

		// [capacity indicator]
		// If the capacity is 0 (but hundled this freught type), do not display the bar
		if (halt->get_capacity(i) > 0) {
			display_ddd_box_clip(pos.x + left, pos.y + yoff, HALT_CAPACITY_BAR_WIDTH + 2, 8, MN_GREY0, MN_GREY4);
			display_fillbox_wh_clip(pos.x + left + 1, pos.y + yoff + 1, HALT_CAPACITY_BAR_WIDTH, 6, MN_GREY2, true);
			// transferring (to this station) bar
			display_fillbox_wh_clip(pos.x + left + 1, pos.y + yoff + 1, min(100, (transship_in_sum + wainting_sum) * 100 / halt->get_capacity(i)), 6, MN_GREY1, true);

			const COLOR_VAL col = overcrowded ? COL_OVERCROWD : COL_GREEN;
			uint8 waiting_factor = min(100, wainting_sum * 100 / halt->get_capacity(i));

			display_cylinderbar_wh_clip(pos.x + left + 1, pos.y + yoff + 1, HALT_CAPACITY_BAR_WIDTH * waiting_factor /100, 6, col, true);
		}

		left += HALT_CAPACITY_BAR_WIDTH+2 +D_H_SPACE;

		// [transfer time]
		capacity_buf.append("  ");
		if (i == 2) {
			capacity_buf.append(translator::translate("Transshipment time: "));
			welt->sprintf_time_tenths(transfer_time_as_clock, sizeof(transfer_time_as_clock), (halt->get_transshipment_time()));
		}
		else {
			capacity_buf.append(translator::translate("Transfer time: "));
			welt->sprintf_time_tenths(transfer_time_as_clock, sizeof(transfer_time_as_clock), (halt->get_transfer_time()));
		}
		capacity_buf.append(transfer_time_as_clock);
		left += display_proportional_clip(pos.x + left, pos.y + yoff, capacity_buf, ALIGN_LEFT, is_operating ? SYSCOL_TEXT : MN_GREY0, true) + D_H_SPACE;

		if (!is_operating && skinverwaltung_t::alerts)
		{
			display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(2), pos.x + left, pos.y + yoff, 0, false, false, translator::translate("No service"));
		}

		yoff += LINESPACE;
	}
}



void halt_detail_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
	tabs.set_size(get_client_windowsize() - tabs.get_pos() - scr_size(0, 1));
}



halt_detail_t::halt_detail_t():
	gui_frame_t("", NULL),
	scrolly(&cont),
	scrolly_pas(&pas),
	scrolly_goods(&goods),
	pas(halthandle_t()),
	goods(halthandle_t()),
	nearby_factory(halthandle_t()),
	txt_info(&buf),
	line_number(halthandle_t())

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


halt_detail_pas_t::halt_detail_pas_t(halthandle_t halt)
{
	this->halt = halt;
}

void halt_detail_pas_t::draw_class_table(scr_coord offset, const uint8 class_name_cell_width, const goods_desc_t *good_category)
{
	if (good_category != goods_manager_t::mail && good_category != goods_manager_t::passengers) {
		return; // this table is for pax and mail
	}
	KOORD_VAL y = offset.y;

	int base_capacity = halt->get_capacity(good_category->get_catg_index()) ? max(halt->get_capacity(good_category->get_catg_index()), 10) : 10; // Note that capacity may have 0 even if pax_enabled
	int transferring_sum = 0;
	bool served = false;
	int left = 0;

	display_color_img(good_category == goods_manager_t::mail ? skinverwaltung_t::mail->get_image_id(0) : skinverwaltung_t::passengers->get_image_id(0), offset.x, y, 0, false, false);
	for (int i = 0; i < good_category->get_number_of_classes(); i++)
	{
		if (halt->get_connexions(good_category->get_catg_index(), i)->empty())
		{
			served = false;
		}
		else {
			served = true;
		}
		pas_info.append(goods_manager_t::get_translated_wealth_name(good_category->get_catg_index(), i));
		display_proportional_clip(offset.x + GOODS_SYMBOL_CELL_WIDTH, y, pas_info, ALIGN_LEFT, SYSCOL_TEXT, true);
		pas_info.clear();

		if (served || halt->get_ware_summe(good_category, i)) {
			pas_info.append(halt->get_ware_summe(good_category, i));
		}
		else {
			pas_info.append("-");
		}
		display_proportional_clip(offset.x + class_name_cell_width + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH, y, pas_info, ALIGN_RIGHT, SYSCOL_TEXT, true);
		pas_info.clear();

		int transfers_out = halt->get_leaving_goods_sum(good_category, i);
		int transfers_in = halt->get_transferring_goods_sum(good_category, i) - transfers_out;
		if (served || halt->get_transferring_goods_sum(good_category, i))
		{
			pas_info.printf("%4u/%4u", transfers_in, transfers_out);
		}
		else {
			pas_info.append("-");
		}
		left += display_proportional_clip(offset.x + class_name_cell_width + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5, y, pas_info, ALIGN_RIGHT, SYSCOL_TEXT, true);
		pas_info.clear();

		// color bar
		COLOR_VAL overlay_color = i < good_category->get_number_of_classes() / 2 ? COL_BLACK : COL_WHITE;
		uint8 overlay_transparency = abs(good_category->get_number_of_classes() / 2 - i) * 7;
		int bar_width = (halt->get_ware_summe(good_category, i) * GOODS_WAITING_BAR_BASE_WIDTH) / base_capacity;
		// transferring bar
		display_fillbox_wh_clip(offset.x + class_name_cell_width + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 10, y + 1, (transfers_in * GOODS_WAITING_BAR_BASE_WIDTH / base_capacity) + bar_width, 6, BARCOL_TRANSFER_IN, true);
		transferring_sum += halt->get_transferring_goods_sum(good_category, i);
		// leaving bar
		display_fillbox_wh_clip(offset.x + class_name_cell_width + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 10, y + 8, transfers_out * GOODS_WAITING_BAR_BASE_WIDTH / base_capacity, GOODS_LEAVING_BAR_HEIGHT, MN_GREY0, true);
		// waiting bar
		display_cylinderbar_wh_clip(offset.x + class_name_cell_width + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 10, y + 1, bar_width, 6, good_category->get_color(), true);

		y += LINESPACE + GOODS_LEAVING_BAR_HEIGHT + 1;

	}
	y += 2;
	display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH, y, offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width, y, MN_GREY1);
	display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + 4, y, offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + GOODS_WAITING_CELL_WIDTH, y, MN_GREY1);
	display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + GOODS_WAITING_CELL_WIDTH + 4, y, offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + GOODS_WAITING_CELL_WIDTH * 2 + 5, y, MN_GREY1);
	display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + GOODS_WAITING_CELL_WIDTH * 2 + 5 + 4, y, offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + GOODS_WAITING_CELL_WIDTH * 2 + 5 + GOODS_WAITING_BAR_BASE_WIDTH, y, MN_GREY1);
	y += 4;

	// total
	COLOR_VAL color;
	color = halt->is_overcrowded(good_category->get_catg_index()) ? COL_DARK_PURPLE : SYSCOL_TEXT;
	pas_info.append(halt->get_ware_summe(good_category));
	display_proportional_clip(offset.x + class_name_cell_width + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH, y, pas_info, ALIGN_RIGHT, color, true);
	pas_info.clear();

	pas_info.append(transferring_sum);
	left += display_proportional_clip(offset.x + class_name_cell_width + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5, y, pas_info, ALIGN_RIGHT, SYSCOL_TEXT, true);
	pas_info.clear();

	pas_info.printf("  %u ", halt->get_ware_summe(good_category) + transferring_sum);
	pas_info.printf(translator::translate("(max: %u)"), halt->get_capacity(good_category->get_catg_index()));
	left += display_proportional_clip(offset.x + class_name_cell_width + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 10, y, pas_info, ALIGN_LEFT, SYSCOL_TEXT, true);
	pas_info.clear();
}


void halt_detail_pas_t::draw(scr_coord offset)
{
	// keep previous maximum width
	int x_size = get_size().w - 51 - pos.x;
	int top = D_MARGIN_TOP;
	offset.x += D_MARGIN_LEFT;
	int left = 0;

	if (halt.is_bound()) {
		// Calculate width of class name cell
		uint8 class_name_cell_width = 0;
		uint8 pass_classes = goods_manager_t::passengers->get_number_of_classes();
		uint8 mail_classes = goods_manager_t::mail->get_number_of_classes();
		for (uint8 i = 0; i < pass_classes; i++) {
			class_name_cell_width = max(proportional_string_width(goods_manager_t::get_translated_wealth_name(goods_manager_t::INDEX_PAS, i)), class_name_cell_width);
		}
		for (uint8 i = 0; i < mail_classes; i++) {
			class_name_cell_width = max(proportional_string_width(goods_manager_t::get_translated_wealth_name(goods_manager_t::INDEX_MAIL, i)), class_name_cell_width);
		}

		display_proportional_clip(offset.x + GOODS_SYMBOL_CELL_WIDTH, offset.y + top, translator::translate("hd_wealth"), ALIGN_LEFT, SYSCOL_TEXT, true);
		display_proportional_clip(offset.x + class_name_cell_width + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH, offset.y + top, translator::translate("hd_waiting"), ALIGN_RIGHT, SYSCOL_TEXT, true);
		display_proportional_clip(offset.x + class_name_cell_width + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5, offset.y + top, translator::translate("hd_transfers"), ALIGN_RIGHT, SYSCOL_TEXT, true);
		top += LINESPACE + 2;

		display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width, offset.y + top, MN_GREY1);
		display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + 4, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + GOODS_WAITING_CELL_WIDTH, offset.y + top, MN_GREY1);
		display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + GOODS_WAITING_CELL_WIDTH + 4, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + GOODS_WAITING_CELL_WIDTH * 2 + 5, offset.y + top, MN_GREY1);
		display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + GOODS_WAITING_CELL_WIDTH * 2 + 5 + 4, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + class_name_cell_width + GOODS_WAITING_CELL_WIDTH * 2 + 5 + GOODS_WAITING_BAR_BASE_WIDTH, offset.y + top, MN_GREY1);
		top += 4;

		if (halt->get_pax_enabled()) {
			draw_class_table(scr_coord(offset.x, offset.y + top), class_name_cell_width, goods_manager_t::passengers);
			top += (pass_classes + 1) * (LINESPACE + GOODS_LEAVING_BAR_HEIGHT + 1) + 6;

			top += LINESPACE * 2;
		}

		if (halt->get_mail_enabled()) {
			draw_class_table(scr_coord(offset.x, offset.y + top), class_name_cell_width, goods_manager_t::mail);
			top += (mail_classes + 1) * (LINESPACE + GOODS_LEAVING_BAR_HEIGHT + 1) + 6;
		}

		// TODO: change this to "nearby" info and add nearby atraction building info. and more population and jobs info
		// Passengers can be further separated into commuters and visitors

		top += D_MARGIN_TOP;

		scr_size size(max(x_size + pos.x, get_size().w), top);
		if (size != get_size()) {
			set_size(size);
		}
	}
}



halt_detail_goods_t::halt_detail_goods_t(halthandle_t halt)
{
	this->halt = halt;
}


void halt_detail_goods_t::draw(scr_coord offset)
{
	// keep previous maximum width
	int x_size = get_size().w - 51 - pos.x;
	int top = D_MARGIN_TOP;
	offset.x += D_MARGIN_LEFT;
	int left = GOODS_SYMBOL_CELL_WIDTH;

	//goods_info.clear();
	if (halt.is_bound()) {
		active_freight_catg = 0;
		if (halt->get_ware_enabled())
		{
			int base_capacity = halt->get_capacity(2) ? max(halt->get_capacity(2), 10) : 10; // Note that capacity may have 0 even if enabled
			int waiting_sum = 0;
			int transship_sum = 0;

			goods_info.clear();

			// [Waiting goods info]
			display_proportional_clip(offset.x + GOODS_SYMBOL_CELL_WIDTH, offset.y + top, translator::translate("hd_category"), ALIGN_LEFT, SYSCOL_TEXT, true);
			display_proportional_clip(offset.x + D_BUTTON_WIDTH + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH, offset.y + top, translator::translate("hd_waiting"), ALIGN_RIGHT, SYSCOL_TEXT, true);
			display_proportional_clip(offset.x + D_BUTTON_WIDTH + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5, offset.y + top, translator::translate("hd_transship"), ALIGN_RIGHT, SYSCOL_TEXT, true);
			top += LINESPACE + 2;

			display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH, offset.y + top, MN_GREY1);
			display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + 4, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + GOODS_WAITING_CELL_WIDTH, offset.y + top, MN_GREY1);
			display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + GOODS_WAITING_CELL_WIDTH + 4, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5, offset.y + top, MN_GREY1);
			display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5 + 4, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5 + GOODS_WAITING_BAR_BASE_WIDTH, offset.y + top, MN_GREY1);
			top += 4;

			uint32 max_capacity = halt->get_capacity(2);
			const uint8 max_classes = max(goods_manager_t::passengers->get_number_of_classes(), goods_manager_t::mail->get_number_of_classes());
			for (uint i = 0; i < goods_manager_t::get_max_catg_index(); i++) {
				if (i == goods_manager_t::INDEX_PAS || i == goods_manager_t::INDEX_MAIL)
				{
					continue;
				}

				const goods_desc_t* info = goods_manager_t::get_info_catg_index(i);
				goods_info.append(translator::translate(info->get_catg_name()));

				const goods_desc_t *wtyp = goods_manager_t::get_info(i);
				if (halt->gibt_ab(info)) {
					uint32 waiting_sum_catg = 0;
					uint32 transship_in_catg = 0;
					uint32 leaving_sum_catg = 0;

					// category symbol
					display_color_img(info->get_catg_symbol(), offset.x, offset.y + top, 0, false, false);

					display_proportional_clip(offset.x + GOODS_SYMBOL_CELL_WIDTH, offset.y + top, goods_info, ALIGN_LEFT, SYSCOL_TEXT, true);
					goods_info.clear();

					int bar_offset_left = 0;
					switch (i) {
					case 0:
						waiting_sum_catg = halt->get_ware_summe(wtyp);
						display_fillbox_wh_clip(offset.x + D_BUTTON_WIDTH + bar_offset_left + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 10, offset.y + top + 1, halt->get_ware_summe(wtyp) * GOODS_WAITING_BAR_BASE_WIDTH / base_capacity, 6, wtyp->get_color(), true);
						display_blend_wh(offset.x + D_BUTTON_WIDTH + bar_offset_left + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 10, offset.y + top + 6, halt->get_ware_summe(wtyp) * GOODS_WAITING_BAR_BASE_WIDTH / base_capacity, 1, COL_BLACK, 10);
						bar_offset_left = halt->get_ware_summe(wtyp) * GOODS_WAITING_BAR_BASE_WIDTH / base_capacity;
						leaving_sum_catg = halt->get_leaving_goods_sum(wtyp, 0);
						transship_in_catg = halt->get_transferring_goods_sum(wtyp, 0) - leaving_sum_catg;
						break;
					default:
						const uint8  count = goods_manager_t::get_count();
						for (uint32 j = 3; j < count; j++) {
							goods_desc_t const* const wtyp2 = goods_manager_t::get_info(j);
							if (wtyp2->get_catg_index() != i) {
								continue;
							}
							waiting_sum_catg += halt->get_ware_summe(wtyp2);
							leaving_sum_catg += halt->get_leaving_goods_sum(wtyp2, 0);
							transship_in_catg += halt->get_transferring_goods_sum(wtyp2, 0) - halt->get_leaving_goods_sum(wtyp2, 0);

							// color bar
							int bar_width = halt->get_ware_summe(wtyp2) * GOODS_WAITING_BAR_BASE_WIDTH / base_capacity;

							// waiting bar
							if (bar_width) {
								display_cylinderbar_wh_clip(offset.x + D_BUTTON_WIDTH + bar_offset_left + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 10, offset.y + top + 1, bar_width, 6, wtyp2->get_color(), true);
							}
							bar_offset_left += bar_width;
						}
						break;
					}
					transship_sum += leaving_sum_catg + transship_in_catg;
					// transferring bar
					display_fillbox_wh_clip(offset.x + D_BUTTON_WIDTH + bar_offset_left + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 10, offset.y + top + 1, transship_in_catg * GOODS_WAITING_BAR_BASE_WIDTH / base_capacity, 6, BARCOL_TRANSFER_IN, true);
					// leaving bar
					display_fillbox_wh_clip(offset.x + D_BUTTON_WIDTH + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 10, offset.y + top + 8, leaving_sum_catg * GOODS_WAITING_BAR_BASE_WIDTH / base_capacity, GOODS_LEAVING_BAR_HEIGHT, MN_GREY0, true);

					//waiting
					goods_info.append(waiting_sum_catg);
					display_proportional_clip(offset.x + D_BUTTON_WIDTH + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH, offset.y + top, goods_info, ALIGN_RIGHT, SYSCOL_TEXT, true);
					goods_info.clear();
					waiting_sum += waiting_sum_catg;

					//transfer
					goods_info.printf("%4u/%4u", transship_in_catg, leaving_sum_catg);
					//goods_info.append(transship_sum_catg);
					left += display_proportional_clip(offset.x + D_BUTTON_WIDTH + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5, offset.y + top, goods_info, ALIGN_RIGHT, SYSCOL_TEXT, true);
					goods_info.clear();
					top += LINESPACE + GOODS_LEAVING_BAR_HEIGHT + 1;
					active_freight_catg++;
				}
				goods_info.clear();

			}
			if (!active_freight_catg) {
				// There is no data until connection data is updated, or no freight service has been operated
				if (skinverwaltung_t::alerts) {
					display_color_img(skinverwaltung_t::alerts->get_image_id(2), offset.x + D_BUTTON_WIDTH, offset.y + top, 0, false, false);
				}
				display_proportional_clip(offset.x + D_BUTTON_WIDTH + GOODS_SYMBOL_CELL_WIDTH, offset.y + top, translator::translate("no data"), ALIGN_LEFT, MN_GREY0, true);
				top += LINESPACE;
			}

			top += 2;

			display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH, offset.y + top, MN_GREY1);
			display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + 4, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + GOODS_WAITING_CELL_WIDTH, offset.y + top, MN_GREY1);
			display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + GOODS_WAITING_CELL_WIDTH + 4, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5, offset.y + top, MN_GREY1);
			display_direct_line(offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5 + 4, offset.y + top, offset.x + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5 + GOODS_WAITING_BAR_BASE_WIDTH, offset.y + top, MN_GREY1);
			top += 4;

			// total
			COLOR_VAL color;
			color = halt->is_overcrowded(2) ? COL_DARK_PURPLE : SYSCOL_TEXT;
			goods_info.append(waiting_sum);
			display_proportional_clip(offset.x + D_BUTTON_WIDTH + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH, offset.y + top, goods_info, ALIGN_RIGHT, color, true);
			goods_info.clear();

			goods_info.append(transship_sum);
			left += display_proportional_clip(offset.x + D_BUTTON_WIDTH + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 5, offset.y + top, goods_info, ALIGN_RIGHT, SYSCOL_TEXT, true);
			goods_info.clear();

			goods_info.printf("  %u ", waiting_sum + transship_sum);
			goods_info.printf(translator::translate("(max: %u)"), halt->get_capacity(2));
			left += display_proportional_clip(offset.x + D_BUTTON_WIDTH + GOODS_SYMBOL_CELL_WIDTH + GOODS_WAITING_CELL_WIDTH * 2 + 10, offset.y + top, goods_info, ALIGN_LEFT, SYSCOL_TEXT, true);
			goods_info.clear();

			top += LINESPACE * 2;

			goods_info.clear();

		}
		goods_info.clear();

		scr_size size(max(x_size + pos.x, get_size().w), top);
		if (size != get_size()) {
			set_size(size);
		}
	}
}


void halt_detail_goods_t::recalc_size()
{
	set_size(scr_size(400, D_MARGIN_TOP + active_freight_catg * (LINESPACE + GOODS_LEAVING_BAR_HEIGHT + 1) + LINESPACE * 4 + 12));
}

gui_halt_nearby_factory_info_t::gui_halt_nearby_factory_info_t(const halthandle_t & halt)
{
	this->halt = halt;

	//recalc_size();
	line_selected = 0xFFFFFFFFu;
}

void gui_halt_nearby_factory_info_t::recalc_size()
{
	int yoff = 0;
	if (halt.is_bound()) {
		// set for default size
		yoff = halt->get_fab_list().get_count() * (LINESPACE + 1);
		if (yoff) { yoff += LINESPACE; }
		yoff += LINESPACE*2;

		yoff += LINESPACE * max(required_material.get_count(), active_product.get_count() + inactive_product.get_count());

		set_size(scr_size(400, yoff));
	}
}


void gui_halt_nearby_factory_info_t::draw(scr_coord offset)
{
	clip_dimension const cd = display_get_clip_wh();
	const int start = cd.y - LINESPACE - 1;
	const int end = cd.yy + LINESPACE + 1;

	static cbuffer_t buf;
	int xoff = pos.x;
	int yoff = pos.y;

	const slist_tpl<fabrik_t*> & fab_list = halt->get_fab_list();

	uint32 sel = line_selected;
	FORX(const slist_tpl<fabrik_t*>, const fab, fab_list, yoff += LINESPACE + 1) {
		xoff = D_POS_BUTTON_WIDTH + D_H_SPACE;
		// [status color bar]
		COLOR_VAL col = fabrik_t::status_to_color[fab->get_status() % fabrik_t::staff_shortage];
		uint indikatorfarbe = fabrik_t::status_to_color[fab->get_status() % fabrik_t::staff_shortage];
		display_fillbox_wh_clip(offset.x + xoff + 1, offset.y + yoff + 3, D_INDICATOR_WIDTH / 2 - 1, D_INDICATOR_HEIGHT, col, true);
		xoff += D_INDICATOR_WIDTH / 2 + 3;

		// [name]
		buf.clear();
		buf.printf("%s (%d,%d)", fab->get_name(), fab->get_pos().x, fab->get_pos().y);
		xoff += display_proportional_clip(offset.x + xoff, offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);

		xoff = max(xoff, D_BUTTON_WIDTH * 2 + D_INDICATOR_WIDTH);

		int has_input_output = 0;
		FOR(array_tpl<ware_production_t>, const& i, fab->get_input()) {
			goods_desc_t const* const ware = i.get_typ();
			if (skinverwaltung_t::input_output && !has_input_output) {
				display_color_img(skinverwaltung_t::input_output->get_image_id(0), offset.x + xoff, offset.y + yoff, 0, false, false);
				xoff += GOODS_SYMBOL_CELL_WIDTH;
			}
			// input goods color square box
			ware->get_name();
			display_colorbox_with_tooltip(offset.x + xoff, offset.y + yoff, 8, 8, ware->get_color(), true, translator::translate(ware->get_name()));
			xoff += GOODS_SYMBOL_CELL_WIDTH - 2;

			required_material.append_unique(ware);
			has_input_output++;
		}

		if (has_input_output < 4) {
			xoff += has_input_output ? (GOODS_SYMBOL_CELL_WIDTH - 2) * (4 - has_input_output) : (GOODS_SYMBOL_CELL_WIDTH - 2) * 5 + 2;
		}
		has_input_output = 0;
		FOR(array_tpl<ware_production_t>, const& i, fab->get_output()) {
			goods_desc_t const* const ware = i.get_typ();
			if (skinverwaltung_t::input_output && !has_input_output) {
				display_color_img(skinverwaltung_t::input_output->get_image_id(1), offset.x + xoff, offset.y + yoff, 0, false, false);
				xoff += GOODS_SYMBOL_CELL_WIDTH;
			}
			// output goods color square box
			display_colorbox_with_tooltip(offset.x + xoff, offset.y + yoff, 8, 8, ware->get_color(), true, translator::translate(ware->get_name()));
			xoff += GOODS_SYMBOL_CELL_WIDTH - 2;

			if (!active_product.is_contained(ware)) {
				if ((fab->get_status() % fabrik_t::no_material || fab->get_status() % fabrik_t::material_shortage) && !i.menge) {
					// this factory is not in operation
					inactive_product.append_unique(ware);
				}
				else {
					active_product.append(ware);
				}
			}
			has_input_output++;
		}
		if (fab->get_sector() == fabrik_t::power_plant) {
			xoff += GOODS_SYMBOL_CELL_WIDTH - 2;
			display_color_img(skinverwaltung_t::electricity->get_image_id(0), offset.x + xoff, offset.y + yoff, 0, false, false);
		}


		// goto button
		bool selected = sel == 0 || welt->get_viewport()->is_on_center(fab->get_pos());
		display_img_aligned(gui_theme_t::pos_button_img[selected], scr_rect(offset.x + D_H_SPACE, offset.y + yoff, D_POS_BUTTON_WIDTH, LINESPACE), ALIGN_CENTER_V | ALIGN_CENTER_H, true);
		sel--;

		if (win_get_magic((ptrdiff_t)fab)) {
			display_blend_wh(offset.x, offset.y + yoff, size.w, LINESPACE, SYSCOL_TEXT, 20);
		}
	}
	if (!halt->get_fab_list().get_count()) {
		display_proportional_clip(offset.x + D_MARGIN_LEFT, offset.y + yoff, translator::translate("keine"), ALIGN_LEFT, MN_GREY0, true);
		yoff += LINESPACE;
	}
	yoff += LINESPACE;

	// [Goods needed by nearby industries]
	xoff = GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH * 1.5; // 2nd col x offset
	offset.x += D_MARGIN_LEFT;
	if ((!required_material.empty() || !active_product.empty() || !inactive_product.empty()) && halt->get_ware_enabled()) {
		int input_cnt = 0;
		int output_cnt = 0;
		if (skinverwaltung_t::input_output) {
			display_color_img_with_tooltip(skinverwaltung_t::input_output->get_image_id(0), offset.x, offset.y + yoff, 0, false, false, translator::translate("Angenommene Waren"));
			display_color_img(skinverwaltung_t::input_output->get_image_id(1), offset.x + xoff, offset.y + yoff, 0, false, false);
		}
		display_proportional_clip(skinverwaltung_t::input_output ? offset.x + GOODS_SYMBOL_CELL_WIDTH : offset.x, offset.y + yoff, translator::translate("Needed"), ALIGN_LEFT, SYSCOL_TEXT, true);
		display_proportional_clip(skinverwaltung_t::input_output ? offset.x + GOODS_SYMBOL_CELL_WIDTH + xoff : offset.x + xoff, offset.y + yoff, translator::translate("Products"), ALIGN_LEFT, SYSCOL_TEXT, true);
		yoff += LINESPACE;
		yoff += 2;

		display_direct_line(offset.x, offset.y + yoff, offset.x + xoff - 4, offset.y + yoff, MN_GREY1);
		display_direct_line(offset.x + xoff, offset.y + yoff, offset.x + xoff + GOODS_SYMBOL_CELL_WIDTH + D_BUTTON_WIDTH * 1.5, offset.y + yoff, MN_GREY1);
		yoff += 4;

		for (uint32 i = 0; i < goods_manager_t::get_count(); i++) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			// inuput
			if (required_material.is_contained(ware)) {
				// category symbol
				display_color_img(ware->get_catg_symbol(), offset.x, offset.y + yoff + LINESPACE * input_cnt, 0, false, false);
				// goods color
				display_ddd_box_clip(offset.x + GOODS_SYMBOL_CELL_WIDTH, offset.y + yoff + LINESPACE * input_cnt, 8, 8, MN_GREY0, MN_GREY4);
				display_fillbox_wh_clip(offset.x + GOODS_SYMBOL_CELL_WIDTH + 1, offset.y + yoff + 1 + LINESPACE * input_cnt, 6, 6, ware->get_color(), true);
				// goods name
				display_proportional_clip(offset.x + GOODS_SYMBOL_CELL_WIDTH * 2 - 2, offset.y + yoff + LINESPACE * input_cnt, translator::translate(ware->get_name()), ALIGN_LEFT, SYSCOL_TEXT, true);
				input_cnt++;
			}

			//output
			if (active_product.is_contained(ware) || inactive_product.is_contained(ware)) {
				// category symbol
				display_color_img(ware->get_catg_symbol(), offset.x + xoff, offset.y + yoff + LINESPACE * output_cnt, 0, false, false);
				// goods color
				display_ddd_box_clip(offset.x + xoff + GOODS_SYMBOL_CELL_WIDTH, offset.y + yoff + LINESPACE * output_cnt, 8, 8, MN_GREY0, MN_GREY4);
				display_fillbox_wh_clip(offset.x + xoff + GOODS_SYMBOL_CELL_WIDTH + 1, offset.y + yoff + 1 + LINESPACE * output_cnt, 6, 6, ware->get_color(), true);
				// goods name
				COLOR_VAL text_color;
				display_proportional_clip(offset.x + xoff + GOODS_SYMBOL_CELL_WIDTH * 2 - 2, offset.y + yoff + LINESPACE * output_cnt, translator::translate(ware->get_name()), ALIGN_LEFT, text_color = active_product.is_contained(ware) ? SYSCOL_TEXT : MN_GREY0, true);
				output_cnt++;
			}
		}

		if (required_material.empty()) {
			display_proportional_clip(offset.x + D_MARGIN_LEFT, offset.y + yoff, translator::translate("keine"), ALIGN_LEFT, MN_GREY0, true);
		}
		if (active_product.empty() && inactive_product.empty()) {
			display_proportional_clip(offset.x + xoff + D_MARGIN_LEFT, offset.y + yoff, translator::translate("keine"), ALIGN_LEFT, MN_GREY0, true);
		}
		yoff += LINESPACE * max(input_cnt, output_cnt) + 1;
	}

	set_size(scr_size(400, yoff - pos.y));
}


bool gui_halt_nearby_factory_info_t::infowin_event(const event_t * ev)
{
	const unsigned int line = (ev->cy) / (LINESPACE + 1);
	line_selected = 0xFFFFFFFFu;
	if (line >= halt->get_fab_list().get_count()) {
		return false;
	}

	fabrik_t * fab = halt->get_fab_list().at(line);
	const koord3d fab_pos = fab->get_pos();

	if (!fab) {
		welt->get_viewport()->change_world_position(fab_pos);
		return false;
	}

	if (IS_LEFTRELEASE(ev)) {
		if (ev->cx > 0 && ev->cx < 15) {
			welt->get_viewport()->change_world_position(fab_pos);
		}
		else {
			fab->show_info();
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		welt->get_viewport()->change_world_position(fab_pos);
	}
	return false;
}
