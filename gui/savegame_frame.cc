/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "savegame_frame.h"
#include "../pathes.h"
#include "../simsys.h"
#include "../simdebug.h"
#include "../gui/simwin.h"
#include "../utils/simstring.h"
#include "../utils/searchfolder.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"

#define L_BUTTON_COL_PERCENT (0.45) // Button column coverage in % of client area
#define L_MIN_ROWS           (4)    // Minimum file entries to show (calculates min window size)
#define L_DEFAULT_ROWS       (12)   // Number of file entries to show as default
#define L_DIALOG_WIDTH       (488)  // You go and figure...

#define L_SHORTENED_SIZE   (48)

// The part of the dialog that is always fixed.
#define L_FIXED_DIALOG_HEIGHT (D_TITLEBAR_HEIGHT + D_MARGINS_Y + D_EDIT_HEIGHT + D_V_SPACE + D_DIVIDER_HEIGHT + D_BUTTON_HEIGHT)


/**
 * SAVE GAME FRAME CONSTRUCTOR
 * @author Hj. Malthaner
 * @author Max Kielland
 *
 * @param suffix            Optional file pattern to populate the file list.
 *                          Example ".sve" or "sve"
 *                          Default value is NULL to disregard extension.
 * @param only_directories  Populate the file list with only directories. This is
 *                          optional with a default value of false.
 * @param path              Optional search path. If null next call to add_path()
 *                          defines the deefault path. Default value is NULL.
 * @param delete_enabled    Show (true) or hide (false) the delete buttons.
 *                          This is an optional parameter with a default value of true;
 */
savegame_frame_t::savegame_frame_t(const char *suffix, bool only_directories, const char *path, const bool delete_enabled, bool use_table) :
	gui_frame_t( translator::translate("Load/Save") ),
	suffix(suffix),
	in_action(false),
	only_directories(only_directories),
	searchpath_defined(false),
	fnlabel("Filename"),
	use_table(use_table),
	scrolly(use_table ? (gui_component_t*)&file_table : (gui_component_t*)&button_frame),
	num_sections(0),
	delete_enabled(delete_enabled)
{
	init(suffix, path);
}

void savegame_frame_t::init(const char *suffix, const char *path)
{
	scr_coord cursor = scr_coord(D_MARGIN_LEFT,D_MARGIN_TOP);

	label_enabled = true;

	fnlabel.set_pos(cursor);
	add_component(&fnlabel);

	// Input box for game name
	tstrncpy(ibuf, "", lengthof(ibuf));
	input.set_pos( scr_coord ( fnlabel.get_pos().x + fnlabel.get_size().w + D_H_SPACE, cursor.y ) );
	input.set_size( scr_size ( D_BUTTON_WIDTH, D_EDIT_HEIGHT ) );
	input.set_text(ibuf, 128);
	fnlabel.align_to(&input,ALIGN_CENTER_V);
	add_component(&input);
	cursor.y += D_EDIT_HEIGHT;
	cursor.y += D_V_SPACE;

	// Needs to be scrollable, size is adjusted in set_windowsize()
	scrolly.set_pos( cursor );
	scrolly.set_scroll_amount_y(D_BUTTON_HEIGHT + D_FOCUS_OFFSET_V);
	scrolly.set_size_corner(false);
	scrolly.set_scrollbar_mode( scrollbar_t::show_auto );

	// The file entries
	release_file_table_button();
	file_table.set_size( scr_size( L_DIALOG_WIDTH-1, 40 ) );
	file_table.set_grid_width(koord(0,0));
	file_table.add_listener(this);
	button_frame.set_size( scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT) );

	add_component(&scrolly);

	// Controls below will be sized and positioned in set_windowsize()
	add_component(&divider1);

	savebutton.init( button_t::roundbox, "Ok" );
	savebutton.add_listener( this );
	add_component( &savebutton );

	cancelbutton.init( button_t::roundbox, "Cancel" );
	cancelbutton.add_listener( this );
	add_component( &cancelbutton );

	set_focus( &input );

	set_min_windowsize( scr_size ( cursor.x + (D_BUTTON_WIDTH<<1) + D_H_SPACE + D_MARGIN_RIGHT, L_FIXED_DIALOG_HEIGHT + (L_MIN_ROWS * D_BUTTON_HEIGHT) ) );
	set_windowsize( scr_size (L_DIALOG_WIDTH, L_FIXED_DIALOG_HEIGHT + (L_DEFAULT_ROWS * D_BUTTON_HEIGHT) ) );

	set_resizemode(diagonal_resize);

	if(this->suffix == NULL) {
		this->suffix = "";
	}

	if(path != NULL) {
		this->add_path(path);
		// needed?
		dr_mkdir(path);
	}

	resize(scr_coord(0, 0));
}



