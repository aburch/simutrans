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
#include "../dataobj/sve_cache.h"

#include "../network/network.h"
#include "../network/network_cmd.h"
#include "../network/network_cmd_ingame.h"
#include "../network/network_socket_list.h"

#include "../utils/simstring.h"



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



loadsave_frame_t::loadsave_frame_t(bool do_load, bool back_to_menu) : savegame_frame_t(".sve",false,"save/",true, back_to_menu)
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

	sve_cache_t::load_cache();
}


const char *loadsave_frame_t::get_info(const char *fname)
{
	return sve_cache_t::get_info(fname);
}


/**
 * Set the window associated helptext
 * @return the filename for the helptext, or NULL
 */
const char *loadsave_frame_t::get_help_filename() const
{
	return do_load ? "load.txt" : "save.txt";
}


loadsave_frame_t::~loadsave_frame_t()
{
	// save hashtable
	sve_cache_t::write_cache();
}


bool loadsave_frame_t::compare_items ( const dir_entry_t & entry, const char *info, const char *)
{
	return (strcmp(entry.label->get_text_pointer(), info) < 0);
}
