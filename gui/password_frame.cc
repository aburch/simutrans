/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include "../simdebug.h"
#include "../simwerkz.h"
#include "../simwin.h"
#include "../simworld.h"

#include "../utils/sha1.h"
#include "../utils/simstring.h"

#include "components/list_button.h"
#include "password_frame.h"
#include "player_frame_t.h"

#define DIALOG_WIDTH (360)


password_frame_t::password_frame_t( spieler_t *sp ) :
	gui_frame_t("Enter Password",sp)
{
	this->sp = sp;

	if(  !sp->is_locked()  ||  (sp->get_welt()->get_active_player_nr()==1  &&  !sp->get_welt()->get_spieler(1)->is_locked())   ) {
		// allow to change name name
		tstrncpy( player_name_str, sp->get_name(), lengthof(player_name_str) );
		player_name.set_text(player_name_str, lengthof(player_name_str));
		player_name.add_listener(this);
		player_name.set_pos(koord(10,4));
		player_name.set_groesse(koord(DIALOG_WIDTH-10-10, BUTTON_HEIGHT));
		add_komponente(&player_name);
	}
	else {
		const_player_name.set_text( sp->get_name() );
		const_player_name.set_pos(koord(10,4));
		add_komponente(&const_player_name);
	}


	fnlabel.set_pos (koord(10,4+BUTTON_HEIGHT+6));
	fnlabel.set_text( "Password" );
	add_komponente(&fnlabel);

	// Input box for password
	ibuf[0] = 0;
	password.set_text(ibuf, lengthof(ibuf) );
	password.add_listener(this);
	password.set_pos(koord(75,4+BUTTON_HEIGHT+4));
	password.set_groesse(koord(DIALOG_WIDTH-75-10, BUTTON_HEIGHT));
	add_komponente(&password);
	set_focus( &password );

	set_fenstergroesse(koord(DIALOG_WIDTH, 16+12+2*BUTTON_HEIGHT));
}




/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool password_frame_t::action_triggered( gui_action_creator_t *komp, value_t p )
{
	if(komp == &password) {
		// Enter-Key pressed
		// test for matching password to unlock
		SHA1 sha1;
		sha1.Input( password.get_text(), strlen( password.get_text() ) );
		uint8 hash[20];
		MEMZERO(hash);
		sha1.Result( hash );
		/* if current active player is player 1 and this is unlocked, he may reset passwords
		 * otherwise you need the valid previous password
		 */
		if(  !sp->is_locked()  ||  (sp->get_welt()->get_active_player_nr()==1  &&  !sp->get_welt()->get_spieler(1)->is_locked())   ) {
			// set this to world
			sp->get_welt()->set_player_password_hash( sp->get_player_nr(), hash );
			// and change player password
			werkzeug_t *w = create_tool( WKZ_PWDHASH_TOOL | SIMPLE_TOOL );
			cbuffer_t buf(512);
			for(  int i=0;  i<20;  i++  ) {
				buf.printf( "%02X", hash[i] );
			}
			w->set_default_param(buf);
			sp->get_welt()->set_werkzeug( w, sp );
			delete w;
		}
		else {
			// set this to world to unlock
			sp->get_welt()->set_player_password_hash( sp->get_player_nr(), hash );
			sp->set_unlock( hash );
			// update the player window
			ki_kontroll_t* playerwin = (ki_kontroll_t*)win_get_magic(magic_ki_kontroll_t);
			if (playerwin) {
				playerwin->update_data();
			}
		}
	}

	if(  komp == &player_name  ) {
		// rename a player
		cbuffer_t buf(300);
		buf.printf( "p%u,%s", sp->get_player_nr(), player_name.get_text() );
		werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
		w->set_default_param( buf );
		sp->get_welt()->set_werkzeug( w, sp );
		// since init always returns false, it is save to delete immediately
		delete w;
	}

	if(  p.i==1  ) {
		// destroy window after enter is pressed
		destroy_win(this);
	}
	return true;
}
