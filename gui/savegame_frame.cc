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
	gui_frame_t("Load/Save"),
	input(),
	fnlabel("Filename"),
	scrolly(&button_frame)
{
	use_table = false;
	init(suffix, path);
}
savegame_frame_t::savegame_frame_t(const char *suffix, const char *path, bool use_table ) :
	gui_frame_t("Load/Save") ,
	fnlabel("Filename"),
	scrolly(use_table ? (gui_komponente_t*)&file_table : (gui_komponente_t*)&button_frame)
{
	this->use_table = use_table;
	init(suffix, path);
}

void savegame_frame_t::init(const char *suffix, const char *path)
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
	input.set_pos(koord(75,8));
	//input.set_groesse(koord(DIALOG_WIDTH-75-10, 14));
	add_komponente(&input);

	// needs to be scrollable
	scrolly.set_pos( koord(10,30) );
	scrolly.set_show_scroll_x(false);
	scrolly.set_size_corner(false);
	//scrolly.set_groesse( koord(DIALOG_WIDTH-20,30) );

	// The file entries
	int y = 0;
	release_file_table_button();
	file_table.set_groesse( koord( DIALOG_WIDTH-1, y ) );
	file_table.set_grid_width(koord(0,0));
	file_table.add_listener(this);
	button_frame.set_groesse( koord( DIALOG_WIDTH-1, y ) );

	add_komponente(&scrolly);

	y += 10+30;
	//divider1.set_pos(koord(10,y));
	//divider1.set_groesse(koord(DIALOG_WIDTH-20,0));
	add_komponente(&divider1);

	y += 10;
	//savebutton.set_pos(koord(10,y));
	savebutton.set_groesse(koord(BUTTON_WIDTH, 14));
	savebutton.set_text("Ok");
	savebutton.set_typ(button_t::roundbox);
	savebutton.add_listener(this);
	add_komponente(&savebutton);

	//cancelbutton.set_pos(koord(DIALOG_WIDTH-BUTTON_WIDTH-10,y));
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
						add_file(entry->d_name, not_cutting_extension);
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
					add_file(entry.name, not_cutting_extension);
				}
			} while(_findnext(hfind, &entry) == 0 );
		}
	}
#endif

	// The file entries
	if (use_table)
	{
		set_file_table_default_sort_order();
		file_table.sort_rows();
		file_table.set_groesse(file_table.get_table_size());
		set_fenstergroesse(file_table.get_groesse() + koord(25 + 14, 90));
	}
	else
	{
		int y = 0;
		for (slist_tpl<entry>::const_iterator i = entries.begin(), end = entries.end(); i != end; ++i) {
			button_t*    button1 = i->del;
			button_t*    button2 = i->button;
			gui_label_t* label   = i->label;

			button1->set_groesse(koord(14, 14));
			button1->set_text("X");
			button1->set_pos(koord(0, y));
			button1->set_tooltip("Delete this file.");

			button2->set_pos(koord(20, y));
			button2->set_groesse(koord(150, 14));

			label->set_pos(koord(176, y+3));

			button1->add_listener(this);
			button2->add_listener(this);

			button_frame.add_komponente(button1);
			button_frame.add_komponente(button2);
			button_frame.add_komponente(label);

			y += 14;
		}
		// since width was maybe increased, we only set the heigth.
		button_frame.set_groesse( koord( get_fenstergroesse().x-14, y ) );
		set_fenstergroesse(koord(get_fenstergroesse().x, y + 90));
	}
}


