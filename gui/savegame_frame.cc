/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#else
#include <io.h>
#include <direct.h>
#endif
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include "../pathes.h"

#include "../simdebug.h"
#include "../simwin.h"
#include "../simintr.h"
#include "../utils/simstring.h"

#include "components/list_button.h"
#include "savegame_frame.h"

#define DIALOG_WIDTH (360)


// we need this trick, with the function pointer.
// Since during initialisations virtual functions do not work yet
// in derived classes (since the object in question is not full initialized yet)
// this functions returns true for files to be added.
savegame_frame_t::savegame_frame_t(const char *suffix, const char *path ) :
	gui_frame_t("Load/Save") ,
	fnlabel("Filename"),
	scrolly(&button_frame)
{
	this->suffix = suffix;
	this->fullpath = path;
	use_pak_extension = suffix==NULL  ||  strcmp( suffix, ".sve" )==0;

	// both NULL is not acceptable
	assert(suffix!=path);

	fnlabel.setze_pos (koord(10,12));
	add_komponente(&fnlabel);

	// Input box for game name
	tstrncpy(ibuf, "", lengthof(ibuf));
	input.setze_text(ibuf, 58);
	input.add_listener(this);
	input.setze_pos(koord(75,8));
	input.setze_groesse(koord(DIALOG_WIDTH-75-10-10, 14));
	add_komponente(&input);

	// needs to be scrollable
	scrolly.setze_pos( koord(0,30) );
	scrolly.set_show_scroll_x(false);
	scrolly.set_size_corner(false);
	scrolly.setze_groesse( koord(DIALOG_WIDTH-12,30) );

	// The file entries
	int y = 0;
	button_frame.setze_groesse( koord( DIALOG_WIDTH-1, y ) );
	add_komponente(&scrolly);

	y += 10+30;
	divider1.setze_pos(koord(10,y));
	divider1.setze_groesse(koord(DIALOG_WIDTH-20,0));
	add_komponente(&divider1);

	y += 10;
	savebutton.setze_pos(koord(10,y));
	savebutton.setze_groesse(koord(BUTTON_WIDTH, 14));
	savebutton.setze_text("Ok");
	savebutton.setze_typ(button_t::roundbox);
	savebutton.add_listener(this);
	add_komponente(&savebutton);

	cancelbutton.setze_pos(koord(DIALOG_WIDTH-BUTTON_WIDTH-10,y));
	cancelbutton.setze_groesse(koord(BUTTON_WIDTH, 14));
	cancelbutton.setze_text("Cancel");
	cancelbutton.setze_typ(button_t::roundbox);
	cancelbutton.add_listener(this);
	add_komponente(&cancelbutton);

	setze_fenstergroesse(koord(DIALOG_WIDTH, y + 40));
}




void savegame_frame_t::fill_list()
{
	char searchpath[1024];
	bool not_cutting_extension = (suffix==NULL  ||  suffix[0]!='.');

	if(fullpath==NULL) {
#ifndef _MSC_VER
		sprintf( searchpath, "%s/.", SAVE_PATH );
#else
		sprintf( searchpath, "%s/*%s", SAVE_PATH, suffix==NULL ? "" : suffix );
#endif
#ifndef	WIN32
		mkdir(SAVE_PATH, 0700);
#else
		mkdir(SAVE_PATH);
#endif
		fullpath = SAVE_PATH_X;
	}
	else {
		// we provide everything
#ifndef _MSC_VER
		sprintf( searchpath, "%s.", fullpath );
#else
		sprintf( searchpath, "%s*", fullpath );
#endif
		fullpath = NULL;
	}

#ifndef _MSC_VER
	// find filenames
	DIR     *dir;                      /* Schnittstellen zum BS */

	dir=opendir( searchpath );
	if(dir==NULL) {
		dbg->warning("savegame_frame_t::savegame_frame_t()","Couldn't read directory.");
	}
	else {
		const dirent* entry;
		do {
			entry=readdir(dir);
			if(entry!=NULL) {
				if(entry->d_name[0]!='.' ||  (entry->d_name[1]!='.' && entry->d_name[1]!=0)) {
					if(check_file(entry->d_name,suffix)) {
						add_file(entry->d_name, get_info(entry->d_name), not_cutting_extension);
					}
				}
			}
		}while(entry!=NULL);
		closedir(dir);
	}
#else
	{
		struct _finddata_t entry;
		long hfind;

		hfind = _findfirst( searchpath, &entry);
		if(hfind == -1) {
			dbg->warning("savegame_frame_t::savegame_frame_t()","Couldn't read directory.");
		}
		else {
			do {
				if(check_file(entry.name,suffix)) {
					add_file(entry.name, get_info(entry.name), not_cutting_extension);
				}
			} while(_findnext(hfind, &entry) == 0 );
		}
	}
#endif

	// The file entries
	int y = 0;
	for (slist_tpl<entry>::const_iterator i = entries.begin(), end = entries.end(); i != end; ++i) {
		button_t*    button1 = i->del;
		button_t*    button2 = i->button;
		gui_label_t* label   = i->label;

		button1->setze_groesse(koord(14, 14));
		button1->setze_text("X");
		button1->setze_pos(koord(5, y));
		button1->set_tooltip("Delete this file.");

		button2->setze_pos(koord(25, y));
		button2->setze_groesse(koord(140, 14));

		label->setze_pos(koord(170, y+3));

		button1->add_listener(this);
		button2->add_listener(this);

		button_frame.add_komponente(button1);
		button_frame.add_komponente(button2);
		button_frame.add_komponente(label);

		y += 14;
	}
	button_frame.setze_groesse( koord( DIALOG_WIDTH-1, y ) );
	setze_fenstergroesse(koord(DIALOG_WIDTH, y + 90));
}