/**
 * SAVE GAME FRAME DESTRUCTOR
 * Free all list items.
 * @author Hj. Malthaner
 */
savegame_frame_t::~savegame_frame_t()
{
	FOR(slist_tpl<dir_entry_t>, const& i, entries) {
		if(i.button) {
			delete [] const_cast<char*>(i.button->get_text());
			delete i.button;
		}
		if(i.label) {
			char *tooltip = const_cast<char*>(i.label->get_tooltip_pointer());
			delete [] tooltip;
			delete [] const_cast<char*>(i.label->get_text_pointer());
			delete i.label;
		}
		delete i.del;
		delete [] i.info;
	}

	this->paths.clear();
}



/**
 * ADD SECTION
 * Adds a section entry to the list...
 *
 * @param name  Section name?!?
 */
void savegame_frame_t::add_section(std::string &name){

	const char *prefix_label = translator::translate("Files from:");
	size_t prefix_len = strlen(prefix_label);

	// NOTE: These char buffers will be freed on the destructor
	// +2 because of the space in printf and the ending \0
	char *label_text = new char [L_SHORTENED_SIZE+prefix_len+2];
	char *path_expanded = new char[FILENAME_MAX];

	size_t program_dir_len = strlen(env_t::program_dir);

	if (strncmp(name.c_str(),env_t::program_dir,program_dir_len) == 0) {
		// starts with program_dir
		strncpy(path_expanded, name.c_str(), FILENAME_MAX);
	}
	else {
		// user_dir path
		size_t name_len = strlen(name.c_str());
		size_t user_dir_len = strlen(env_t::user_dir);

		if ( name_len+user_dir_len > FILENAME_MAX-1 ) {
			// shouldn't happen, but I'll control anyway
			strcpy(path_expanded,"** ERROR ** Path too long");
		}
		else {
			sprintf(path_expanded,"%s%s", env_t::user_dir, name.c_str());
		}
	}

	cleanup_path(path_expanded);

	char shortened_path[L_SHORTENED_SIZE+1];

	shorten_path(shortened_path,path_expanded,L_SHORTENED_SIZE);

	sprintf(label_text,"%s %s", prefix_label , shortened_path);

	gui_label_t* l = new gui_label_t(NULL, SYSCOL_TEXT_HIGHLIGHT);
	l->set_text_pointer(label_text);
	l->set_tooltip(path_expanded);

	this->entries.append(dir_entry_t(NULL, NULL, l, LI_HEADER, NULL));
	this->num_sections++;
}



/**
 * ADD PATH
 * Adds a path to the list of path included in the file search.
 * Several paths can be added one at a time. All added paths will
 * be searched by fill_list().
 *
 * @param path  A nul terminated path to include in the search.
 */
void savegame_frame_t::add_path(const char * path){

	if (!this->searchpath_defined) {
		sprintf(this->searchpath, "%s", path);
		this->searchpath_defined = true;
	}
	this->paths.append(path);
}



/**
 * FILL LIST
 * Populates the item list with matching file names. Each matching file
 * is first checked (check_file) and then added (add_file).
 */
void savegame_frame_t::fill_list( void )
{
	const char *suffixnodot;
	searchfolder_t sf;
	char *fullname;
	bool not_cutting_extension = (suffix==NULL  ||  suffix[0]!='.');

	if(  suffix == NULL  ){
		suffixnodot = NULL;
	}
	else {
		suffixnodot = (suffix[0] == '.')  ?suffix+1 : suffix;
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

			if(  check_file(fullname, suffix)  ){
				if(!section_added) {
					add_section(path);
					section_added = true;
				}
				add_file(fullname, name, /* get_info(fullname),*/ not_cutting_extension);
			}
			else {
				// NOTE: we just free "fullname" memory when add_file is not called. That memory will be
				// freed in the class destructor. This way we save the cost of re-allocate/copy it inside there
				delete [] fullname;
			}
		}

	}

	// Notify of the end
	list_filled();

	// force position and size calculation of list elements
	resize(scr_coord(0, 0));
}



