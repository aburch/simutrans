/*
 * savegame_frame.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
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
#include "../utils/simstring.h"


label_frame_t *label_frame_t::instance = NULL;

/**
 * Konstruktor.
 * @author V. Meyer
 */
label_frame_t::label_frame_t(karte_t *welt, spieler_t *sp, koord pos) : gui_frame_t("Marker") , fnlabel("Filename")
{
    int i=0;

    destroy_win(instance);
    instance = this;

    slist_iterator_tpl <koord> iter (welt->gib_label_list());

    while(iter.next()) {
	button_t * button = new button_t();
	koord my_pos = iter.get_current();
	const char * text = welt->lookup(my_pos)->gib_kartenboden()->gib_text();

	if(text) {
	    char *my_text = new char[strlen(text) + 16];

	    sprintf(my_text, "(%d,%d) %s", my_pos.x, my_pos.y, text);

	    button->setze_text(my_text);
	    buttons.insert(button);
	    button->setze_groesse(koord(205, 14));
	    button->setze_pos(koord(10,30+14*i));

	    button->add_listener(this);
	    add_komponente(button);
	    i++;
	}
    }

    this->welt = welt;
    this->pos = pos;
    this->sp = sp;

    grund_t *gr = welt->lookup(pos)->gib_kartenboden();
    if(gr && (gr->gib_besitzer() == NULL || gr->gib_besitzer() == sp)) {
	sprintf(txlabel, "(%d,%d) ", pos.x, pos.y);

        // Text
	fnlabel.setze_pos (koord(10,12));
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
	    removebutton.setze_pos(koord(80,50+14*i));
	    removebutton.setze_groesse(koord(65, 14));
	    removebutton.setze_text("Remove");
	    removebutton.setze_typ(button_t::roundbox);
	    removebutton.add_listener(this);
	    add_komponente(&removebutton);
	}
    }
    else {
        // Text
        fnlabel.setze_text ("Das Feld gehoert\neinem anderen Spieler\n");
	add_komponente(&fnlabel);

    }
    divider1.setze_pos(koord(10,40+14*i));
    divider1.setze_groesse(koord(205,0));
    add_komponente(&divider1);

    savebutton.setze_pos(koord(10,50+14*i));
    savebutton.setze_groesse(koord(65, 14));
    savebutton.setze_text("Ok");
    savebutton.setze_typ(button_t::roundbox);
    savebutton.add_listener(this);
    add_komponente(&savebutton);


    cancelbutton.setze_pos(koord(150,50+14*i));
    cancelbutton.setze_groesse(koord(65, 14));
    cancelbutton.setze_text("Cancel");
    cancelbutton.setze_typ(button_t::roundbox);
    cancelbutton.add_listener(this);
    add_komponente(&cancelbutton);

    setze_opaque(true);
    setze_fenstergroesse(koord(175+26*2, 90+i*14));
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


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Markus Weber
 */
void label_frame_t::zeichnen(koord pos, koord gr)
{
	fnlabel.setze_text (txlabel);
	savebutton.setze_text("Ok");
	cancelbutton.setze_text("Cancel");
	gui_frame_t::zeichnen(pos, gr);
}



void label_frame_t::load_label(char *name)
{
	grund_t *gr = welt->lookup(pos)->gib_kartenboden();

	if(gr &&  (gr->gib_besitzer() == NULL || gr->gib_besitzer() == sp)  &&  gr->gib_text()) {
		tstrncpy(name, gr->gib_text(), 64);
	}
}

void label_frame_t::create_label(const char *name)
{
	grund_t *gr = welt->lookup(pos)->gib_kartenboden();

	if(gr && strlen(name) &&
		(gr->gib_besitzer() == NULL || gr->gib_besitzer() == sp)) {

		if(gr->gib_besitzer() == NULL) {
			sp->buche(-12500, pos, COST_CONSTRUCTION);
		}
		gr->setze_besitzer(sp);
		gr->setze_text(strdup(name));
		if(!gr->gib_halt().is_bound()) {
			welt->add_label(pos);
		}
	}
}

void label_frame_t::remove_label()
{
	grund_t *gr = welt->lookup(pos)->gib_kartenboden();

	if(gr && (gr->gib_besitzer() == NULL || gr->gib_besitzer() == sp)) {
		gr->setze_besitzer(NULL);
		free(const_cast<char *>(gr->gib_text()));
		gr->setze_text(NULL);
		welt->remove_label(pos);
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
