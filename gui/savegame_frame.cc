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

#include "../dataobj/umgebung.h"
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
	in_action = false;

	// both NULL is not acceptable
	assert(suffix!=path);

	fnlabel.set_pos (koord(10,12));
	add_komponente(&fnlabel);

	// Input box for game name
	tstrncpy(ibuf, "", lengthof(ibuf));
	input.set_text(ibuf, 128);
	input.add_listener(this);
	input.set_pos(koord(75,8));
	input.set_groesse(koord(DIALOG_WIDTH-75-10-10, 14));
	add_komponente(&input);

	// needs to be scrollable
	scrolly.set_pos( koord(0,30) );
	scrolly.set_show_scroll_x(false);
	scrolly.set_size_corner(false);
	scrolly.set_groesse( koord(DIALOG_WIDTH-12,30) );

	// The file entries
	int y = 0;
	button_frame.set_groesse( koord( DIALOG_WIDTH-1, y ) );
	add_komponente(&scrolly);

	y += 10+30;
	divider1.set_pos(koord(10,y));
	divider1.set_groesse(koord(DIALOG_WIDTH-20,0));
	add_komponente(&divider1);

	y += 10;
	savebutton.set_pos(koord(10,y));
	savebutton.set_groesse(koord(BUTTON_WIDTH, 14));
	savebutton.set_text("Ok");
	savebutton.set_typ(button_t::roundbox);
	savebutton.add_listener(this);
	add_komponente(&savebutton);

	cancelbutton.set_pos(koord(DIALOG_WIDTH-BUTTON_WIDTH-10,y));
	cancelbutton.set_groesse(koord(BUTTON_WIDTH, 14));
	cancelbutton.set_text("Cancel");
	cancelbutton.set_typ(button_t::roundbox);
	cancelbutton.add_listener(this);
	add_komponente(&cancelbutton);

	set_focus( &input );
	set_fenstergroesse(koord(DIALOG_WIDTH, y + 40));
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
		} while(entry!=NULL);
		closedir(dir);
	}
#else
	{
		struct _finddata_t entry;
		size_t hfind;

		hfind = _findfirst( searchpath, &entry );
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

		button1->set_groesse(koord(14, 14));
		button1->set_text("X");
		button1->set_pos(koord(5, y));
		button1->set_tooltip("Delete this file.");

		button2->set_pos(koord(25, y));
		button2->set_groesse(koord(140, 14));

		label->set_pos(koord(170, y+3));

		button1->add_listener(this);
		button2->add_listener(this);

		button_frame.add_komponente(button1);
		button_frame.add_komponente(button2);
		button_frame.add_komponente(label);

		y += 14;
	}
	// since width was maybe increased, we only set the heigth.
	button_frame.set_groesse( koord( get_fenstergroesse().x-1, y ) );
	set_fenstergroesse(koord(get_fenstergroesse().x, y + 90));
}



savegame_frame_t::~savegame_frame_t()
{
	for (slist_tpl<entry>::const_iterator i = entries.begin(), end = entries.end(); i != end; ++i) {
		delete [] const_cast<char*>(i->button->get_text());
		delete i->button;
		delete [] const_cast<char*>(i->label->get_text_pointer());
		delete i->label;
		delete i->del;
	}
}



// sets the current filename in the input box
void savegame_frame_t::set_filename(const char *fn)
{
	size_t len = strlen(fn);
	if(len>=4  &&  len-SAVE_PATH_X_LEN-3<128) {
		if(strncmp(fn,SAVE_PATH_X,SAVE_PATH_X_LEN)==0) {
			tstrncpy(ibuf, fn+SAVE_PATH_X_LEN, len-SAVE_PATH_X_LEN-3 );
		}
		else {
			tstrncpy(ibuf, fn, len-3 );
		}
		input.set_text( ibuf, 128 );
	}
}



