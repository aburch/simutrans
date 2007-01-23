/*
 * fabrik_info.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "fabrik_info.h"

#include "components/gui_button.h"
#include "help_frame.h"

#include "../simfab.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simcity.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../dataobj/translator.h"

cbuffer_t fabrik_info_t::info_buf(8192);

fabrik_info_t::fabrik_info_t(fabrik_t *fab, gebaeude_t *gb, karte_t *welt) :
	ding_infowin_t(welt,gb),
	scrolly(&cont),
	txt("\n")
{
	this->fab = fab;
	this->welt = welt;
	this->about = 0;

	txt.setze_pos(koord(16,-15));
	txt.setze_text(info_buf);
	cont.add_komponente(&txt);

	view.set_location(gb->gib_pos());

	const vector_tpl <koord> & lieferziele =  fab->gib_lieferziele();
#ifdef _MSC_VER
	// V.Meyer: MFC has a bug with "new x[0]"
	lieferbuttons = new button_t [lieferziele.get_count()+1];
#else
	lieferbuttons = new button_t [lieferziele.get_count()];
#endif
	{
	for(unsigned i=0; i<lieferziele.get_count(); i++) {
		lieferbuttons[i].setze_pos(koord(16, 50+i*11));
		lieferbuttons[i].setze_typ(button_t::arrowright);
		lieferbuttons[i].add_listener(this);
		cont.add_komponente(&lieferbuttons[i]);
	}
	}
	int y_off = lieferziele.get_count() ? (int)lieferziele.get_count()*11-11 : -33;

	const vector_tpl <koord> & suppliers =  fab->get_suppliers();
#ifdef _MSC_VER
	// V.Meyer: MFC has a bug with "new x[0]"
	supplierbuttons = new button_t [1+suppliers.get_count()];
#else
	supplierbuttons = new button_t [suppliers.get_count()];
#endif
	{
	for(unsigned i=0; i<suppliers.get_count(); i++) {
		supplierbuttons[i].setze_pos(koord(16, 83+y_off+i*11));
		supplierbuttons[i].setze_typ(button_t::arrowright);
		supplierbuttons[i].add_listener(this);
		cont.add_komponente(&supplierbuttons[i]);
	}
	}
	int yy_off = suppliers.get_count() ? (int)suppliers.get_count()*11-11 : -33;
	y_off += yy_off;

	const slist_tpl <stadt_t *> & arbeiterziele = fab->gib_arbeiterziele();
#ifdef _MSC_VER
	// V.Meyer: MFC has a bug with "new x[0]"
	stadtbuttons = new button_t [arbeiterziele.count()+1];
#else
	stadtbuttons = new button_t [arbeiterziele.count()];
#endif
	for(unsigned i=0; i<arbeiterziele.count(); i++) {
		stadtbuttons[i].setze_pos(koord(16, 116+y_off+i*11));
		stadtbuttons[i].setze_typ(button_t::arrowright);
		stadtbuttons[i].add_listener(this);
		cont.add_komponente(&stadtbuttons[i]);
	}

	// Hajo: "About" button only if translation is available
	char key[256];
	sprintf(key, "factory_%s_details", fab->gib_name());
	const char * value = translator::translate(key);
	about = new button_t();
	if(value && *value != 'f') {
		about->init(button_t::roundbox,translator::translate("About"),koord(266 - 64, 6+64+10),koord(64, 14));
		about->add_listener(this);
		add_komponente(about);
	}

	// calculate height
	info_buf.clear();
	fab->info(info_buf);
	int  height = max(count_char(buf, '\n')*LINESPACE+40, get_tile_raster_width()+30 );

	setze_opaque(true);
	setze_fenstergroesse(koord((short)290, min(height, 408)));
	cont.setze_groesse(koord((short)290, height-20));

	scrolly.set_show_scroll_x(false);
	scrolly.set_size_corner(false);
	scrolly.setze_groesse(gib_fenstergroesse()-koord(1,16));
	add_komponente(&scrolly);

	view.setze_pos(koord(266 - 64, 8));	// view is actually borrowed from ding-info ...
	view.setze_groesse( koord(64,56) );
	add_komponente(&view);

	gui_frame_t::set_owner( fab->gib_besitzer() );
	setze_name( fab->gib_name() );
}


fabrik_info_t::~fabrik_info_t()
{
	delete [] lieferbuttons;
	delete [] supplierbuttons;
	delete [] stadtbuttons;
	delete about;
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
	info_buf.clear();
	fab->info( info_buf );

	gui_frame_t::zeichnen(pos,gr);

	unsigned indikatorfarbe = fabrik_t::status_to_color[ fab->calc_factory_status( NULL, NULL ) ];
	display_ddd_box_clip(pos.x + view.pos.x, pos.y + view.pos.y + 75, 64, 8, MN_GREY0, MN_GREY4);
	display_fillbox_wh_clip(pos.x + view.pos.x+1, pos.y + view.pos.y + 76, 62, 6, indikatorfarbe, true);
	if (fab->get_prodfaktor() > 16) {
		display_color_img(skinverwaltung_t::electricity->gib_bild_nr(0), pos.x + view.pos.x+4, pos.y + view.pos.y+18, 0, false, false);
	}
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 * V.Meyer
   */
bool fabrik_info_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	unsigned int i;

	for(i=0; i<fab->gib_lieferziele().get_count(); i++) {
		if(komp == &lieferbuttons[i]) {
			welt->setze_ij_off(koord3d(fab->gib_lieferziele()[i], welt->max_hgt(fab->gib_lieferziele()[i])));
		}
	}

	for(i=0; i<fab->get_suppliers().get_count(); i++) {
		if(komp == &supplierbuttons[i]) {
			welt->setze_ij_off(koord3d(fab->get_suppliers()[i],welt->max_hgt(fab->get_suppliers()[i])));
		}
	}

	for(i=0; i<fab->gib_arbeiterziele().count(); i++) {
		if(komp == &stadtbuttons[i]) {
			welt->setze_ij_off(koord3d(fab->gib_arbeiterziele().at(i)->gib_pos(),welt->min_hgt(fab->gib_arbeiterziele().at(i)->gib_pos())));
		}
	}

	if(komp == about) {
		help_frame_t * frame = new help_frame_t();
		char key[256];
		sprintf(key, "factory_%s_details", fab->gib_name());
		frame->setze_text(translator::translate(key));
		create_win(frame, w_autodelete, magic_none);
	}

	return true;
}
