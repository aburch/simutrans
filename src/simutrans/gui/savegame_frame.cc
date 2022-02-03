/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <string.h>
#include <stdio.h>

#include "savegame_frame.h"
#include "../pathes.h"
#include "../sys/simsys.h"
#include "../simdebug.h"
#include "simwin.h"
#include "../utils/simstring.h"
#include "../utils/searchfolder.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"

#define L_DEFAULT_ROWS       (12)   // Number of file entries to show as default
#define L_SHORTENED_SIZE   (48)

/**
 * Small helper class for tiny 'X' delete buttons
 */
class del_button_t : public button_t
{
	scr_coord_val w;
public:
	del_button_t() : button_t()
	{
		init(button_t::roundbox, "X");
		w = max(D_BUTTON_HEIGHT, display_get_char_width('X') + D_BUTTON_PADDINGS_X);
	}
	scr_size get_min_size() const OVERRIDE
	{
		return scr_size(w, D_BUTTON_HEIGHT);
	}
};

/**
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
savegame_frame_t::savegame_frame_t(const char *suffix, bool only_directories, const char *path, const bool delete_enabled) : gui_frame_t( translator::translate("Load/Save") ),
	suffix(suffix),
	in_action(false),
	only_directories(only_directories),
	searchpath_defined(false),
	input(),
	fnlabel("Filename"),
	scrolly(&button_frame, true),
	num_sections(0),
	delete_enabled(delete_enabled)
{
	set_table_layout(1,0);
	label_enabled = true;

	// Filename input
	top_frame.set_table_layout(2,0);
	add_component(&top_frame);
	{
		top_frame.add_component(&fnlabel);

		tstrncpy(ibuf, "", lengthof(ibuf));
		input.set_text(ibuf, 128);
		top_frame.add_component(&input);
	}

	// Needs to be scrollable, size is adjusted in set_windowsize()
	scrolly.set_scroll_amount_y(D_BUTTON_HEIGHT + D_FOCUS_OFFSET_V);
	scrolly.set_size_corner(false);
	add_component(&scrolly);
	scrolly.set_maximize(true);

	// Controls below will be sized and positioned in set_windowsize()
	new_component<gui_divider_t>();

	add_table(3,1);
	add_component(&bottom_left_frame);
	bottom_left_frame.set_table_layout(1,0);

	new_component<gui_fill_t>();

	add_table(2,1)->set_force_equal_columns(true);
	savebutton.init( button_t::roundbox | button_t::flexible, "Ok" );
	savebutton.add_listener( this );
	add_component( &savebutton );

	cancelbutton.init( button_t::roundbox | button_t::flexible, "Cancel" );
	cancelbutton.add_listener( this );
	add_component( &cancelbutton );
	end_table();

	end_table();

	top_frame.set_focus( &input );
	set_focus(&top_frame);

	if(this->suffix == NULL) {
		this->suffix = "";
	}

	if(path != NULL) {
		this->add_path(path);
		// needed?
		dr_mkdir(path);
	}

	set_resizemode(diagonal_resize);
}



/**
 * Free all list items.
 */
