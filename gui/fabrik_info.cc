/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "fabrik_info.h"

#include "components/gui_button.h"
#include "help_frame.h"

#include "../simfab.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simcity.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"


cbuffer_t fabrik_info_t::info_buf(8192);

fabrik_info_t::fabrik_info_t(const fabrik_t* fab_, const gebaeude_t* gb) :
	ding_infowin_t(gb),
	fab(fab_),
	scrolly(&cont),
	txt("\n")
{
	about = lieferbuttons = supplierbuttons = stadtbuttons = NULL;

	const sint16 width = max(290, 226+view.get_groesse().x);

	// Hajo: "About" button only if translation is available
	char key[256];
	sprintf(key, "factory_%s_details", fab->get_besch()->get_name());
	const char * value = translator::translate(key);
	if(value && *value != 'f') {
		about = new button_t();
		about->init(button_t::roundbox,translator::translate("About"),koord(width - view.get_groesse().x - 20, view.get_groesse().y + 18), koord(view.get_groesse().x, 14));
		about->add_listener(this);
		add_komponente(about);
	}

	// fill position buttons etc
	update_info();

	// calculate height
	info_buf.clear();
	fab->info(info_buf);

	// check, if something changed ...
	const sint16 height = max(count_char(info_buf, '\n')*LINESPACE+36, view.get_groesse().y+8+14+36 );
	set_fenstergroesse(koord(width, min(height+10, 408)));
	cont.set_groesse(koord(width, height-10));

	scrolly.set_show_scroll_x(false);
	scrolly.set_size_corner(false);
	scrolly.set_groesse(get_fenstergroesse()-koord(1,16));
	add_komponente(&scrolly);

	view.set_pos(koord(width - view.get_groesse().x - 20, 10));	// view is actually borrowed from ding-info ...
	add_komponente(&view);

	gui_frame_t::set_owner( fab->get_besitzer() );
	set_name( fab->get_name() );
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
 * 	
 * component redraw. The given values refer to the window, i.e., it
 * is the screen coordinates of the window in which the component is shown.
 *
 * @author Hj. Malthaner
 */
void fabrik_info_t::zeichnen(koord pos, koord gr)
{
	buf.clear();	// clear the buffer of the base class
	info_buf.clear();
	fab->info( info_buf );
	const sint16 height = max(count_char(info_buf, '\n')*LINESPACE+36, view.get_groesse().y+8+14+36 );
	if((cont.get_groesse().y+10)!=height) {
		update_info();
		cont.set_groesse(koord(cont.get_groesse().x, height-10));
		set_fenstergroesse(koord(get_fenstergroesse().x, min(height+10, 408)));
		scrolly.set_groesse(get_fenstergroesse()-koord(1,16));
	}

	gui_frame_t::zeichnen(pos,gr);

	unsigned indikatorfarbe = fabrik_t::status_to_color[fab->get_status()];
	display_ddd_box_clip(pos.x + view.get_pos().x, pos.y + view.get_pos().y + view.get_groesse().y + 16, view.get_groesse().x, 8, MN_GREY0, MN_GREY4);
	display_fillbox_wh_clip(pos.x + view.get_pos().x + 1, pos.y + view.get_pos().y + view.get_groesse().y + 17, view.get_groesse().x - 2, 6, indikatorfarbe, true);
	if (fab->get_prodfaktor() > 16) {
		display_color_img(skinverwaltung_t::electricity->get_bild_nr(0), pos.x + view.get_pos().x + 4, pos.y + view.get_pos().y + 20, 0, false, false);
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
bool fabrik_info_t::action_triggered( gui_action_creator_t *komp, value_t v)
{
	karte_t* welt = ding->get_welt();

	if(komp == about) {
		help_frame_t * frame = new help_frame_t();
		char key[256];
		sprintf(key, "factory_%s_details", fab->get_besch()->get_name());
		frame->set_text(translator::translate(key));
		create_win(frame, w_info, (long)this);
	}
	else if(v.i&~1) {
		koord k = *(const koord *)v.p;
		welt->change_world_position( koord3d(k,welt->max_hgt(k)) );
	}

	return true;
}



void fabrik_info_t::update_info()
{
	cont.remove_all();

	delete [] lieferbuttons;
	delete [] supplierbuttons;
	delete [] stadtbuttons;

	// needs to update all text
	txt.set_pos(koord(16,4));
	txt.set_text(info_buf);
	cont.add_komponente(&txt);

	const vector_tpl <koord> & lieferziele =  fab->get_lieferziele();
#ifdef _MSC_VER
	// V.Meyer: MFC has a bug with "new x[0]"
	lieferbuttons = new button_t [lieferziele.get_count()+1];
#else
	lieferbuttons = new button_t [lieferziele.get_count()];
#endif
	
	const uint8 x = fab->get_city() == NULL ? 1 : 3;
	
	for(unsigned i=0; i<lieferziele.get_count(); i++) {
		lieferbuttons[i].set_pos(koord(16, 46+(i+x)*LINESPACE));
		lieferbuttons[i].set_typ(button_t::posbutton);
		lieferbuttons[i].set_targetpos(lieferziele[i]);
		lieferbuttons[i].add_listener(this);
		cont.add_komponente(&lieferbuttons[i]);
	}

	int y_off = (lieferziele.get_count() ? (int)lieferziele.get_count()-1 : -3)*LINESPACE;
	const vector_tpl <koord> & suppliers =  fab->get_suppliers();
#ifdef _MSC_VER
	// V.Meyer: MFC has a bug with "new x[0]"
	supplierbuttons = new button_t [suppliers.get_count()+1];
#else
	supplierbuttons = new button_t [suppliers.get_count()];
#endif
	
	for(unsigned i=0; i<suppliers.get_count(); i++) {
		supplierbuttons[i].set_pos(koord(16, 79+y_off+(i+x)*LINESPACE));
		supplierbuttons[i].set_typ(button_t::posbutton);
		supplierbuttons[i].set_targetpos(suppliers[i]);
		supplierbuttons[i].add_listener(this);
		cont.add_komponente(&supplierbuttons[i]);
	}

	int yy_off = (suppliers.get_count() ? (int)suppliers.get_count()-1 : -3)*LINESPACE;
	y_off += yy_off;
	const slist_tpl <stadt_t *> & arbeiterziele = fab->get_arbeiterziele();
#ifdef _MSC_VER
	// V.Meyer: MFC has a bug with "new x[0]"
	stadtbuttons = new button_t [arbeiterziele.get_count()+1];
#else
	stadtbuttons = new button_t [arbeiterziele.get_count()];
#endif
	for(unsigned i=0; i<arbeiterziele.get_count(); i++) {
		stadtbuttons[i].set_pos(koord(16, 112+y_off+(i+x)*LINESPACE));
		stadtbuttons[i].set_typ(button_t::posbutton);
		stadtbuttons[i].set_targetpos(arbeiterziele.at(i)->get_pos());
		stadtbuttons[i].add_listener(this);
		cont.add_komponente(&stadtbuttons[i]);
	}
}
