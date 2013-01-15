/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string>
#include <string.h>
#include <stdio.h>

#include "../pathes.h"

#include "../simdebug.h"
#include "../simsys.h"
#include "../simwin.h"

#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"
#include "../utils/searchfolder.h"

#include "savegame_frame.h"

#define DIALOG_WIDTH (488)


savegame_frame_t::savegame_frame_t(const char *suffix, bool only_directories, const char *path, const bool delete_enabled) : gui_frame_t( translator::translate("Load/Save") ),
	in_action(false),
	searchpath_defined(false),
	suffix(suffix),
	only_directories(only_directories),
	delete_enabled(delete_enabled),
	input(),
	fnlabel("Filename"),
	scrolly(&button_frame),
	num_sections(0)
{

	fnlabel.set_pos(koord(10, 4));
	add_komponente(&fnlabel);

	// Input box for game name
	tstrncpy(ibuf, "", lengthof(ibuf));
	input.set_text(ibuf, 128);
	input.set_pos(koord(75, 2));
	input.set_groesse(koord(DIALOG_WIDTH-75-scrollbar_t::BAR_SIZE-1, D_BUTTON_HEIGHT));
	add_komponente(&input);

	// needs to be scrollable
	scrolly.set_pos(koord(0, 20));
	scrolly.set_scroll_amount_y(D_BUTTON_HEIGHT);
	scrolly.set_size_corner(false);
	add_komponente(&scrolly);

	add_komponente(&divider1);

	savebutton.set_groesse(koord(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	savebutton.set_text("Ok");
	savebutton.set_typ(button_t::roundbox);
	savebutton.add_listener(this);
	add_komponente(&savebutton);

	cancelbutton.set_groesse(koord(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	cancelbutton.set_text("Cancel");
	cancelbutton.set_typ(button_t::roundbox);
	cancelbutton.add_listener(this);
	add_komponente(&cancelbutton);

	set_focus(&input);

	set_min_windowsize(koord(2*(D_BUTTON_WIDTH+scrollbar_t::BAR_SIZE)+D_H_SPACE, get_fenstergroesse().y+1));
	set_fenstergroesse(koord(DIALOG_WIDTH, D_TITLEBAR_HEIGHT+20+3*D_BUTTON_HEIGHT+30+1));

	set_resizemode(diagonal_resize);
	resize(koord(0, 0));

	if(this->suffix == NULL) {
		this->suffix = "";
	}

	if(path != NULL) {
		this->add_path(path);
		// needed?
		dr_mkdir(path);
	}
}


savegame_frame_t::~savegame_frame_t()
{
	FOR(slist_tpl<dir_entry_t>, const& i, entries) {
		if(i.button) {
			delete [] const_cast<char*>(i.button->get_text());
			delete i.button;
		}
		if(i.label) {
			char *tooltip = const_cast<char*>(i.label->get_tooltip_pointer());
			if (tooltip) {
				delete [] tooltip;
			}
			delete [] const_cast<char*>(i.label->get_text_pointer());
			delete i.label;
		}
		if(i.del) {
			delete i.del;
		}
		if(i.info) {
			delete [] i.info;
		}
	}

	this->paths.clear();
}


// sets the current filename in the input box
void savegame_frame_t::set_filename(const char *fn)
{
	size_t len = strlen(fn);
	if(len>=4  &&  len-SAVE_PATH_X_LEN-3<128) {
		if(strstart(fn, SAVE_PATH_X)) {
			tstrncpy(ibuf, fn+SAVE_PATH_X_LEN, len-SAVE_PATH_X_LEN-3 );
		}
		else {
			tstrncpy(ibuf, fn, len-3);
		}
		input.set_text(ibuf, 128);
	}
}


void savegame_frame_t::list_filled()
{
	// The file entries
	int y = 0;

	FOR(slist_tpl<dir_entry_t>, const& i, entries) {
		button_t*    const button1 = i.del;
		button_t*    const button2 = i.button;
		gui_label_t* const label   = i.label;

		if(i.type == LI_HEADER) {
			label->set_pos(koord(10, y+4));
			button_frame.add_komponente(label);
			if(this->num_sections < 2) {
				// If just 1 section added, we won't print the header, skipping the y increment
				label->set_visible(false);
				continue;
			}
		}
		else{
			button1->set_groesse(koord(14, D_BUTTON_HEIGHT));
			button1->set_text("X");
			button1->set_pos(koord(5, y));
#ifdef SIM_SYSTEM_TRASHBINAVAILABLE
			button1->set_tooltip("Send this file to the system trash bin. SHIFT+CLICK to permanently delete.");
#else
			button1->set_tooltip("Delete this file.");
#endif

			button2->set_pos(koord(25, y));
			button2->set_groesse(koord(140, D_BUTTON_HEIGHT));

			label->set_pos(koord(170, y+2));

			button1->add_listener(this);
			button2->add_listener(this);

			button_frame.add_komponente(button1);
			button_frame.add_komponente(button2);
			button_frame.add_komponente(label);
		}

		y += D_BUTTON_HEIGHT;
	}
	// since width was maybe increased, we only set the heigth.
	button_frame.set_groesse(koord(get_fenstergroesse().x-1, y));
	set_fenstergroesse(koord(get_fenstergroesse().x, D_TITLEBAR_HEIGHT+12+y+30+1));
}


void savegame_frame_t::add_file(const char *fullpath, const char *filename, const char *pak, const bool no_cutting_suffix)
{
	button_t *button = new button_t();
	char *name = new char[strlen(filename)+10];
	char *date = new char[strlen(pak)+1];

	strcpy(date, pak);
	strcpy(name, filename);

	if(!no_cutting_suffix) {
		name[strlen(name)-4] = '\0';
	}
	button->set_no_translate(true);
	button->set_text(name);	// to avoid translation

	std::string const compare_to = !umgebung_t::objfilename.empty() ? umgebung_t::objfilename.substr(0, umgebung_t::objfilename.size() - 1) + " -" : std::string();
	// sort by date descending:
	slist_tpl<dir_entry_t>::iterator i = entries.begin();
	slist_tpl<dir_entry_t>::iterator end = entries.end();

	// This needs optimizing, advance to the last section, since inserts come allways to the last section, we could just update  last one on last_section

	slist_tpl<dir_entry_t>::iterator lastfound;
	while(i != end) {
		if(i->type == LI_HEADER) {
			lastfound = i;
		}
		i++;
	}
	i = ++lastfound;

	// END of optimizing

	if(!strstart(pak, compare_to.c_str())) {
		// skip current ones
		while(i != end) {
			// extract palname in same format than in savegames ...
			if(!strstart(i->label->get_text_pointer(), compare_to.c_str())) {
				break;
			}
			++i;
		}
		// now just sort according time
		while(i != end) {
			if(strcmp(i->label->get_text_pointer(), date) < 0) {
				break;
			}
			++i;
		}
	}
	else {
		// Insert to our games (or in front if none)
		while(i != end) {
			if(i->type == LI_HEADER) {
				++i;
				continue;
			}

			if(strcmp(i->label->get_text_pointer(), date) < 0) {
				break;
			}
			// not our savegame any more => insert
			if(!strstart(i->label->get_text_pointer(), compare_to.c_str())) {
				break;
			}
			++i;
		}
	}

	gui_label_t* l = new gui_label_t(NULL);
	l->set_text_pointer(date);
	button_t *del = new button_t();
	entries.insert(i, dir_entry_t(button, del, l, LI_ENTRY, fullpath));
}


// true, if this is a correct file
bool savegame_frame_t::check_file(const char *filename, const char *suffix)
{
	// assume truth, if there is no pattern to compare
	return suffix==NULL  ||  (strncmp(filename+strlen(filename)-4, suffix, 4) == 0);
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool savegame_frame_t::action_triggered(gui_action_creator_t *komp, value_t /* */)
{
	char buf[1024];

	if(komp==&input  ||  komp==&savebutton) {
		// Save/Load Button or Enter-Key pressed
		//---------------------------------------
		if(strstart(ibuf, "net:")) {
			tstrncpy(buf, ibuf, lengthof(buf));
		}
		else {
			if(searchpath_defined) {
				tstrncpy(buf, searchpath, lengthof(buf));
			}
			else {
				buf[0] = 0;
			}
			strcat(buf, ibuf);
			if(suffix) {
				strcat(buf, suffix);
			}
		}
		set_focus(NULL);
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
		FOR(slist_tpl<dir_entry_t>, const& i, entries) {
			if(in_action){
				break;
			}
			if(komp==i.button  ||  komp==i.del) {
				in_action = true;
				bool const action_btn = komp == i.button;

				if(action_btn) {
					set_focus(NULL);
					action(i.info);
					destroy_win(this);
				}
				else {
					if(del_action(i.info)) {
						set_focus(NULL);
						destroy_win(this);
					}
					else {
						set_focus(NULL);
						// do not delete components
						// simply hide them
						i.button->set_visible(false);
						i.del->set_visible(false);
						i.label->set_visible(false);
						i.button->set_groesse(koord(0, 0));
						i.del->set_groesse(koord(0, 0));

						resize(koord(0, 0));
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
	if(groesse.y > display_get_height()-70) {
		// too large ...
		groesse.y = ((display_get_height()-D_TITLEBAR_HEIGHT-12-30-1)/D_BUTTON_HEIGHT)*D_BUTTON_HEIGHT+D_TITLEBAR_HEIGHT+12+30+1-70;
		// position adjustment will be done automatically ... nice!
	}
	gui_frame_t::set_fenstergroesse(groesse);
	groesse = get_fenstergroesse();

	input.set_groesse(koord(groesse.x-75-scrollbar_t::BAR_SIZE-1, D_BUTTON_HEIGHT));

	sint16 y = 0;
	sint16 curr_section = 0;

	FOR(slist_tpl<dir_entry_t>, const& i, entries) {
		// resize all but delete button

		// header entry, it's only shown if we have more than one
		if(i.type == LI_HEADER && this->num_sections>1) {
			i.label->set_pos(koord(10, y+2));
			y += D_BUTTON_HEIGHT;
			curr_section++;
			continue;
		}
		if (i.type == LI_HEADER && this->num_sections<2){
			// skip this header, we don't increment y
			curr_section++;
			continue;
		}

		if(i.button->is_visible()) {
			button_t* const button1 = i.del;
			button1->set_pos(koord(button1->get_pos().x, y));
			// We disable the delete button for extra sections entries
			if(curr_section > 1  ||  !this->delete_enabled) {
				button1->set_visible(false);
			}
			button_t* const button2 = i.button;
			button2->set_pos(koord(button2->get_pos().x, y));
			button2->set_groesse(koord(groesse.x/2-40, D_BUTTON_HEIGHT));
			i.label->set_pos(koord(groesse.x/2-40+30, y+2));
			y += D_BUTTON_HEIGHT;
		}
	}

	button_frame.set_groesse(koord(groesse.x, y));
	scrolly.set_groesse(koord(groesse.x, groesse.y-D_TITLEBAR_HEIGHT-12-30-1));

	divider1.set_pos(koord(4, groesse.y-36));
	divider1.set_groesse(koord(groesse.x-8-1, 0));

	savebutton.set_pos(koord(scrollbar_t::BAR_SIZE, groesse.y-D_BUTTON_HEIGHT-2-16-1));
	cancelbutton.set_pos(koord(groesse.x-D_BUTTON_WIDTH-scrollbar_t::BAR_SIZE, groesse.y-D_BUTTON_HEIGHT-2-16-1));
}


bool savegame_frame_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_OPEN  &&  entries.empty()) {
		// before no virtual functions can be used ...
		fill_list();
		set_focus(&input);
	}
	if(  ev->ev_class == EVENT_KEYBOARD  &&  ev->ev_code == 13  ) {
		action_triggered(&input, (long)0);
		return true;	// swallowed
	}
	return gui_frame_t::infowin_event(ev);
}

void savegame_frame_t::cleanup_path(char *path)
{
#ifdef _WIN32
	char *p = path;

	while (*(p++) != '\0'){
		if(*p == '/') {
			*p='\\';
		}
	}

	if ( strlen(path)>2  && path[1]==':' ) {
		path[0] = (char) toupper(path[0]);
	}

#else
	(void)path;
#endif
}

void savegame_frame_t::fill_list()
{
	const char *suffixnodot;
	searchfolder_t sf;
	char *fullname;
	bool not_cutting_extension = (suffix==NULL  ||  suffix[0]!='.');

	if(suffix == NULL){
		suffixnodot = NULL;
	}
	else{
		suffixnodot = (suffix[0] == '.')?suffix+1:suffix;
	}

	// for each path, we search.
	FOR(vector_tpl<std::string>, &path, paths){

		const char		*path_c = path.c_str();
		const size_t	path_c_len = strlen(path_c);

		sf.search(path, std::string(suffixnodot), this->only_directories, false);

		bool section_added = false;

		// Add the entries that pass the check
		FOR(searchfolder_t, const &name, sf) {
			fullname = new char [path_c_len+strlen(name)+1];
			sprintf(fullname,"%s%s",path_c,name);

			if(check_file(fullname, suffix)){
				if (!section_added) {
					add_section(path);
					section_added = true;
				}
				add_file(fullname, name, get_info(fullname), not_cutting_extension);
			}
			else{
				// NOTE: we just free "fullname" memory when add_file is not called. That memory will be
				// free'd in the class destructor. This way we save the cost of re-allocate/copy it inside there
				delete fullname;
			}
		}

	}

	// Notify of the end
	list_filled();
}

void savegame_frame_t::shorten_path(char *dest,const char *orig,const size_t max_size)
{
	assert (max_size > 2);

	const size_t orig_size = strlen(orig);

	if ( orig_size < max_size ) {
		strcpy(dest,orig);
		return;
	}

	const int half = max_size/2;
	const int odd = max_size%2;

	strncpy(dest,orig,half-1);
	strncpy(&dest[half-1],"...",3);
	strcpy(&dest[half+2],&orig[orig_size-half+2-odd]);

}

#define SHORTENED_SIZE 48

void savegame_frame_t::add_section(std::string &name){

	const char *prefix_label = translator::translate("Files from:");
	size_t prefix_len = strlen(prefix_label);

	// NOTE: These char buffers will be freed on the destructor
	// +2 because of the space in printf and the ending \0
	char *label_text = new char [SHORTENED_SIZE+prefix_len+2];
	char *path_expanded = new char[FILENAME_MAX];

	size_t program_dir_len = strlen(umgebung_t::program_dir);

	if (strncmp(name.c_str(),umgebung_t::program_dir,program_dir_len) == 0) {
		// starts with program_dir
		strncpy(path_expanded, name.c_str(), FILENAME_MAX);
	}
	else {
		// user_dir path
		size_t name_len = strlen(name.c_str());
		size_t user_dir_len = strlen(umgebung_t::user_dir);

		if ( name_len+user_dir_len > FILENAME_MAX-1 ) {
			// shoudn't happen, but I'll control anyway
			strcpy(path_expanded,"** ERROR ** Path too long");
		}
		else {
			sprintf(path_expanded,"%s%s", umgebung_t::user_dir, name.c_str());
		}
	}

	cleanup_path(path_expanded);

	char shortened_path[SHORTENED_SIZE+1];

	shorten_path(shortened_path,path_expanded,SHORTENED_SIZE);

	sprintf(label_text,"%s %s", prefix_label , shortened_path);

	gui_label_t* l = new gui_label_t(NULL, COL_WHITE);
	l->set_text_pointer(label_text);
	l->set_tooltip(path_expanded);

	this->entries.append(dir_entry_t(NULL, NULL, l, LI_HEADER, NULL));
	this->num_sections++;
}

void savegame_frame_t::add_path(const char * path){

	if (!this->searchpath_defined) {
		sprintf(this->searchpath, "%s", path);
		this->searchpath_defined = true;
	}
	this->paths.append(path);
}
/**
 * @note On Windows Plattform, we use the trash bin.
 */
bool savegame_frame_t::del_action(const char * fullpath)
{
#ifdef SIM_SYSTEM_TRASHBINAVAILABLE

	if (event_get_last_control_shift()&1) {
		// shift pressed, delete without trash bin
		remove(fullpath);
		return false;
	}

	dr_movetotrash(fullpath);
	return false;

#else
	remove(fullpath);
#endif
	return false;
}


std::string savegame_frame_t::get_basename(const char *fullpath)
{
	std::string path = fullpath;
	size_t last = path.find_last_of("\\/");
	if (last==std::string::npos){
		return path;
	}
	return path.substr(0,last+1);
}


std::string savegame_frame_t::get_filename(const char *fullpath,const bool with_extension)
{
	std::string path = fullpath;

	// Remove until last \ or /

	size_t last = path.find_last_of("\\/");
	if (last!=std::string::npos) {
		path = path.erase(0,last+1);
	}

	// Remove extension if it's present, will remove from '.' till the end.

	if (!with_extension){
		last = path.find_last_of(".");
		if (last!=std::string::npos) {
			path = path.erase(last);
		}
	}
	return path;
}