/**
 * LIST FILLED
 * All items has been inserted in the list.
 * On return resize() is called and all item's GYU members are positioned and resized.
 * Therefore it is no use to set the button's and label's width or any items y position.
 * The only control keeping its size is the delete button.
 * @author Unknown
 * @author Max Kielland
 */
void savegame_frame_t::list_filled( void )
{
	// The file entries
	if (use_table)
	{
		set_file_table_default_sort_order();
		file_table.sort_rows();
		file_table.set_size(file_table.get_table_size());
		set_windowsize(file_table.get_size() + scr_size(25 + 14, 90));
	}
	else
	{
		// The file entries
		int y = 0;

		FOR(slist_tpl<dir_entry_t>, const& i, entries) {
			button_t*    const button1 = i.del;
			button_t*    const button2 = i.button;
			gui_label_t* const label   = i.label;

			if(i.type == LI_HEADER) {
				label->set_pos(scr_coord(10, y+4));
				button_frame.add_component(label);
				if(this->num_sections < 2) {
					// If just 1 section added, we won't print the header, skipping the y increment
					label->set_visible(false);
					continue;
				}
			}
			else{
				button1->set_size(scr_size(14, D_BUTTON_HEIGHT));
				button1->set_text("X");
				button1->set_pos(scr_coord(5, y));
#ifdef SIM_SYSTEM_TRASHBINAVAILABLE
				button1->set_tooltip("Send this file to the system trash bin. SHIFT+CLICK to permanently delete.");
#else
				button1->set_tooltip("Delete this file.");
#endif

				button2->set_pos(scr_coord(25, y));
				button2->set_size(scr_size(140, D_BUTTON_HEIGHT));

				label->set_pos(scr_coord(170, y+2));

				button1->add_listener(this);
				button2->add_listener(this);

				button_frame.add_component(button1);
				button_frame.add_component(button2);
				button_frame.add_component(label);
			}

			y += D_BUTTON_HEIGHT;
		}
		// since width was maybe increased, we only set the heigth.
		button_frame.set_size(scr_size(get_windowsize().w-1, y));
		set_windowsize(scr_size(get_windowsize().w, D_TITLEBAR_HEIGHT+12+y+30+1));
	}
}


void savegame_frame_t::set_file_table_default_sort_order()
{
}


void savegame_frame_t::add_file(const char *fullpath, const char *filename, const bool not_cutting_suffix ) 
{
	add_file(fullpath, filename, get_info(fullpath), not_cutting_suffix);
}

void savegame_frame_t::add_file(const char *fullpath, const char *filename, const char *pak, const bool no_cutting_suffix )
{
	if (use_table) {
		//char pathname[1024];
		//sprintf( pathname, "%s%s", fullpath ? fullpath : "", filename );
		char buttontext[1024];
		strcpy( buttontext, filename );
		if ( !no_cutting_suffix ) {
			buttontext[strlen(buttontext)-4] = '\0';
		}
		file_table.add_row( new gui_file_table_row_t( fullpath, buttontext ));
	}
	else {
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

		std::string const compare_to = !env_t::objfilename.empty() ? env_t::objfilename.substr(0, env_t::objfilename.size() - 1) + " -" : std::string();
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
		del->set_typ( button_t::roundbox );
		entries.insert(i, dir_entry_t(button, del, l, LI_ENTRY, fullpath));
	}
}



/**
 * INFO WIN EVENT
 * This dialogue's message event handler. The enter key is dispateched as
 * an action button click event. The WIN_OPEN event starts to fill the file
 * list if it is empty.
 *
 * @param event  The received event message.
 *
 * @retval true   Stop event propagation.
 * @retval false  Continue event propagation.
 */
bool savegame_frame_t::infowin_event(const event_t *event)
{
	if(event->ev_class == INFOWIN  &&  event->ev_code == WIN_OPEN  &&  entries.empty()) {
		// before no virtual functions can be used ...
		fill_list();
	}
	if(  event->ev_class == EVENT_KEYBOARD  &&  event->ev_code == 13  ) {
		action_triggered(&input, (long)0);
		return true; // swallowed
	}
	return gui_frame_t::infowin_event(event);
}



// true, if this is a correct file
bool savegame_frame_t::check_file(const char *filename, const char *suffix)
{
	// assume truth, if there is no pattern to compare
	return suffix==NULL  ||  (strncmp(filename+strlen(filename)-4, suffix, 4) == 0);
}


