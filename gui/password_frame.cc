/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include "../simdebug.h"
#include "../simwerkz.h"
#include "../simwin.h"
#include "../simworld.h"

#include "../dataobj/translator.h"
#include "../dataobj/network_cmd_ingame.h"

#include "../utils/cbuffer_t.h"
#include "../utils/sha1.h"
#include "../utils/simstring.h"


#include "password_frame.h"
#include "player_frame_t.h"

#define DIALOG_WIDTH (360)


password_frame_t::password_frame_t( spieler_t *sp ) :
	gui_frame_t( translator::translate("Enter Password"), sp )
{
	this->sp = sp;

	if(  !sp->is_locked()  ||  (sp->get_welt()->get_active_player_nr()==1  &&  !sp->get_welt()->get_spieler(1)->is_locked())   ) {
		// allow to change name name
		tstrncpy( player_name_str, sp->get_name(), lengthof(player_name_str) );
		player_name.set_text(player_name_str, lengthof(player_name_str));
		player_name.add_listener(this);
		player_name.set_pos(koord(10,4));
		player_name.set_groesse(koord(DIALOG_WIDTH-10-10, D_BUTTON_HEIGHT));
		add_komponente(&player_name);
	}
	else {
		const_player_name.set_text( sp->get_name() );
		const_player_name.set_pos(koord(10,4));
		add_komponente(&const_player_name);
	}


	fnlabel.set_pos (koord(10,4+D_BUTTON_HEIGHT+6));
	fnlabel.set_text( "Password" );
	add_komponente(&fnlabel);

	// Input box for password
	ibuf[0] = 0;
	password.set_text(ibuf, lengthof(ibuf) );
	password.add_listener(this);
	password.set_pos(koord(75,4+D_BUTTON_HEIGHT+4));
	password.set_groesse(koord(DIALOG_WIDTH-75-10, D_BUTTON_HEIGHT));
	add_komponente(&password);
	set_focus( &password );

	set_fenstergroesse(koord(DIALOG_WIDTH, 16+12+2*D_BUTTON_HEIGHT));
}




/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool password_frame_t::action_triggered( gui_action_creator_t *komp, value_t p )
{
	if(komp == &password  &&  (ibuf[0]!=0  ||  p.i == 1)) {
		// Enter-Key pressed
		// test for matching password to unlock
		SHA1 sha1;
		size_t len = strlen( password.get_text() );
		sha1.Input( password.get_text(), len );
		pwd_hash_t hash;
		// remove hash to re-open slot if password is empty
		if(len>0) {
			hash.set(sha1);
		}
		// store the hash
		sp->get_welt()->store_player_password_hash( sp->get_player_nr(), hash );

		if(  umgebung_t::networkmode) {
			sp->unlock(!sp->is_locked(), true);
			// send hash to server: it will unlock player or change password
			nwc_auth_player_t *nwc = new nwc_auth_player_t(sp->get_player_nr(), hash);
			network_send_server(nwc);
		}
		else {
			/* if current active player is player 1 and this is unlocked, he may reset passwords
			 * otherwise you need the valid previous password
			 */
			if(  !sp->is_locked()  ||  (sp->get_welt()->get_active_player_nr()==1  &&  !sp->get_welt()->get_spieler(1)->is_locked())   ) {
				// set password
				sp->access_password_hash() = hash;
				sp->unlock(true, false);
			}
			else {
				sp->check_unlock(hash);
			}
		}
	}

	if(  komp == &player_name  ) {
		// rename a player
		cbuffer_t buf;
		buf.printf( "p%u,%s", sp->get_player_nr(), player_name.get_text() );
		werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
		w->set_default_param( buf );
		sp->get_welt()->set_werkzeug( w, sp );
		// since init always returns false, it is safe to delete immediately
		delete w;
	}

	if(  p.i==1  ) {
		// destroy window after enter is pressed
		destroy_win(this);
	}
	return true;
}
