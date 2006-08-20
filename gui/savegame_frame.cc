/*
 * savegame_frame.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#else
#include <io.h>
#include <direct.h>
#endif
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "../pathes.h"

#include "../simdebug.h"
#include "savegame_frame.h"
#include "button.h"
#include "../simwin.h"
#include "../simintr.h"

#include "../dataobj/translator.h"              // 29-Oct-2001       Markus Weber    Added
#include "../utils/simstring.h"



/**
 * Konstruktor.
 * @author Hj. Malthaner
 */
savegame_frame_t::savegame_frame_t(const char *suffix) : gui_frame_t("Load/Save") , fnlabel("Filename")
{
    this->suffix = suffix;

#ifdef __MINGW32__
    mkdir(SAVE_PATH);
#else
    mkdir(SAVE_PATH, 0700);
#endif

#ifndef _MSC_VER
    // find filenames
    DIR     *dir;                      /* Schnittstellen  zum BS */
    struct  dirent  *entry;

    dir=opendir(SAVE_PATH_X ".");

    if(dir==NULL) {
	dbg->warning("savegame_frame_t::savegame_frame_t()",
                     "Couldn't read directory.");
    } else {
	do {
	    entry=readdir(dir);
	    if(entry!=NULL) {
		if(entry->d_name[0]!='.' ||
		   (entry->d_name[1]!='.' && entry->d_name[1]!=0)) {
		    if(strncmp(entry->d_name + strlen(entry->d_name) - 4, suffix, 4) == 0) {
			add_file(entry->d_name);
		    }
		}
	    }
	}while(entry!=NULL);
	closedir(dir);
    }
#else
    struct _finddata_t entry;
    long hfind;

    char wild[32];
    tstrncpy(wild, SAVE_PATH_X "*", 32);
    strcat(wild, suffix);

    hfind = _findfirst( wild, &entry);
    if(hfind == -1) {
	dbg->warning("savegame_frame_t::savegame_frame_t()",
                     "Couldn't read directory.");
    } else {
        do{
	    add_file(entry.name);
        } while(_findnext(hfind, &entry) == 0 );
    }
#endif

    // Text 'Game name'
    fnlabel.setze_pos (koord(10,8));
    fnlabel.setze_text ( translator::translate("Filename"));
    add_komponente(&fnlabel);

    // Input box for game name
    tstrncpy(ibuf, "", 1);
    input.setze_text(ibuf, 58);
    input.add_listener(this);
    input.setze_pos(koord(75,8));
    input.setze_groesse(koord(140, 14));

    add_komponente(&input);

    // The file entries
    slist_iterator_tpl <button_t *> iter1(deletes);
    slist_iterator_tpl <button_t *> iter2(buttons);
    slist_iterator_tpl <gui_label_t *> iter3(labels);
    int y = 30;

    while(iter1.next() && iter2.next() && iter3.next()) {
	button_t * button1 = iter1.get_current();
	button_t * button2 = iter2.get_current();
	gui_label_t * label = iter3.get_current();

	button1->setze_groesse(koord(14, 14));
	button1->setze_text("X");
	button1->setze_pos(koord(5, y));
	button1->set_tooltip(translator::translate("Delete this file."));

	button2->setze_pos(koord(25, y));
	button2->setze_groesse(koord(140, 14));

	label->setze_pos(koord(170, y));

	button1->add_listener(this);
	button2->add_listener(this);

	add_komponente(button1);
	add_komponente(button2);
	add_komponente(label);

	y += 14;
    }

    y += 10;
    divider1.setze_pos(koord(10,y));
    divider1.setze_groesse(koord(245,0));
    add_komponente(&divider1);

    y += 10;
    savebutton.setze_pos(koord(10,y));
    savebutton.setze_groesse(koord(65, 14));
    savebutton.text = translator::translate("Ok");
    savebutton.setze_typ(button_t::roundbox);
    savebutton.add_listener(this);
    add_komponente(&savebutton);

    cancelbutton.setze_pos(koord(190,y));
    cancelbutton.setze_groesse(koord(65, 14));
    cancelbutton.text = translator::translate("Cancel");
    cancelbutton.setze_typ(button_t::roundbox);
    cancelbutton.add_listener(this);
    add_komponente(&cancelbutton);

    setze_opaque(true);
    setze_fenstergroesse(koord(175+26*2+56, y + 40));
}


savegame_frame_t::~savegame_frame_t()
{
    slist_iterator_tpl <button_t *> b_iter (buttons);

    while(b_iter.next()) {
	delete [] const_cast<char *>(b_iter.get_current()->gib_text());
	delete b_iter.get_current();
    }
    slist_iterator_tpl <gui_label_t *> l_iter (labels);

    while(l_iter.next()) {
	delete [] const_cast<char *>(l_iter.get_current()->gib_text());
	delete l_iter.get_current();
    }
    slist_iterator_tpl <button_t *> s_iter (deletes);

    while(s_iter.next()) {
	delete s_iter.get_current();
    }
}

void savegame_frame_t::add_file(const char *filename)
{
    button_t * button = new button_t();
    char * name = new char [strlen(filename)+10];
    char * date = new char[18];

    sprintf(name, SAVE_PATH_X "%s", filename);

    struct stat sb;
    stat(name, &sb);
    struct tm *tm = localtime(&sb.st_mtime);
    if(tm) {
	strftime(date, 18, "%Y-%m-%d %H:%M", tm);
    } else {
	tstrncpy(date, "??.??.???? ??:??", 15);
    }

    sprintf(name, "%s", filename);
    name[strlen(name)-4] = '\0';
    button->setze_text(name);

    unsigned int i;

    // sort by date descending:
    for(i = 0; i < buttons.count(); i++) {
	if(strcmp(labels.at(i)->gib_text(), date) < 0) {
	    break;
	}
    }
    buttons.insert(button, i);
    labels.insert(new gui_label_t(date), i);
    deletes.insert(new button_t, i);
}

/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool savegame_frame_t::action_triggered(gui_komponente_t *komp)
{
    char buf[128];
    //if(komp == &input) {       //29-Oct-2001         Markus Weber    Added   savebutton case

    if(komp == &input || komp == &savebutton) {

        // Save/Load Button or Enter-Key pressed
        //---------------------------------------

        destroy_win(this);      //29-Oct-2001         Markus Weber    Close window

	tstrncpy(buf, SAVE_PATH_X, 128);
	strcat(buf, ibuf);
	strcat(buf, suffix);
	action(buf);

    } else if(komp == &cancelbutton) {

        // Cancel-button pressed
        //-----------------------------
        destroy_win(this);      //29-Oct-2001         Markus Weber    Added   savebutton case

    } else {

        // File in list selected
        //--------------------------

	slist_iterator_tpl <button_t *> iter (buttons);
	slist_iterator_tpl <button_t *> iter2 (deletes);

	while(iter.next()) {
	    iter2.next();
	    if(komp == iter.get_current() || komp == iter2.get_current()) {
		destroy_win(this);
		intr_refresh_display( true );

		tstrncpy(buf, SAVE_PATH_X, 128);
		strcat(buf, iter.get_current()->gib_text());
		strcat(buf, suffix);

		if(komp == iter.get_current()) {
		    action(buf);
		} else {
		    del_action(buf);
		}
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
void savegame_frame_t::zeichnen(koord pos, koord gr)
{

    fnlabel.setze_text ( translator::translate("Filename"));
    savebutton.text = translator::translate("Ok");
    cancelbutton.text = translator::translate("Cancel");
    gui_frame_t::zeichnen(pos, gr);
}
