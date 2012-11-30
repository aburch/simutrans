/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "fabrik_info.h"

#include "components/gui_button.h"

#include "help_frame.h"
#include "factory_chart.h"

#include "../simfab.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simcity.h"
#include "../simwin.h"
#include "../simmenu.h"
#include "../player/simplay.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"


fabrik_info_t::fabrik_info_t(fabrik_t* fab_, const gebaeude_t* gb) :
	gui_frame_t("", fab_->get_besitzer()),
	fab(fab_),
	chart(fab_),
	view(gb, koord( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width() * 7) / 8))),
	scrolly(&fab_info),
	fab_info(fab_),
	prod(&prod_buf),
	txt(&info_buf)
{
	lieferbuttons = supplierbuttons = stadtbuttons = NULL;
	welt = fab->get_besitzer()->get_welt();

	tstrncpy( fabname, fab->get_name(), lengthof(fabname) );
	gui_frame_t::set_name( fabname );

	const sint16 total_width = D_MARGIN_LEFT + 3*(D_BUTTON_WIDTH + D_H_SPACE) + max( D_BUTTON_WIDTH, view.get_groesse().x ) + D_MARGIN_RIGHT;

	input.set_pos(koord(D_MARGIN_LEFT,D_MARGIN_TOP));
	input.set_text( fabname, lengthof(fabname) );
	input.add_listener(this);
	add_komponente(&input);

	add_komponente(&view);

	prod.set_pos( koord( D_MARGIN_LEFT, D_MARGIN_TOP+D_BUTTON_HEIGHT+D_V_SPACE ) );
	fab->info_prod( prod_buf );
	prod.recalc_size();
	add_komponente( &prod );

	const sint16 offset_below_viewport = D_MARGIN_TOP+D_BUTTON_HEIGHT+D_V_SPACE+ max( prod.get_groesse().y, view.get_groesse().y + 8 + D_V_SPACE );

	chart.set_pos( koord(0, offset_below_viewport - 5) );

	chart_button.init(button_t::roundbox_state, "Chart", koord(BUTTON3_X,offset_below_viewport), koord(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	chart_button.set_tooltip("Show/hide statistics");
	chart_button.add_listener(this);
	add_komponente(&chart_button);

	// Hajo: "About" button only if translation is available
	char key[256];
	sprintf(key, "factory_%s_details", fab->get_besch()->get_name());
	const char * value = translator::translate(key);
	if(value && *value != 'f') {
		details_button.init( button_t::roundbox, "Details", koord(BUTTON4_X,offset_below_viewport), koord(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
//		details_button.set_tooltip("Factory details");
		details_button.add_listener(this);
		add_komponente(&details_button);
	}

	// calculate height
	fab->info_prod(prod_buf);

	// fill position buttons etc
	fab->info_conn(info_buf);
	txt.recalc_size();
	update_info();

	scrolly.set_pos(koord(0, offset_below_viewport+D_BUTTON_HEIGHT+D_V_SPACE+6));
	scrolly.set_bottom_margin(true);
	add_komponente(&scrolly);

	set_min_windowsize(koord(total_width, D_TITLEBAR_HEIGHT+scrolly.get_pos().y+LINESPACE*5+D_MARGIN_BOTTOM));
	set_fenstergroesse(koord(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+scrolly.get_pos().y+fab_info.get_groesse().y+D_MARGIN_BOTTOM ));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}


fabrik_info_t::~fabrik_info_t()
{
	rename_factory();
	fabname[0] = 0;

	delete [] lieferbuttons;
	delete [] supplierbuttons;
	delete [] stadtbuttons;
}


void fabrik_info_t::rename_factory()
{
	if(  fabname[0]  &&  welt->get_fab_list().is_contained(fab)  &&  strcmp(fabname, fab->get_name())  ) {
		// text changed and factory still exists => call tool
		cbuffer_t buf;
		buf.printf( "f%s,%s", fab->get_pos().get_str(), fabname );
		werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
		w->set_default_param( buf );
		welt->set_werkzeug( w, welt->get_spieler(1));
		// since init always returns false, it is safe to delete immediately
		delete w;
	}
}


/**
 * Set window size and adjust component sizes and/or positions accordingly
 * @author Markus Weber
 */
void fabrik_info_t::set_fenstergroesse(koord groesse)
{
	gui_frame_t::set_fenstergroesse(groesse);

	// would be only needed in case of enabling horizontal resizes
	input.set_groesse(koord(get_fenstergroesse().x-D_MARGIN_LEFT-D_MARGIN_RIGHT, D_BUTTON_HEIGHT));
	view.set_pos(koord(get_fenstergroesse().x - view.get_groesse().x - D_MARGIN_RIGHT , D_MARGIN_TOP+D_BUTTON_HEIGHT+D_V_SPACE ));

	scrolly.set_groesse(get_client_windowsize()-scrolly.get_pos()-koord(0,D_MARGIN_BOTTOM));
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 *
 * @author Hj. Malthaner
 */
void fabrik_info_t::zeichnen(koord pos, koord gr)
{
	const koord old_size = txt.get_groesse();

	fab->info_prod( prod_buf );
	fab->info_conn( info_buf );

	gui_frame_t::zeichnen(pos,gr);

	if(  old_size != txt.get_groesse()  ) {
		update_info();
	}

	prod_buf.clear();
	prod_buf.append( translator::translate("Durchsatz") );
	prod_buf.append( fab->get_current_production(), 0 );
	prod_buf.append( translator::translate("units/day") );

	unsigned indikatorfarbe = fabrik_t::status_to_color[fab->get_status()];
	display_ddd_box_clip(pos.x + view.get_pos().x, pos.y + view.get_pos().y + view.get_groesse().y + D_TITLEBAR_HEIGHT, view.get_groesse().x, 8, MN_GREY0, MN_GREY4);
	display_fillbox_wh_clip(pos.x + view.get_pos().x + 1, pos.y + view.get_pos().y + view.get_groesse().y + D_TITLEBAR_HEIGHT+1, view.get_groesse().x - 2, 6, indikatorfarbe, true);
	KOORD_VAL x_view_pos = D_MARGIN_LEFT;
	KOORD_VAL x_prod_pos = D_MARGIN_LEFT+proportional_string_width(prod_buf)+10;
	if(  skinverwaltung_t::electricity->get_bild_nr(0)!=IMG_LEER  ) {
		// indicator for recieving
		if(  fab->get_prodfactor_electric()>0  ) {
			display_color_img(skinverwaltung_t::electricity->get_bild_nr(0), pos.x + view.get_pos().x + x_view_pos, pos.y + view.get_pos().y + D_TITLEBAR_HEIGHT+4, 0, false, false);
			x_view_pos += skinverwaltung_t::electricity->get_bild(0)->get_pic()->w+4;
		}
		// indicator for enabled
		if(  fab->get_besch()->get_electric_boost()  ) {
			display_color_img( skinverwaltung_t::electricity->get_bild_nr(0), pos.x + x_prod_pos, pos.y + view.get_pos().y + D_TITLEBAR_HEIGHT, 0, false, false);
			x_prod_pos += skinverwaltung_t::electricity->get_bild(0)->get_pic()->w+4;
		}
	}
	if(  skinverwaltung_t::passagiere->get_bild_nr(0)!=IMG_LEER  ) {
		if(  fab->get_prodfactor_pax()>0  ) {
			display_color_img(skinverwaltung_t::passagiere->get_bild_nr(0), pos.x + view.get_pos().x + x_view_pos, pos.y + view.get_pos().y + D_TITLEBAR_HEIGHT+4, 0, false, false);
			x_view_pos += skinverwaltung_t::passagiere->get_bild(0)->get_pic()->w+4;
		}
		if(  fab->get_besch()->get_pax_boost()  ) {
			display_color_img( skinverwaltung_t::passagiere->get_bild_nr(0), pos.x + x_prod_pos, pos.y + view.get_pos().y + D_TITLEBAR_HEIGHT, 0, false, false);
			x_prod_pos += skinverwaltung_t::passagiere->get_bild(0)->get_pic()->w+4;
		}
	}
	if(  skinverwaltung_t::post->get_bild_nr(0)!=IMG_LEER  ) {
		if(  fab->get_prodfactor_mail()>0  ) {
			display_color_img(skinverwaltung_t::post->get_bild_nr(0), pos.x + view.get_pos().x + x_view_pos, pos.y + view.get_pos().y + D_TITLEBAR_HEIGHT+4, 0, false, false);
		}
		if(  fab->get_besch()->get_mail_boost()  ) {
			display_color_img( skinverwaltung_t::post->get_bild_nr(0), pos.x + x_prod_pos, pos.y + view.get_pos().y + D_TITLEBAR_HEIGHT, 0, false, false);
		}
	}
}


bool fabrik_info_t::is_weltpos()
{
	return ( welt->get_x_off() | welt->get_y_off()) == 0  &&
		welt->get_world_position() == welt->calculate_world_position( get_weltpos(false) );
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 * V.Meyer
 */
bool fabrik_info_t::action_triggered( gui_action_creator_t *komp, value_t v)
{
	if(komp == &chart_button) {
		chart_button.pressed ^= 1;
		if(  !chart_button.pressed  ) {
			remove_komponente( &chart );
		}
		else {
			add_komponente( &chart );
		}
		const koord offset = chart_button.pressed ? koord(0, chart.get_groesse().y - 6) : koord(0, -(chart.get_groesse().y - 6));
		set_min_windowsize(get_min_windowsize() + offset);
		chart_button.set_pos( chart_button.get_pos() + offset );
		details_button.set_pos( details_button.get_pos() + offset );
		scrolly.set_pos( scrolly.get_pos() + offset );
		resize( koord(0,(chart_button.pressed ? chart.get_groesse().y : -chart.get_groesse().y) ) );
	}
	else if(komp == &input) {
		rename_factory();
	}
	else if(komp == &details_button) {
		help_frame_t * frame = new help_frame_t();
		char key[256];
		sprintf(key, "factory_%s_details", fab->get_besch()->get_name());
		frame->set_text(translator::translate(key));
		create_win(frame, w_info, (ptrdiff_t)this);
	}
	else if(v.i&~1) {
		koord k = *(const koord *)v.p;
		karte_t* const welt = fab->get_besitzer()->get_welt();
		welt->change_world_position( koord3d(k,welt->max_hgt(k)) );
	}

	return true;
}


static inline koord const& get_coord(koord   const&       c) { return c; }
static inline koord        get_coord(stadt_t const* const c) { return c->get_pos(); }


template <typename T> static void make_buttons(button_t*& dst, T const& coords, int& y_off, gui_container_t& fab_info, action_listener_t* const listener)
{
	delete [] dst;
	if (coords.empty()) {
		dst = 0;
	}
	else {
		button_t* b = dst = new button_t[coords.get_count()];
		FORTX(T, const& i, coords, ++b) {
			b->set_pos(koord(D_MARGIN_LEFT, y_off));
			y_off += LINESPACE;
			b->set_typ(button_t::posbutton);
			b->set_targetpos(get_coord(i));
			b->add_listener(listener);
			fab_info.add_komponente(b);
		}
		y_off += 2 * LINESPACE;
	}
}


void fabrik_info_t::update_info()
{
	tstrncpy( fabname, fab->get_name(), lengthof(fabname) );
	gui_frame_t::set_name( fabname );
	input.set_text( fabname, lengthof(fabname) );

	fab_info.remove_all();

	// needs to update all text
	fab_info.set_pos( koord(0,0) );
	txt.set_pos( koord(D_MARGIN_LEFT,0) );
	fab_info.add_komponente(&txt);

	int y_off = LINESPACE;
	make_buttons(lieferbuttons,   fab->get_lieferziele(),   y_off, fab_info, this);
	make_buttons(supplierbuttons, fab->get_suppliers(),     y_off, fab_info, this);
	make_buttons(stadtbuttons,    fab->get_target_cities(), y_off, fab_info, this);

	fab_info.set_groesse( koord( fab_info.get_groesse().x, txt.get_groesse().y ) );
}


// component for city demand icons display
gui_fabrik_info_t::gui_fabrik_info_t(const fabrik_t* fab)
{
	this->fab = fab;
}


void gui_fabrik_info_t::zeichnen(koord offset)
{
	int xoff = pos.x+offset.x+D_MARGIN_LEFT+16;
	int yoff = pos.y+offset.y;

	gui_container_t::zeichnen( offset );

	if(  fab->get_lieferziele().get_count()  ) {
		yoff += LINESPACE;
		FOR(  const vector_tpl<koord>, k, fab->get_lieferziele() ) {
			if(  fab->is_active_lieferziel(k)  ) {
				display_color_img(skinverwaltung_t::waren->get_bild_nr(0), xoff, yoff, 0, false, true);
			}
			yoff += LINESPACE;
		}
		yoff += LINESPACE;
	}

	if(  fab->get_suppliers().get_count()  ) {
		yoff += LINESPACE;
		FOR(  const vector_tpl<koord>, k, fab->get_suppliers() ) {
			if(  const fabrik_t *src = fabrik_t::get_fab(fab->get_besitzer()->get_welt(),k)  ) {
				if(  src->is_active_lieferziel(fab->get_pos().get_2d())  ) {
					display_color_img(skinverwaltung_t::waren->get_bild_nr(0), xoff, yoff, 0, false, true);
				}
			}
			yoff += LINESPACE;
		}
		yoff += LINESPACE;
	}

	const vector_tpl<stadt_t *> &target_cities = fab->get_target_cities();
	if(  !target_cities.empty()  ) {
		yoff += LINESPACE;

		FOR(vector_tpl<stadt_t*>, const c, target_cities) {
			stadt_t::factory_entry_t const* const pax_entry  = c->get_target_factories_for_pax().get_entry(fab);
			stadt_t::factory_entry_t const* const mail_entry = c->get_target_factories_for_mail().get_entry(fab);
			assert( pax_entry && mail_entry );

			cbuffer_t buf;
			int w;

			buf.clear();
			buf.printf("%i", pax_entry->supply);
			w = proportional_string_width( buf );
			display_proportional_clip( xoff+18+(w>21?w-21:0), yoff, buf, ALIGN_RIGHT, COL_BLACK, true );
			display_color_img(skinverwaltung_t::passagiere->get_bild_nr(0), xoff+20+1+(w>21?w-21:0), yoff, 0, false, true);

			buf.clear();
			buf.printf("%i", mail_entry->supply);
			w = proportional_string_width( buf );
			display_proportional_clip( xoff+62+(w>21?w-21:0), yoff, buf, ALIGN_RIGHT, COL_BLACK, true );
			display_color_img(skinverwaltung_t::post->get_bild_nr(0), xoff+64+1+(w>21?w-21:0), yoff, 0, false, true);

			display_proportional_clip(xoff + 90, yoff, c->get_name(), ALIGN_LEFT, COL_BLACK, true);
			yoff += LINESPACE;
		}
		yoff += 2 * LINESPACE;
	}
}