void savegame_frame_t::set_file_table_default_sort_order()
{
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

void savegame_frame_t::add_file(const char *filename, const bool not_cutting_suffix ) 
{
	add_file(filename, get_info(filename), not_cutting_suffix);
}


void savegame_frame_t::add_file(const char *filename, const char *pak, const bool no_cutting_suffix )
{
	if (use_table) {
		char pathname[1024];
		const char *path = get_path();
		sprintf( pathname, "%s%s", path ? path : "", filename );
		char buttontext[1024];
		strcpy( buttontext, filename );
		if ( !no_cutting_suffix ) {
			buttontext[strlen(buttontext)-4] = '\0';
		}
		file_table.add_row( new gui_file_table_row_t( pathname, buttontext ));
	}
	else {
		char * name = new char[strlen(filename)+10];
		char * date = new char[strlen(pak)+1];

		strcpy( date, pak );
		strcpy( name, filename );
		if(!no_cutting_suffix) {
			name[strlen(name)-4] = '\0';
		}
		button_t * button = new button_t();
		button->set_no_translate(true);
		button->set_text(name);	// to avoid translation

		const string compare_to = umgebung_t::objfilename.size()>0  ?  umgebung_t::objfilename.substr(0, umgebung_t::objfilename.size()-1 ) + " -"  :  string("");
		// sort by date descending:
		slist_tpl<entry>::iterator i = entries.begin();
		slist_tpl<entry>::iterator end = entries.end();
		if(  strncmp( compare_to.c_str(), pak, compare_to.size() )!=0  ) {
			// skip current ones
			while(  i != end  ) {
				// extract palname in same format than in savegames ...
				if(  strncmp( compare_to.c_str(), i->label->get_text_pointer(), compare_to.size() ) !=0  ) {
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
				if(  strncmp( compare_to.c_str(), i->label->get_text_pointer(), compare_to.size() ) !=0  ) {
					break;
				}
				++i;
			}
		}
		gui_label_t* l = new gui_label_t(NULL);
		l->set_text_pointer(date);
		entries.insert(i, entry(button, new button_t, l));
	}
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
bool savegame_frame_t::action_triggered( gui_action_creator_t *komp, value_t p)
{
	char buf[1024];

	if(komp == &input || komp == &savebutton) {
		// Save/Load Button or Enter-Key pressed
		//---------------------------------------

		if (strstr(ibuf,"net:")==ibuf) {
			tstrncpy(buf,ibuf,lengthof(buf));
		}
		else {
			tstrncpy(buf, SAVE_PATH_X, lengthof(buf));
			strcat(buf, ibuf);
			strcat(buf, suffix);
		}
		set_focus( NULL );
		action(buf);
		destroy_win(this);      //29-Oct-2001         Markus Weber    Close window

	}
	else if(komp == &cancelbutton) {
		// Cancel-button pressed
		//-----------------------------
		destroy_win(this);      //29-Oct-2001         Markus Weber    Added   savebutton case
	}
	else if (komp == &file_table) {
		gui_table_event_t *event = (gui_table_event_t *) p.p;
		if (event->is_cell_hit) {
			const event_t *ev = event->get_event();
			if (file_table_button_pressed && event->cell != pressed_file_table_button) {
				release_file_table_button();
			}
			switch (ev->ev_code) {
				case MOUSE_LEFTBUTTON: {
					coordinate_t x = event->cell.get_x();
					if (x < 2) {
						const bool action_btn = x == 1;
						coordinate_t y = event->cell.get_y();
						gui_file_table_row_t *row = (gui_file_table_row_t*) file_table.get_row(y);
						switch (ev->ev_class) {
							case EVENT_CLICK:
								press_file_table_button(event->cell);
								break;

							case EVENT_RELEASE:
								if (row->get_pressed())
								{
									if(action_btn) {
										action(row->get_name());
										destroy_win(this);
									}
									else {
										if( del_action(row->get_name()) ) {
											destroy_win(this);
										}
										else {
											file_table.remove_row(y);
										}
									}
								}
								break;
						}
					}
					else {
						release_file_table_button();
						//qsort();
					}
					break;
				}			
			}
		}
		else if (file_table_button_pressed) {
			release_file_table_button();
		}
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
					set_focus( NULL );
					action(buf);
					destroy_win(this);
				}
				else {
					if(  del_action(buf)  ) {
						set_focus( NULL );
						destroy_win(this);
					}
					else {
						set_focus(NULL);
						// do not delete components
						// simply hide them
						i->button->set_visible(false);
						i->del->set_visible(false);
						i->label->set_visible(false);
						i->button->set_groesse( koord( 0, 0 ) );
						i->del->set_groesse( koord( 0, 0 ) );

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
	sint16 width = groesse.x - 10;
	if(groesse.y>display_get_height()-64) {
		// too large ...
		groesse.y = display_get_height()-64;
		// position adjustment will be done automatically ... nice!
	}
	gui_frame_t::set_fenstergroesse(groesse);
	input.set_groesse(koord(width-input.get_pos().x, 14));

	sint16 y = 0;
	if (use_table)
	{
		y = file_table.get_table_height();
		scrolly.set_groesse( koord(width,groesse.y-40-8) - scrolly.get_pos() );
		scrolly.set_show_scroll_y(y > scrolly.get_groesse().y);
		sint32 c = file_table.get_size().get_x();
		if (c > 0) {
			c = c > 0 ? 1 : 0;
			sint16 x = scrolly.get_client_size().x - (file_table.get_table_width() - file_table.get_column_width(c));
			file_table.get_column(c)->set_width(x);
		}
	}
	else
	{
		for (slist_tpl<entry>::const_iterator i = entries.begin(), end = entries.end(); i != end; ++i) {
			// resize all but delete button
			if(  i->button->is_visible()  ) {
				button_t*    button1 = i->del;
				button1->set_pos( koord( button1->get_pos().x, y ) );
				button_t*    button2 = i->button;
				gui_label_t* label   = i->label;
				button2->set_pos( koord( button2->get_pos().x, y ) );
				button2->set_groesse(koord( groesse.x/2-40, 14));
				label->set_pos(koord(groesse.x/2-40+30, y));
				y += 14;
			}
		}
		button_frame.set_groesse( koord( groesse.x, y ) );
		scrolly.set_groesse( koord(width,groesse.y-40-8) - scrolly.get_pos() );
		scrolly.set_show_scroll_y(y > scrolly.get_groesse().y);
	}

	divider1.set_pos(koord(10,groesse.y-44));
	divider1.set_groesse(koord(width-10,0));
	savebutton.set_pos(koord(10,groesse.y-39));
	cancelbutton.set_pos(koord(width-BUTTON_WIDTH,groesse.y-39));
}




bool savegame_frame_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_OPEN  &&  entries.empty()) {
		// before no virtual functions can be used ...
		fill_list();
		set_focus( &input );
	}
	if(  ev->ev_class == EVENT_KEYBOARD  &&  ev->ev_code == 13  ) {
		action_triggered( &input, (long)0 );
		return true;	// swallowed
	}
	return gui_frame_t::infowin_event(ev);
}

void savegame_frame_t::press_file_table_button(coordinates_t &cell)
{
	pressed_file_table_button = cell;
	file_table_button_pressed = true;
	for (coordinate_t i = file_table.get_size().get_x(); i > 0; ) {
		--i;
		((gui_file_table_column_t*)file_table.get_column(i))->set_pressed(i == cell.get_x());
	}
	for (coordinate_t i = file_table.get_size().get_y(); i > 0; ) {
		--i;
		((gui_file_table_row_t*)file_table.get_row(i))->set_pressed(i == cell.get_y());
	}
}

void savegame_frame_t::release_file_table_button()
{
	file_table_button_pressed = false;
	for (coordinate_t i = file_table.get_size().get_x(); i > 0; ) {
		--i;
		((gui_file_table_column_t*)file_table.get_column(i))->set_pressed(false);
	}
	for (coordinate_t i = file_table.get_size().get_y(); i > 0; ) {
		--i;
		((gui_file_table_row_t*)file_table.get_row(i))->set_pressed(false);
	}
}


// BG, 26.03.2010
void gui_file_table_button_column_t::paint_cell(const koord &offset, coordinate_t /*x*/, coordinate_t /*y*/, const gui_table_row_t &row) 
{
 	gui_file_table_row_t &file_row = (gui_file_table_row_t&)row;
	koord size = koord(get_width(), row.get_height());
	btn.set_groesse(size);
	koord mouse(get_maus_x() - offset.x, get_maus_y() - offset.y);
	if (0 <= mouse.x && mouse.x < size.x && 0 <= mouse.y && mouse.y < size.y){ 
		btn.set_typ(button_t::roundbox);
	}
	else
	{
		btn.set_typ(button_t::box);
	}
	btn.pressed = pressed && file_row.pressed;
	btn.zeichnen(offset);
}


// BG, 06.04.2010
void gui_file_table_delete_column_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row)
{
 	gui_file_table_row_t &file_row = (gui_file_table_row_t&)row;
	if (file_row.delete_enabled) {
		gui_file_table_button_column_t::paint_cell(offset, x, y, row);
	}

}


// BG, 26.03.2010
void gui_file_table_label_column_t::paint_cell(const koord &offset, coordinate_t /*x*/, coordinate_t /*y*/, const gui_table_row_t &row) 
{
	lbl.set_pos(koord(2, 2));
	lbl.set_groesse(koord(get_width() - 2, row.get_height() - 2));
	lbl.zeichnen(offset);
}


// BG, 26.03.2010
const char *gui_file_table_action_column_t::get_text(const gui_table_row_t &row) const 
{ 
 	gui_file_table_row_t &file_row = (gui_file_table_row_t&)row;
	return file_row.text.c_str();
}


// BG, 26.03.2010
void gui_file_table_action_column_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row) {
	btn.set_text(get_text(row));
	gui_file_table_button_column_t::paint_cell(offset, x, y, row);
}


// BG, 26.03.2010
time_t gui_file_table_time_column_t::get_time(const gui_table_row_t &row) const 
{ 
 	gui_file_table_row_t &file_row = (gui_file_table_row_t&)row;
	return file_row.info.st_mtime;
}


// BG, 26.03.2010
void gui_file_table_time_column_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row) {
	time_t time = get_time(row);
	struct tm *tm = localtime(&time);
	char date[64];
	if(tm) {
		strftime(date, 18, "%Y-%m-%d %H:%M", tm);
	}
	else {
		tstrncpy(date, "???\?-?\?-?? ??:??", 16); // note: ??- is the trigraph for a tilde, so one ? is escaped.
	}
	lbl.set_text(date);
	gui_file_table_label_column_t::paint_cell(offset, x, y, row);
}


// BG, 26.03.2010
gui_file_table_row_t::gui_file_table_row_t(const char *pathname, const char *buttontext, bool delete_enabled) : gui_table_row_t()
{
	this->pressed = false;
	this->delete_enabled = delete_enabled;
	this->name = pathname;
	this->text = buttontext;

	// first get pak name
	if (stat(name.c_str(), &info)) {
		this->error = "failed opening file";
	}
}


// BG, 26.03.2010
void gui_file_table_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y)
{
	gui_file_table_column_t *column_def = (gui_file_table_column_t *)get_column(x);
	gui_table_row_t *row_def = get_row(y);
	if (column_def && row_def)
	{
		column_def->paint_cell(offset, x, y, *row_def);
	}
}