savegame_frame_t::~savegame_frame_t()
{
	for(dir_entry_t const& i : entries) {
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

	const size_t data_dir_len = strlen(env_t::data_dir);

	if(  name[0]=='/'  ||  name[0]=='\\'  ||  name[1]==':'  ||  strncmp(name.c_str(),env_t::data_dir,data_dir_len) == 0  ) {
		// starts with data_dir or an absolute path
		tstrncpy(path_expanded, name.c_str(), FILENAME_MAX);
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
	for(std::string &path : paths){

		const char *path_c      = path.c_str();
		const size_t path_c_len = strlen(path_c);

		sf.search(path, std::string(suffixnodot), this->only_directories, false);

		bool section_added = false;

		// Add the entries that pass the check
		for(const char* const &name : sf) {
			fullname = new char [path_c_len+strlen(name)+1];
			sprintf(fullname,"%s%s",path_c,name);

			if(  check_file(fullname, suffix)  ){
				if(!section_added) {
					add_section(path);
					section_added = true;
				}
				add_file(fullname, name, get_info(fullname), not_cutting_extension);
			}
			else {
				// NOTE: we just free "fullname" memory when add_file is not called. That memory will be
				// freed in the class destructor. This way we save the cost of re-allocate/copy it inside there
				delete [] fullname;
			}
		}

	}
}



/**
 * All items has been inserted in the list.
 * On return resize() is called and all item's GYU members are positioned and resized.
 * Therefore it is no use to set the button's and label's width or any items y position.
 * The only control keeping its size is the delete button.
 */
void savegame_frame_t::list_filled( void )
{
	uint cols = (delete_enabled ? 1 : 0) + 1 + (label_enabled ? 1 : 0);
	button_frame.set_table_layout(1,0);
	button_frame.add_table(cols,0)->set_spacing(scr_size(D_H_SPACE,D_FILELIST_V_SPACE)); // change space between entries to zero to see more on screen

	for(dir_entry_t const& i : entries) {
		button_t*    const delete_button = i.del;
		button_t*    const action_button = i.button;
		gui_label_t* const label   = i.label;

		if(i.type == LI_HEADER) {
			if(this->num_sections < 2) {
				// If just 1 section added, we won't print the header
				label->set_visible(false);
				continue;
			}
			button_frame.add_component(label, cols);
		}
		else {

			if (dr_cantrash()) {
				delete_button->set_tooltip("Send this file to the system trash bin. SHIFT+CLICK to permanently delete.");
			} else {
				delete_button->set_tooltip("Delete this file.");
			}

			delete_button->add_listener(this);
			action_button->add_listener(this);

			if (delete_enabled) {
				button_frame.add_component(delete_button);
			}
			button_frame.add_component(action_button);
			if (label_enabled) {
				button_frame.add_component(label);
			}
		}

	}
	button_frame.end_table();

	const scr_coord_val row_height = max( D_LABEL_HEIGHT, D_BUTTON_HEIGHT ) + D_V_SPACE;

	reset_min_windowsize();
	scr_size size = get_min_size() + scr_size(0, min(entries.get_count(), L_DEFAULT_ROWS) * row_height);
	// TODO do something smarter here
	size.w = max(size.w, button_frame.get_min_size().w + D_SCROLLBAR_WIDTH);
	set_windowsize(size);
}



/**
 * Check if a file name qualifies to be added to he item list.
 *
 * @param filename  The found file name
 * @param suffix    Suffix pattern to match. If this is NULL
 *                  all filenames qualifies.
 *
 * @retval true     The filename qualified and will be added.
 * @retval false    This file doesn't qualify, don't add.
 */
bool savegame_frame_t::check_file(const char *filename, const char *suffix)
{
	// assume truth, if there is no pattern to compare
	return  suffix==NULL  ||  suffix[0]==0  ||  (strncmp(filename+strlen(filename)-4, suffix, 4)== 0);
}



/**
 * Create and add a list item from the given parameters. The button is set
 * to the filename, the label to the string returned by get_info().
 *
 * @param fullpath           The full path to associate with this item.
 * @param filename           The file name to assign the action button (i.button).
 * @param info               Information to set in the label.
 * @param no_cutting_suffix  Keep the suffix (true) in the file name.
 */
void savegame_frame_t::add_file(const char *fullpath, const char *filename, const char *info, const bool no_cutting_suffix)
{
	button_t *button = new button_t();
	char *name = new char[strlen(filename)+10];
	char *text = new char[strlen(info)+1];

	strcpy(text, info);
	strcpy(name, filename);

	if(!no_cutting_suffix) {
		name[strlen(name)-4] = '\0';
	}
	button->set_typ( button_t::roundbox | button_t::flexible);
	button->set_no_translate(true);
	button->set_text(name); // to avoid translation

	std::string const compare_to = !env_t::objfilename.empty() ? env_t::objfilename.substr(0, env_t::objfilename.size() - 1) + " -" : std::string();
	// sort descending with respect to compare_items
	slist_tpl<dir_entry_t>::iterator i = entries.begin();
	slist_tpl<dir_entry_t>::iterator end = entries.end();

	// This needs optimizing, advance to the last section, since inserts come always to the last section, we could just update  last one on last_section

	slist_tpl<dir_entry_t>::iterator lastfound;
	while(i != end) {
		if(i->type == LI_HEADER) {
			lastfound = i;
		}
		i++;
	}
	i = ++lastfound;

	// END of optimizing

	if(!strstart(info, compare_to.c_str())) {
		// skip current ones
		while(i != end) {
			// extract pakname in same format than in savegames ...
			if(!strstart(i->label->get_text_pointer(), compare_to.c_str())) {
				break;
			}
			++i;
		}
		// now sort with respect to label on button or info text (ie date)
		while(i != end) {
			if( compare_items(*i, text, name ) ) {
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

			if(compare_items( *i, text, name ) ) {
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
	l->set_text_pointer(text);
	entries.insert(i, dir_entry_t(button, new del_button_t(), l, LI_ENTRY, fullpath));
}



/**
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

		// Notify of the end
		list_filled();
	}
	if(  event->ev_class == EVENT_KEYBOARD  &&  event->ev_code == 13  ) {
		action_triggered(&input, (long)0);
		return true; // swallowed
	}
	return gui_frame_t::infowin_event(event);
}



/**
 * Click event handler and dispatcher. This function is called
 * every time a button is clicked and the corresponding handler
 * is called from here.
 *
 * @param component  The component that was clicked.
 *
 * @retval true      This function always returns true to stop
 *                   the event propagation.
 */
bool savegame_frame_t::action_triggered(gui_action_creator_t *component, value_t )
{
	char buf[PATH_MAX];

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
		destroy_win(this);

	}
	else if(component == &cancelbutton) {
		// Cancel-button pressed
		//----------------------------
		cancel_action(buf);
		destroy_win(this);
	}
	else {
		// File in list selected
		//--------------------------
		for(dir_entry_t const& i : entries) {
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

						resize(scr_coord(0, 0));
					}
				}
				in_action = false;
				break;
			}
		}
	}
	return true;
}



/**
 * Generic delete button click handler. This will delete the
 * item from the storage media. If the system supports a
 * trash bin, the file is moved over there instead of being deleted.
 * A shift + Delete always deletes the file imediatly
 *
 * @param fullpath  Full path to the file being deleted.
 *
 * @retval false    This function always return false to prevent the
 *                  dialogue from being closed.
 */
bool savegame_frame_t::del_action(const char *fullpath)
{
	if (!dr_cantrash() || event_get_last_control_shift() & 1) {
		// shift pressed, delete without trash bin
		dr_remove(fullpath);
		return false;
	}

	dr_movetotrash(fullpath);
	return false;
}


/**
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



/**
 * ONLY WIN32
 * Translates all / into \ in a given path string. If a drive
 * letter is present it is translated to upper case
 *
 * @param path  A pointer to the path string. This string is modified.
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
	memcpy(&dest[half-1],"...",sizeof(char) * 3);
	strcpy(&dest[half+2],&source[orig_size-half+2-odd]);

}



/**
 * Returns the path portion of a qualified filename including path.
 *
 * @param fullpath  A null terminated string with a full qualified file name.
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
 * Returns the file name without extension (optional) of a qualified filename
 * including path.
 *
 * @param fullpath            A nul terminated string with a full qualified file name.
 * @param with_extension  If true, the extension is removed from the filename.
 *
 * @retval std::string  The filename without extension.
 */
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



bool savegame_frame_t::compare_items ( const dir_entry_t & entry, const char *, const char *name )
{
	return (strcmp(name, entry.button->get_text()) < 0);
}
