/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * Maintains a scrollable list of custom labels on the screen (may be used as jump points)
 */

#include <string.h>

#include "../simdebug.h"
#include "label_frame.h"
#include "components/gui_button.h"
#include "../simwin.h"
#include "../simintr.h"

#include "../simworld.h"
#include "../simplay.h"

#include "../boden/grund.h"

#include "../dataobj/translator.h"
#include "../dings/label.h"
#include "../utils/simstring.h"


label_frame_t *label_frame_t::instance = NULL;
int label_frame_t::window_height = 16+70+100+10;

label_frame_t::label_frame_t(karte_t *welt, spieler_t *sp, koord pos) :
	gui_frame_t("Marker"),
	scrolly(&button_frame),
	button_frame(),
	fnlabel("Filename")
{
	destroy_win(instance);
	instance = this;

	this->welt = welt;
	this->pos = pos;
	this->sp = sp;

	scrolly.setze_pos(koord(1, 70));
	scrolly.setze_groesse(koord(175+26*2-10, 100));
	scrolly.set_show_scroll_x(false);
	scrolly.set_size_corner(false);

	int y = 0;
	slist_iterator_tpl <koord> iter (welt->gib_label_list());
	while(iter.next()) {
		button_t * button = new button_t();
		koord my_pos = iter.get_current();
		label_t *l = (label_t *)welt->lookup_kartenboden(my_pos)->suche_obj(ding_t::label);

		if(l) {
			const char* text = welt->lookup(my_pos)->gib_kartenboden()->gib_text();
			if(text==NULL) {
				text = "";
			}
			char *my_text = new char[strlen(text) + 16];

			sprintf(my_text, "(%d,%d) %s", my_pos.x, my_pos.y, text);

			button->setze_text(my_text);
			buttons.insert(button);
			button->setze_groesse(koord(187, 14));
			button->setze_pos(koord(10,y));

			button->add_listener(this);
			button_frame.add_komponente(button);

			y+=14;
		}
	}
	button_frame.setze_groesse(koord(175+26*2-10, y));
	add_komponente(&scrolly);

	grund_t *gr = welt->lookup_kartenboden(pos);
	if (gr != NULL) {
		const ding_t* thing = gr->obj_bei(0);
		if (thing == NULL || (
					(thing->gib_besitzer() == NULL && thing->gib_typ() != ding_t::gebaeude) ||
					thing->gib_besitzer() == sp
				)) {
			sprintf(txlabel, "(%d,%d) ", pos.x, pos.y);

			// Text
			fnlabel.setze_pos (koord(10, 12));
			fnlabel.setze_text (txlabel);
			add_komponente(&fnlabel);

			// Input box for new name
			tstrncpy(ibuf, "", 1);
			load_label(ibuf);
			input.setze_text(ibuf, 58);
			input.add_listener(this);
			input.setze_pos(koord(75,8));
			input.setze_groesse(koord(140, 14));
			add_komponente(&input);

			if(!gr->gib_halt().is_bound()) {
				removebutton.setze_pos(koord(80,50));
				removebutton.setze_groesse(koord(65, 14));
				removebutton.setze_text("Remove");
				removebutton.setze_typ(button_t::roundbox);
				removebutton.add_listener(this);
				add_komponente(&removebutton);
			}
		}
	}
	else {
		// Text
		fnlabel.setze_text ("Das Feld gehoert\neinem anderen Spieler\n");
		add_komponente(&fnlabel);
	}

	divider1.setze_pos(koord(10,40));
	divider1.setze_groesse(koord(205,0));
	add_komponente(&divider1);

	savebutton.setze_pos(koord(10,50));
	savebutton.setze_groesse(koord(65, 14));
	savebutton.setze_text("Ok");
	savebutton.setze_typ(button_t::roundbox);
	savebutton.add_listener(this);
	add_komponente(&savebutton);

	cancelbutton.setze_pos(koord(150,50));
	cancelbutton.setze_groesse(koord(65, 14));
	cancelbutton.setze_text("Cancel");
	cancelbutton.setze_typ(button_t::roundbox);
	cancelbutton.add_listener(this);
	add_komponente(&cancelbutton);

	setze_opaque(true);

	set_resizemode(vertical_resize);
	setze_fenstergroesse(koord(175+26*2, window_height));
	set_min_windowsize(koord(175+26*2, 16+70+100+10));

	resize(koord(0,0));
}



label_frame_t::~label_frame_t()
{
	slist_iterator_tpl <button_t *> iter (buttons);

	while(iter.next()) {
		delete const_cast<char *>(iter.get_current()->gib_text());
		delete iter.get_current();
	}
	instance = NULL;
}



/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void label_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta-koord(delta.x,0));
	window_height = gib_fenstergroesse().y;
	koord groesse = gib_fenstergroesse()-koord(10,16+70+10);
	scrolly.setze_groesse(groesse);
}



/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool label_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	if(komp == &input || komp == &savebutton) {
		// OK- Button or Enter-Key pressed
		//---------------------------------------
		create_label(ibuf);
		destroy_win(this);
	} else if(komp == &removebutton) {
		remove_label();
		destroy_win(this);
	} else if(komp == &cancelbutton) {
		// Cancel-button pressed
		//-----------------------------
		destroy_win(this);
	} else {
		// mark in list selected
		//--------------------------
		slist_iterator_tpl <button_t *> iter (buttons);
		while(iter.next()) {
			if(komp == iter.get_current()) {
				goto_label(iter.get_current()->gib_text());
				destroy_win(this);
				intr_refresh_display( true );
				break;
			}
		}
	}
	return true;
}



void label_frame_t::load_label(char *name)
{
	name[0] = 0;
	grund_t *gr = welt->lookup(pos)->gib_kartenboden();
	if (gr != NULL && gr->gib_text()) {
		const ding_t* thing = gr->obj_bei(0);
		if (thing != NULL) {
			const spieler_t* owner = thing->gib_besitzer();
			if (owner == NULL || owner == sp) {
				tstrncpy(name, gr->gib_text(), 64);
			}
		}
	}
}



void label_frame_t::create_label(const char *name)
{
	grund_t *gr = welt->lookup(pos)->gib_kartenboden();
	if(gr) {
		label_t *l = (label_t *)gr->suche_obj(ding_t::label);
		if (l == NULL) {
			if (gr->gib_text() != NULL) return;
			const ding_t* thing = gr->obj_bei(0);
			if (thing == NULL || (thing->gib_besitzer() == NULL || thing->gib_besitzer() == sp)) {
				// label as ground owning indicator
				// always there but only shown if tile is empty but for trees etc.
				gr->obj_add(new label_t(welt, gr->gib_pos(), sp, name));
			}
		} else {
			if (l->gib_besitzer() == sp) {
				// just change text
				free(const_cast<char*>(gr->gib_text()));
				gr->setze_text(strdup(name));
			}
		}
	}
}



void label_frame_t::remove_label()
{
	grund_t *gr = welt->lookup(pos)->gib_kartenboden();
	if(gr  &&  gr->suche_obj(ding_t::label)) {
		delete (label_t *)gr->suche_obj(ding_t::label);
	}
}

void label_frame_t::goto_label(const char *name)
{
	koord my_pos;
	sscanf(name, "(%hd,%hd)", &my_pos.x, &my_pos.y);
	if(welt->ist_in_kartengrenzen(my_pos)) {
		welt->setze_ij_off(koord3d(my_pos,welt->min_hgt(my_pos)));
	}
}
