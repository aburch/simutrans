/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"

#include <sys/stat.h>

#include "loadsave_frame.h"

#include "../sys/simsys.h"
#include "../world/simworld.h"
#include "../simversion.h"
#include "../pathes.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../network/network.h"
#include "../network/network_cmd.h"
#include "../network/network_cmd_ingame.h"
#include "../network/network_socket_list.h"

#include "../utils/simstring.h"


stringhashtable_tpl<sve_info_t *> loadsave_frame_t::cached_info;


sve_info_t::sve_info_t(const char *pak_, time_t mod_, sint32 fs)
: pak(""), mod_time(mod_), file_size(fs)
{
	if(pak_) {
		pak = pak_;
		file_exists = true;
	}
}


bool sve_info_t::operator== (const sve_info_t &other) const
{
	return (mod_time==other.mod_time)  &&  (file_size == other.file_size)  &&  (pak.compare(other.pak)==0);
}


void sve_info_t::rdwr(loadsave_t *file)
{
	const char *s = strdup(pak.c_str());
	file->rdwr_str(s);
	if (file->is_loading()) {
		pak = s ? s : "<unknown pak>";
	}
	free(const_cast<char *>(s));
	file->rdwr_longlong(mod_time);
	file->rdwr_long(file_size);
}



/**
 * Action that's started with a button click
 */
bool loadsave_frame_t::item_action(const char *filename)
{
	if(do_load) {
		welt->switch_server( easy_server.pressed, true );
		long start_load = dr_time();
		if(  !welt->load(filename)  ) {
			welt->switch_server( false, true );
		}
		else if(  env_t::server  ) {
			welt->announce_server(karte_t::SERVER_ANNOUNCE_HELLO);
		}
		DBG_MESSAGE( "loadsave_frame_t::item_action", "load world %li ms", dr_time() - start_load );
	}
	else {
		// saving a game
		if(  env_t::server  ||  socket_list_t::get_playing_clients() > 0  ) {
			network_reset_server();
#if 0
// TODO: saving without kicking all clients off ...
			// we have connected clients, so we do a sync
			const uint32 new_map_counter = welt->generate_new_map_counter();
			nwc_sync_t *nw_sync = new nwc_sync_t(welt->get_sync_steps() + 1, welt->get_map_counter(), -1, new_map_counter);
			network_send_all(nw_sync, false);
			// and now we need to copy the servergame to the map ...
#endif
		}
		long start_save = dr_time();
		welt->save( filename, false, env_t::savegame_version_str, false );
		DBG_MESSAGE( "loadsave_frame_t::item_action", "save world %li ms", dr_time() - start_save );
		welt->set_dirty();
		welt->reset_timer();
	}

	return true;
}



bool loadsave_frame_t::ok_action(const char *filename)
{
	return item_action(filename);
}



loadsave_frame_t::loadsave_frame_t(bool do_load) : savegame_frame_t(".sve",false,"save/",env_t::show_delete_buttons)
{
	this->do_load = do_load;

	if(do_load) {
		set_name(translator::translate("Laden"));
		easy_server.init( button_t::square_automatic, "Start this as a server");
		bottom_left_frame.add_component(&easy_server);
	}
	else {
		set_filename(welt->get_settings().get_filename());
		set_name(translator::translate("Speichern"));
	}

	// load cached entries
	if (cached_info.empty()) {
		loadsave_t file;
		/* We rename the old cache file and remove any incomplete read version.
		 * Upon an error the cache will be rebuilt then next time.
		 */
		dr_rename( SAVE_PATH_X "_cached.xml", SAVE_PATH_X "_load_cached.xml" );
		if(  file.rd_open(SAVE_PATH_X "_load_cached.xml") == loadsave_t::FILE_STATUS_OK  ) {
			// ignore comment
			const char *text=NULL;
			file.rdwr_str(text);

			bool ok = true;
			while(ok) {
				xml_tag_t t(&file, "save_game_info");
				// first filename
				file.rdwr_str(text);
				if (!strempty(text)) {
					sve_info_t *svei = new sve_info_t();
					svei->rdwr(&file);
					cached_info.put(text, svei);
					text = NULL; // it is used as key, do not delete it
				}
				else {
					ok = false;
				}
			}
			if (text) {
				free(const_cast<char *>(text));
			}
			file.close();
			dr_rename( SAVE_PATH_X "_load_cached.xml", SAVE_PATH_X "_cached.xml" );
		}
	}
}


/**
 * Set the window associated helptext
 * @return the filename for the helptext, or NULL
 */
const char *loadsave_frame_t::get_help_filename() const
{
	return do_load ? "load.txt" : "save.txt";
}


const char *loadsave_frame_t::get_info(const char *fname)
{
	static char date[1024];

	std::string pak_extension;

	// get file information
	struct stat  sb;
	if(dr_stat(fname, &sb) != 0) {
		// file not found?
		date[0] = 0;
		return date;
	}

	// check hash table
	sve_info_t *svei = cached_info.get(fname);
	if (svei   &&  svei->file_size == sb.st_size  &&  svei->mod_time == sb.st_mtime) {
		// compare size and mtime
		// if both are equal then most likely the files are the same
		// no need to read the file for pak_extension
		pak_extension = svei->pak.c_str();
		svei->file_exists = true;
	}
	else {
		// read pak_extension from file
		loadsave_t test;
		test.rd_open(fname);
		// add pak extension
		pak_extension = test.get_pak_extension();

		// now insert in hash_table
		sve_info_t *svei_new = new sve_info_t(pak_extension.c_str(), sb.st_mtime, sb.st_size );
		// copy filename
		char *key = strdup(fname);
		sve_info_t *svei_old = cached_info.set(key, svei_new);
		delete svei_old;
	}

	// write everything in string
	// add pak extension
	const size_t n = snprintf( date, lengthof(date), "%s - ", pak_extension.c_str());

	// add the time too
	struct tm *tm = localtime(&sb.st_mtime);
	if(tm) {
		strftime(date+n, 18, "%Y-%m-%d %H:%M", tm);
	}
	else {
		tstrncpy(date, "??.??.???? ??:??", lengthof(date));
	}

	date[lengthof(date)-1] = 0;
	return date;
}


loadsave_frame_t::~loadsave_frame_t()
{
	// save hashtable
	loadsave_t file;
	const char *cache_file = SAVE_PATH_X "_cached.xml";

	if(  file.wr_open(cache_file, loadsave_t::xml, 0, "cache", SAVEGAME_VER_NR) == loadsave_t::FILE_STATUS_OK  ) {
		const char *text="Automatically generated file. Do not edit. An invalid file may crash the game. Deleting is allowed though.";
		file.rdwr_str(text);
		for(auto const& i : cached_info) {
			// save only existing files
			if (i.value->file_exists) {
				xml_tag_t t(&file, "save_game_info");
				char const* filename = i.key;
				file.rdwr_str(filename);
				i.value->rdwr(&file);
			}
		}
		// mark end with empty entry
		{
			xml_tag_t t(&file, "save_game_info");
			text = "";
			file.rdwr_str(text);
		}
		file.close();
	}
}

bool loadsave_frame_t::compare_items ( const dir_entry_t & entry, const char *info, const char *)
{
	return (strcmp(entry.label->get_text_pointer(), info) < 0);
}
