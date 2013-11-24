/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Factory info dialog
 */

#include "fabrik_info.h"

#include "components/gui_button.h"

#include "help_frame.h"
#include "factory_chart.h"

#include "../simfab.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simcity.h"
#include "../gui/simwin.h"
#include "../simmenu.h"
#include "../player/simplay.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../utils/simstring.h"


fabrik_info_t::fabrik_info_t(fabrik_t* fab_, const gebaeude_t* gb) :
	gui_frame_t("", fab_->get_besitzer()),
	fab(fab_),
	chart(fab_),
	view(gb, scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width() * 7) / 8))),
	scrolly(&fab_info),
	prod(&prod_buf),
	txt(&info_buf)
{
	lieferbuttons = supplierbuttons = stadtbuttons = NULL;

	tstrncpy( fabname, fab->get_name(), lengthof(fabname) );
	gui_frame_t::set_name( fabname );

	const sint16 total_width = D_MARGIN_LEFT + 3*(D_BUTTON_WIDTH + D_H_SPACE) + max( D_BUTTON_WIDTH, view.get_size().w ) + D_MARGIN_RIGHT;

	input.set_pos(scr_coord(D_MARGIN_LEFT,D_MARGIN_TOP));
	input.set_text( fabname, lengthof(fabname) );
	input.add_listener(this);
	add_komponente(&input);

	add_komponente(&view);

	prod.set_pos( scr_coord( D_MARGIN_LEFT, D_MARGIN_TOP+D_BUTTON_HEIGHT+D_V_SPACE ) );
	fab->info_prod( prod_buf );
	prod.recalc_size();
	add_komponente( &prod );

	const sint16 offset_below_viewport = D_MARGIN_TOP+D_BUTTON_HEIGHT+D_V_SPACE+ max( prod.get_size().h, view.get_size().h + 8 + D_V_SPACE );

	chart.set_pos( scr_coord(0, offset_below_viewport - 5) );

	chart_button.init(button_t::roundbox_state, "Chart", scr_coord(BUTTON3_X,offset_below_viewport), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	chart_button.set_tooltip("Show/hide statistics");
	chart_button.add_listener(this);
	add_komponente(&chart_button);

	// Hajo: "About" button only if translation is available
	char key[256];
	sprintf(key, "factory_%s_details", fab->get_besch()->get_name());
	const char * value = translator::translate(key);
	if(value && *value != 'f') {
		details_button.init( button_t::roundbox, "Details", scr_coord(BUTTON4_X,offset_below_viewport), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
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

	scrolly.set_pos(scr_coord(0, offset_below_viewport+D_BUTTON_HEIGHT+D_V_SPACE+6));
	add_komponente(&scrolly);

	set_min_windowsize(scr_size(total_width, D_TITLEBAR_HEIGHT+scrolly.get_pos().y+LINESPACE*5+D_MARGINS_Y));
	scr_coord_val y = min( D_TITLEBAR_HEIGHT+scrolly.get_pos().y+fab_info.get_size().h+D_MARGINS_Y,  display_get_height() - env_t::iconsize.h - 16);
	set_windowsize(scr_size(D_DEFAULT_WIDTH, y ));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
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
void fabrik_info_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);

	// would be only needed in case of enabling horizontal resizes
	input.set_size(scr_size(get_windowsize().w-D_MARGIN_LEFT-D_MARGIN_RIGHT, D_BUTTON_HEIGHT));
	view.set_pos(scr_coord(get_windowsize().w - view.get_size().w - D_MARGIN_RIGHT , D_MARGIN_TOP+D_BUTTON_HEIGHT+D_V_SPACE ));

	scrolly.set_size(get_client_windowsize()-scrolly.get_pos());
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 * @author Hj. Malthaner
 */
void fabrik_info_t::draw(scr_coord pos, scr_size size)
{
	const scr_size old_size = txt.get_size();

	fab->info_prod( prod_buf );
	fab->info_conn( info_buf );

	gui_frame_t::draw(pos,size);
	set_dirty();

	if(  old_size != txt.get_size()  ) {
		update_info();
	}

	prod_buf.clear();
	prod_buf.append( translator::translate("Durchsatz") );
	prod_buf.append( fab->get_current_production(), 0 );
	prod_buf.append( translator::translate("units/day") );

	unsigned indikatorfarbe = fabrik_t::status_to_color[fab->get_status()];
	display_ddd_box_clip(pos.x + view.get_pos().x, pos.y + view.get_pos().y + view.get_size().h + D_TITLEBAR_HEIGHT, view.get_size().w, D_INDICATOR_HEIGHT, MN_GREY0, MN_GREY4);
	display_fillbox_wh_clip(pos.x + view.get_pos().x + 1, pos.y + view.get_pos().y + view.get_size().h + D_TITLEBAR_HEIGHT+1, view.get_size().w - 2, D_INDICATOR_HEIGHT-2, indikatorfarbe, true);
	scr_coord_val x_view_pos = D_MARGIN_LEFT;
	scr_coord_val x_prod_pos = D_MARGIN_LEFT+proportional_string_width(prod_buf)+10;
	if(  skinverwaltung_t::electricity->get_bild_nr(0)!=IMG_LEER  ) {
		// indicator for receiving
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
	return ( welt->get_viewport()->is_on_center( get_weltpos(false) ) );
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
		const scr_coord offset = chart_button.pressed ? scr_coord(0, chart.get_size().h - 6) : scr_coord(0, -(chart.get_size().h - 6));
		set_min_windowsize(get_min_windowsize() + offset);
		chart_button.set_pos( chart_button.get_pos() + offset );
		details_button.set_pos( details_button.get_pos() + offset );
		scrolly.set_pos( scrolly.get_pos() + offset );
		resize( scr_coord(0,(chart_button.pressed ? chart.get_size().h : -chart.get_size().h) ) );
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
		welt->get_viewport()->change_world_position( koord3d(k,welt->max_hgt(k)) );
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
			b->set_pos(scr_coord(D_MARGIN_LEFT, y_off));
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

	fab_info.fab = fab;
	fab_info.remove_all();

	// needs to update all text
	fab_info.set_pos( scr_coord(0,0) );
	txt.set_pos( scr_coord(D_MARGIN_LEFT,0) );
	fab_info.add_komponente(&txt);

	int y_off = LINESPACE;
	make_buttons(lieferbuttons,   fab->get_lieferziele(),   y_off, fab_info, this);
	make_buttons(supplierbuttons, fab->get_suppliers(),     y_off, fab_info, this);
	make_buttons(stadtbuttons,    fab->get_target_cities(), y_off, fab_info, this);

	fab_info.set_size( scr_size( fab_info.get_size().w, txt.get_size().h ) );
}


// component for city demand icons display
void gui_fabrik_info_t::draw(scr_coord offset)
{
	int xoff = pos.x+offset.x+D_MARGIN_LEFT+16;
	int yoff = pos.y+offset.y;

	gui_container_t::draw( offset );

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
			if(  const fabrik_t *src = fabrik_t::get_fab(k)  ) {
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


/***************** Saveload stuff from here *****************/


fabrik_info_t::fabrik_info_t() :
	gui_frame_t("", welt->get_spieler(1)),
	fab(NULL),
	chart(NULL),
	view(scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width() * 7) / 8))),
	scrolly(&fab_info),
	prod(&prod_buf),
	txt(&info_buf)
{
	lieferbuttons = supplierbuttons = stadtbuttons = NULL;

	input.set_pos(scr_coord(D_MARGIN_LEFT,D_MARGIN_TOP));
	input.set_text( fabname, lengthof(fabname) );
	input.add_listener(this);
	add_komponente(&input);

	add_komponente(&view);

	prod.set_pos( scr_coord( D_MARGIN_LEFT, D_MARGIN_TOP+D_BUTTON_HEIGHT+D_V_SPACE ) );
	add_komponente( &prod );

	chart_button.init(button_t::roundbox_state, "Chart", scr_coord(BUTTON3_X,0), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	chart_button.set_tooltip("Show/hide statistics");
	chart_button.add_listener(this);
	add_komponente(&chart_button);

	add_komponente(&scrolly);

	const sint16 total_width = D_MARGIN_LEFT + 3*(D_BUTTON_WIDTH + D_H_SPACE) + max( D_BUTTON_WIDTH, view.get_size().w ) + D_MARGIN_RIGHT;
	set_min_windowsize(scr_size(total_width, D_TITLEBAR_HEIGHT+scrolly.get_pos().y+LINESPACE*5+D_MARGIN_BOTTOM));

	set_resizemode(diagonal_resize);
}


void fabrik_info_t::rdwr( loadsave_t *file )
{
	scr_size size = get_windowsize();
	sint32 scroll_x = scrolly.get_scroll_x();
	sint32 scroll_y = scrolly.get_scroll_y();
	koord fabpos;
	scr_coord viewpos = view.get_pos();
	scr_size viewsize = view.get_size();
	scr_coord scrollypos = scrolly.get_pos();

	if(  file->is_saving()  ) {
		fabpos = fab->get_pos().get_2d();
	}

	size.rdwr( file );
	fabpos.rdwr( file );
	viewpos.rdwr( file );
	viewsize.rdwr( file );
	scrollypos.rdwr( file );
	file->rdwr_str( fabname, lengthof(fabname) );
	file->rdwr_long( scroll_x );
	file->rdwr_long( scroll_y );
	file->rdwr_bool( chart_button.pressed );

	if(  file->is_loading()  ) {
		fab = fabrik_t::get_fab(fabpos );

		// will fail on factories with no ground or no building at (0,0)
		view.set_obj( welt->lookup_kartenboden( fabpos )->find<gebaeude_t>() );
		view.set_size( viewsize );
		view.set_pos( viewpos );
		chart.set_factory( fab );
		chart.rdwr( file );

		// now put stuff at old positions
		fab->info_prod( prod_buf );
		prod.recalc_size();

		const sint16 offset_below_viewport = D_MARGIN_TOP+D_BUTTON_HEIGHT+D_V_SPACE+ max( prod.get_size().h, view.get_size().h + 8 + D_V_SPACE );
		chart.set_pos( scr_coord(0, offset_below_viewport - 5) );
		chart_button.set_pos( scr_coord(BUTTON3_X,offset_below_viewport) );

		// calculate height
		fab->info_prod(prod_buf);

		// fill position buttons etc
		fab->info_conn(info_buf);
		txt.recalc_size();
		update_info();

		scrolly.set_pos( scrollypos );
		set_min_windowsize(scr_size(get_min_windowsize().w, D_TITLEBAR_HEIGHT+scrollypos.y+LINESPACE*5+D_MARGIN_BOTTOM));

		// Hajo: "About" button only if translation is available
		char key[256];
		sprintf(key, "factory_%s_details", fab->get_besch()->get_name());
		const char * value = translator::translate(key);
		if(value && *value != 'f') {
			details_button.init( button_t::roundbox, "Details", scr_coord(BUTTON4_X,offset_below_viewport), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
//			details_button.set_tooltip("Factory details");
			details_button.add_listener(this);
			add_komponente(&details_button);
		}

		if(  chart_button.pressed  ) {
			add_komponente( &chart );
			const scr_coord offset = scr_coord(0, chart.get_size().h - 6);
			chart_button.set_pos( chart_button.get_pos() + offset );
			details_button.set_pos( details_button.get_pos() + offset );
		}
		set_windowsize( size );
		resize( scr_coord(0,0) );
		scrolly.set_scroll_position( scroll_x, scroll_y );
	}
	else {
		chart.rdwr( file );
	}
}