/**
 * ACTION TRIGGERED
 * Click event handler and dispatcher. This function is called
 * every time a button is clicked and the corresponding handler
 * is called from here.
 * @author Hj. Malthaner
 * @author Max Kielland
 *
 * @param component  The component that was clicked.
 *
 * @retval true      This function always returns true to stop
 *                   the event propagation.
 */
bool savegame_frame_t::action_triggered(gui_action_creator_t *component, value_t p)
{
	char buf[1024];

	if(component==&input  ||  component==&savebutton) {
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
		ok_action(buf);
		destroy_win(this);      //29-Oct-2001         Markus Weber    Close window

	}
	else if(component == &cancelbutton) {
		// Cancel-button pressed
		//----------------------------
		cancel_action(buf);
		destroy_win(this);      //29-Oct-2001         Markus Weber    Added   savebutton case
	}
	else if (component == &file_table) {
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
										if (item_action(row->get_name())) {
											destroy_win(this);
										}
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
		FOR(slist_tpl<dir_entry_t>, const& i, entries) {
			if(in_action){
				break;
			}
			if(component==i.button  ||  component==i.del) {
				in_action = true;
				bool const action_btn = component == i.button;

				if(action_btn) {
					if(item_action(i.info)) {
						destroy_win(this);
					}
				}
				else {
					if(del_action(i.info)) {
						destroy_win(this);
					}
					else {
						// do not delete components
						// simply hide them
						i.button->set_visible(false);
						i.del->set_visible(false);
						i.label->set_visible(false);
						i.button->set_size(scr_size(0, 0));
						i.del->set_size(scr_size(0, 0));

						resize(scr_coord(0, 0));
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
 * DELETE ACTION
 * Generic delete button click handler. This will delete the
 * item from the storage media. If the system supports a
 * trash bin, the file is moved over there instead of being deleted.
 * A shift + Delete always deletes the file imediatly
 * @author Hansjörg Malthaner
 *
 * @param fullpath  Full path to the file being deleted.
 *
 * @retval false    This function always return false to prevent the
 *                  dialogue from being closed.
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



/**
 * SET WINDOW SIZE
 * Position and resize components when teh dialog size changes.
 * The delete button is untouched regarding x position and width.
 * Action button and label are sharing the space where the button width
 * occupies BUTTON_COL_PERCENT % of the space.
 * If the delete button is hidden, the button and label shares the full width
 * and if the label is hidden the button occpies the 100% of the shared space.
 * @author Hj. Malthaner
 * @author Mathew Hounsell
 * @author Max Kielland
 *
 * @param size  The new dialogue size.
 */
void savegame_frame_t::set_windowsize(scr_size size)
{
	//const scr_coord_val label_y_offset = D_GET_CENTER_ALIGN_OFFSET(D_LABEL_HEIGHT,D_BUTTON_HEIGHT);
	const scr_coord_val row_height     = D_FOCUS_OFFSET_V + max( D_LABEL_HEIGHT, D_BUTTON_HEIGHT );
	const scr_coord_val width          = size.w - D_MARGIN_RIGHT;
	      scr_coord_val y              = D_FOCUS_OFFSET_V; // leave room for focus frame
	      sint16        curr_section   = 0;

	size.clip_rightbottom( scr_coord( display_get_width(), display_get_height()-env_t::iconsize.h-D_STATUSBAR_HEIGHT ) );

	gui_frame_t::set_windowsize(size);
	size = get_windowsize();

	// Adjust filename edit box
	input.set_size  ( scr_size( width - input.get_pos().x, D_EDIT_HEIGHT ) );
	scrolly.set_size( scr_size( size.w - D_MARGINS_X, size.h - L_FIXED_DIALOG_HEIGHT ) );

	if (use_table)
	{
		scrolly.set_show_scroll_y(file_table.get_table_height() > scrolly.get_size().h);
		sint32 c = file_table.get_grid_size().get_x();
		if (c > 0) {
			c = c > 0 ? 1 : 0;
			// "w" was formerly "x". This may need to be "h" instead, as the conversion is not clear. 
			sint16 x = scrolly.get_client().get_size().w - (file_table.get_table_width() - file_table.get_column_width(c));
			file_table.get_column(c)->set_width(x);
		}
	}
	else
	{

		const scr_coord_val button_width = (scr_coord_val) ((size.w - D_MARGINS_X - (delete_enabled * (D_BUTTON_HEIGHT + D_H_SPACE)) - D_H_SPACE) * L_BUTTON_COL_PERCENT );
		const scr_coord_val label_width =  (scr_coord_val) ( scrolly.get_client().w -  (delete_enabled * (D_BUTTON_HEIGHT + D_H_SPACE)) - button_width - D_H_SPACE);

		// Arrange list elements (file names)
		FOR(slist_tpl<dir_entry_t>, const& i, entries) {

			// resize all but delete button
			// header entry, it's only shown if we have more than one
			if(  i.type == LI_HEADER  &&  num_sections > 1  ) {
				i.label->set_pos(scr_coord(0, y + 2));
				y += row_height;
				curr_section++;
				continue;
			}

			if(  i.type == LI_HEADER  &&  num_sections < 2  ){
				// skip this header, we don't increment y
				curr_section++;
				continue;
			}

			if(  i.button->is_visible()  ) {
				button_t* const delete_button = i.del;
				button_t* const action_button = i.button;

				// We disable the delete button for extra sections entries
				if(curr_section > 1  ||  !delete_enabled) {
					delete_button->set_visible(false);
				}
				else {
					delete_button->set_pos(scr_coord(D_FOCUS_OFFSET_H, y));
				}

				action_button->set_pos( scr_coord( D_FOCUS_OFFSET_H + delete_enabled * ( D_BUTTON_HEIGHT + D_H_SPACE ), y ) );
				if (label_enabled) {
					action_button->set_width(button_width);
					i.label->align_to( action_button, ALIGN_LEFT | ALIGN_EXTERIOR_H | ALIGN_CENTER_V, scr_coord( D_H_SPACE, 0 ) );
					i.label->set_width( label_width - (D_FOCUS_OFFSET_H<<1) );
				}
				else {
					i.label->set_visible( false );
					action_button->set_width( button_width + label_width - ( D_FOCUS_OFFSET_H<<1 ) );
				}

				y += row_height;
			}
		}
		button_frame.set_size( scr_size(scrolly.get_client().x, button_frame.get_min_boundaries().h) );
	}

	divider1.set_width(size.w-D_MARGINS_X);
	divider1.align_to(&scrolly,ALIGN_TOP | ALIGN_EXTERIOR_V | ALIGN_LEFT);
	cancelbutton.align_to(&divider1, ALIGN_RIGHT | ALIGN_TOP | ALIGN_EXTERIOR_V );
	savebutton.align_to(&cancelbutton, ALIGN_RIGHT | ALIGN_EXTERIOR_H | ALIGN_TOP, scr_coord( D_H_SPACE, 0 ) );
}



/**
 * SET FILE NAME
 * Sets the current filename in the input box
 *
 * @param file_name  A nul terminated string to assign the edit control.
 */
void savegame_frame_t::set_filename(const char *file_name)
{
	size_t len = strlen(file_name);
	if(len>=4  &&  len-SAVE_PATH_X_LEN-3<128) {
		if(strstart(file_name, SAVE_PATH_X)) {
			tstrncpy(ibuf, file_name+SAVE_PATH_X_LEN, len-SAVE_PATH_X_LEN-3 );
		}
		else {
			tstrncpy(ibuf, file_name, len-3);
		}
		input.set_text(ibuf, 128);
	}
}

void savegame_frame_t::press_file_table_button(coordinates_t &cell)
{
	pressed_file_table_button = cell;
	file_table_button_pressed = true;
	for (coordinate_t i = file_table.get_grid_size().get_x(); i > 0; ) {
		--i;
		((gui_file_table_column_t*)file_table.get_column(i))->set_pressed(i == cell.get_x());
	}
	for (coordinate_t i = file_table.get_grid_size().get_y(); i > 0; ) {
		--i;
		((gui_file_table_row_t*)file_table.get_row(i))->set_pressed(i == cell.get_y());
	}
}

void savegame_frame_t::release_file_table_button()
{
	file_table_button_pressed = false;
	for (coordinate_t i = file_table.get_grid_size().get_x(); i > 0; ) {
		--i;
		((gui_file_table_column_t*)file_table.get_column(i))->set_pressed(false);
	}
	for (coordinate_t i = file_table.get_grid_size().get_y(); i > 0; ) {
		--i;
		((gui_file_table_row_t*)file_table.get_row(i))->set_pressed(false);
	}
}


// BG, 26.03.2010
void gui_file_table_button_column_t::paint_cell(const scr_coord& offset, coordinate_t /*x*/, coordinate_t /*y*/, const gui_table_row_t &row) 
{
 	gui_file_table_row_t &file_row = (gui_file_table_row_t&)row;
	scr_size size = scr_size(get_width(), row.get_height());
	scr_coord mouse(get_mouse_x() - offset.x, get_mouse_y() - offset.y);
	if (0 <= mouse.x && mouse.x < size.w && 0 <= mouse.y && mouse.y < size.h){ 
		btn.set_typ(button_t::roundbox);
	}
	else
	{
		btn.set_typ(button_t::box);
	}
	btn.pressed = pressed && file_row.pressed;
	// set size after type as type sets size to a default size as well.
	btn.set_size(size);
	btn.draw(offset);
}


// BG, 06.04.2010
void gui_file_table_delete_column_t::paint_cell(const scr_coord& offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row)
{
 	gui_file_table_row_t &file_row = (gui_file_table_row_t&)row;
	if (file_row.delete_enabled) {
		gui_file_table_button_column_t::paint_cell(offset, x, y, row);
	}

}


// BG, 26.03.2010
void gui_file_table_label_column_t::paint_cell(const scr_coord& offset, coordinate_t /*x*/, coordinate_t /*y*/, const gui_table_row_t &row) 
{
	lbl.set_pos(scr_coord(2, 2));
	lbl.set_size(scr_size(get_width() - 2, row.get_height() - 2));
	lbl.draw(offset);
}


// BG, 26.03.2010
const char *gui_file_table_action_column_t::get_text(const gui_table_row_t &row) const 
{ 
 	gui_file_table_row_t &file_row = (gui_file_table_row_t&)row;
	return file_row.text.c_str();
}


// BG, 26.03.2010
void gui_file_table_action_column_t::paint_cell(const scr_coord& offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row) {
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
void gui_file_table_time_column_t::paint_cell(const scr_coord& offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row) {
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
void gui_file_table_t::paint_cell(const scr_coord& offset, coordinate_t x, coordinate_t y)
{
	gui_file_table_column_t *column_def = (gui_file_table_column_t *)get_column(x);
	gui_table_row_t *row_def = get_row(y);
	if (column_def && row_def)
	{
		column_def->paint_cell(offset, x, y, *row_def);
	}
}


/**
 * CLEAN UP PATH - ONLY WIN32
 * Translates all / into \ in a given path string. If a drive
 * letter is present it is translated to upper case
 *
 * @param path  A pointer to the path string. This string is modified.
 *
 * @return      The function is not returning anything per say, but the
 *              contents of the path parameter has been modified on return.
 */
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



/**
 * SHORTEN PATH
 * Outputs a truncated path by replacing the middle portion with "..."
 *
 * @param dest      Destination string.
 * @param source    Nul terminated source string to parse.
 * @param max_size  Truncate the string to this number of characters.
 */
void savegame_frame_t::shorten_path(char *dest,const char *source,const size_t max_size)
{
	assert (max_size > 2);

	const size_t orig_size = strlen(source);

	if ( orig_size <= max_size ) {
		strcpy(dest,source);
		return;
	}

	const int half = max_size/2;
	const int odd = max_size%2;

	strncpy(dest,source,half-1);
	strncpy(&dest[half-1],"...",3);
	strcpy(&dest[half+2],&source[orig_size-half+2-odd]);

}



/**
 * GET BASE NAME
 * Returns the path portion of a qualified filename including path.
 *
 * @param fullpath A null terminated string with a full qualified file name.
 */
std::string savegame_frame_t::get_basename(const char *fullpath)
{
	std::string path = fullpath;
	size_t last = path.find_last_of("\\/");
	if (last==std::string::npos){
		return path;
	}
	return path.substr(0,last+1);
}



/**
 * GET FILE NAME
 * Returns the file name without extension (optional) of a qualified filename
 * including path.
 *
 * @param fullpath A null terminated string with a full qualified file name.
 * @param with_extension  If true, the extension is removed from the filename.
 *
 * @retval std::string  The filename without extension.
 */
std::string savegame_frame_t::get_filename(const char *fullpath,const bool with_extension) const
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