void savegame_frame_t::add_file(const char *filename, const char *pak, const bool no_cutting_suffix )
{
	button_t * button = new button_t();
	char * name = new char [strlen(filename)+10];
	char * date = new char[strlen(pak)+1];

	strcpy( date, pak );
	strcpy( name, filename );
	if(!no_cutting_suffix) {
		name[strlen(name)-4] = '\0';
	}
	button->set_no_translate(true);
	button->set_text(name);	// to avoid translation

	const cstring_t compare_to = umgebung_t::objfilename.len()>0  ?  umgebung_t::objfilename.left( umgebung_t::objfilename.len()-1 ) + " -"  :  cstring_t("");
	// sort by date descending:
	slist_tpl<entry>::iterator i = entries.begin();
	slist_tpl<entry>::iterator end = entries.end();
	if(  strncmp( compare_to, pak, compare_to.len() )!=0  ) {
		// skip current ones
		while(  i != end  ) {
			// extract palname in same format than in savegames ...
			if(  strncmp( compare_to, i->label->get_text_pointer(), compare_to.len() ) !=0  ) {
				break;
			}
			++i;
		}
		// now just sort according time
		while(  i != end  ) {
			if(  strcmp(i->label->get_text_pointer(), date) < 0  ) {
				break;
			}
			++i;
		}
	}
	else {
		// Insert to our games (or in front if none)
		while(  i != end  ) {
			if(  strcmp(i->label->get_text_pointer(), date) < 0  ) {
				break;
			}
			// not our savegame any more => insert
			if(  strncmp( compare_to, i->label->get_text_pointer(), compare_to.len() ) !=0  ) {
				break;
			}
			++i;
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
bool savegame_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	char buf[1024];

	if(komp == &input || komp == &savebutton) {
		// Save/Load Button or Enter-Key pressed
		//---------------------------------------

		if (strstr(ibuf,"net:")==ibuf) {
			tstrncpy(buf,ibuf,lengthof(buf));
		} else {
			tstrncpy(buf, SAVE_PATH_X, lengthof(buf));
			strcat(buf, ibuf);
			strcat(buf, suffix);
		}

		action(buf);

		destroy_win(this);      //29-Oct-2001         Markus Weber    Close window

	}
	else if(komp == &cancelbutton) {
		// Cancel-button pressed
		//-----------------------------
		destroy_win(this);      //29-Oct-2001         Markus Weber    Added   savebutton case

	}
	else {
		// File in list selected
		//--------------------------
		for(  slist_tpl<entry>::iterator i = entries.begin(), end = entries.end();  i != end  &&  !in_action;  ++i  ) {
			if(  komp == i->button  ||  komp == i->del  ) {
				in_action = true;
				const bool action_btn = komp == i->button;
				buf[0] = 0;
				if(fullpath) {
					tstrncpy(buf, fullpath, lengthof(buf));
				}
				strcat(buf, i->button->get_text());
				if(fullpath) {
					strcat(buf, suffix);
				}

				if(action_btn) {
					action(buf);
					destroy_win(this);
				}
				else {
					if(  del_action(buf)  ) {
						destroy_win(this);
					}
					else {
						set_focus(NULL);
						// do not delete components
						// simply hide them
						i->button->set_visible(false);
						i->del->set_visible(false);
						i->label->set_visible(false);
						// .. and remove entry from list
						entries.erase( i );

						resize( koord(0,0) );
						in_action = false;
					}
				}
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
void savegame_frame_t::set_fenstergroesse(koord groesse)
{
	if(groesse.y>display_get_height()-64) {
		// too large ...
		groesse.y = display_get_height()-64;
		// position adjustment will be done automatically ... nice!
	}
	gui_frame_t::set_fenstergroesse(groesse);
	input.set_groesse(koord(groesse.x-75-10-10, 14));

	sint16 y = 0;
	for (slist_tpl<entry>::const_iterator i = entries.begin(), end = entries.end(); i != end; ++i) {
		// resize all but delete button
		button_t*    button1 = i->del;
		button1->set_pos( koord( button1->get_pos().x, y ) );
		button_t*    button2 = i->button;
		gui_label_t* label   = i->label;
		button2->set_pos( koord( button2->get_pos().x, y ) );
		button2->set_groesse(koord( groesse.x/2-40, 14));
		label->set_pos(koord(groesse.x/2-40+30, y));
		y += 14;
	}

	button_frame.set_groesse( koord( groesse.x, y ) );
	scrolly.set_groesse( koord(groesse.x,groesse.y-30-40-8) );

	divider1.set_pos(koord(10,groesse.y-44));
	divider1.set_groesse(koord(groesse.x-20,0));
	savebutton.set_pos(koord(10,groesse.y-34));
	cancelbutton.set_pos(koord(groesse.x-BUTTON_WIDTH-10,groesse.y-34));
}




bool savegame_frame_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN && ev->ev_code == WIN_OPEN  &&  entries.empty()) {
		// before no virtual functions can be used ...
		fill_list();
		set_focus( &input );
	}
	return gui_frame_t::infowin_event(ev);
}