savegame_frame_t::~savegame_frame_t()
{
	for (slist_tpl<entry>::const_iterator i = entries.begin(), end = entries.end(); i != end; ++i) {
		delete [] const_cast<char*>(i->button->gib_text());
		delete i->button;
		delete [] const_cast<char*>(i->label->get_text_pointer());
		delete i->label;
		delete i->del;
	}
}



// sets the current filename in the input box
void
savegame_frame_t::set_filename(const char *fn)
{
	long len=strlen(fn);
	if(len>=4  &&  len-SAVE_PATH_X_LEN-3<58) {
		if(strncmp(fn,SAVE_PATH_X,SAVE_PATH_X_LEN)==0) {
			tstrncpy(input.gib_text(), fn+SAVE_PATH_X_LEN, len-SAVE_PATH_X_LEN-3 );
		}
		else {
			tstrncpy(input.gib_text(), fn, len-3 );
		}
	}
}



void savegame_frame_t::add_file(const char *filename, const char *pak, const bool no_cutting_suffix )
{
	button_t * button = new button_t();
	char * name = new char [strlen(filename)+10];
	char * date = new char[strlen(pak)+1];

	sprintf(date, "%s", pak);
	sprintf(name, "%s", filename);
	if(!no_cutting_suffix) {
		name[strlen(name)-4] = '\0';
	}
	button->set_no_translate(true);
	button->setze_text(name);	// to avoid translation

	// sort by date descending:
	slist_tpl<entry>::iterator i = entries.begin();
	for (slist_tpl<entry>::iterator end = entries.end(); i != end; ++i) {
		if (strcmp(i->label->get_text_pointer(), date) < 0) {
			break;
		}
	}

	gui_label_t* l = new gui_label_t(NULL);
	l->set_text_pointer(date);
	entries.insert(i, entry(button, new button_t, l));
}


// true, if this is a croorect file
bool savegame_frame_t::check_file( const char *filename, const char *suffix )
{
	// assume truth, if there is no pattern to compare
	return suffix==NULL  ||  (strncmp( filename + strlen(filename) - 4, suffix, 4) == 0);
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool savegame_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	char buf[1024];

	if(komp == &input || komp == &savebutton) {
		// Save/Load Button or Enter-Key pressed
		//---------------------------------------

		tstrncpy(buf, SAVE_PATH_X, lengthof(buf));
		strcat(buf, ibuf);
		strcat(buf, suffix);

		destroy_win(this);      //29-Oct-2001         Markus Weber    Close window

		action(buf);
	}
	else if(komp == &cancelbutton) {
		// Cancel-button pressed
		//-----------------------------
		destroy_win(this);      //29-Oct-2001         Markus Weber    Added   savebutton case

	}
	else {
		// File in list selected
		//--------------------------
		for (slist_tpl<entry>::const_iterator i = entries.begin(), end = entries.end(); i != end; ++i) {
			if (komp == i->button || komp == i->del) {
				buf[0] = 0;
				if(fullpath) {
					tstrncpy(buf, fullpath, lengthof(buf));
				}
				strcat(buf, i->button->gib_text());
				if(fullpath) {
					strcat(buf, suffix);
				}

				if (komp == i->button) {
					action(buf);
				} else {
					del_action(buf);
				}
				destroy_win(this);
				break;
			}
		}
	}
	return true;
}




/**
 * Bei Scrollpanes _muss_ diese Methode zum setzen der Groesse
 * benutzt werden.
 * @author Hj. Malthaner
 */
void savegame_frame_t::setze_fenstergroesse(koord groesse)
{
	if(groesse.y>display_get_height()-64) {
		// too large ...
		groesse.y = display_get_height()-64;
		// position adjustment will be done automatically ... nice!
	}
	scrolly.setze_groesse( koord(groesse.x,groesse.y-30-40-8) );
	gui_frame_t::setze_fenstergroesse(groesse);
	divider1.setze_pos(koord(10,groesse.y-44));
	savebutton.setze_pos(koord(10,groesse.y-34));
	cancelbutton.setze_pos(koord(DIALOG_WIDTH-BUTTON_WIDTH-10,groesse.y-34));
}




void savegame_frame_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN && ev->ev_code == WIN_OPEN  &&  entries.empty()) {
		// before no virtual functions can be used ...
		fill_list();
	}
	gui_frame_t::infowin_event(ev);
}
