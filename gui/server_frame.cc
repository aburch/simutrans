/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simworld.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simwin.h"
#include "../simmesg.h"
#include "../utils/simstring.h"

#include "components/list_button.h"
#include "../dataobj/translator.h"
#include "../dataobj/network.h"
#include "../dataobj/umgebung.h"
#include "server_frame.h"
#include "messagebox.h"



server_frame_t::server_frame_t(karte_t* w) :
	gui_frame_t("Game info"),
	welt(w),
	gi(welt),
	buf(1024),
	time(32),
	revision_buf(64)
{
	update_info();

	sint16 pos_y = 4;
	serverlist.set_pos( koord(2,pos_y) );
	serverlist.set_groesse( koord(236,BUTTON_HEIGHT) );
	serverlist.add_listener(this);
	serverlist.set_selection( 0 );
	add_komponente( &serverlist );

	pos_y += 6+BUTTON_HEIGHT;
	add.init( button_t::box, "add server", koord( 4, pos_y ), koord( 112, BUTTON_HEIGHT) );
	add.add_listener(this);
	add_komponente( &add );

	loadlist.init( button_t::box, "download list", koord( 124, pos_y ), koord( 112, BUTTON_HEIGHT) );
	loadlist.add_listener(this);
	add_komponente( &loadlist );

	pos_y += BUTTON_HEIGHT+8;
	date.set_pos( koord( 4, pos_y ) );
	add_komponente( &date );

	pos_y += 8*LINESPACE;

	revision.set_pos( koord( 4, pos_y ) );
	add_komponente( &revision );

	pos_y += LINESPACE;
	pak_version.set_pos( koord( 4, pos_y ) );
	add_komponente( &pak_version );

	pos_y += 5+LINESPACE;
	join.init( button_t::box, "join game", koord( 4, pos_y ), koord( 112, BUTTON_HEIGHT) );
	join.add_listener(this);
	add_komponente( &join );

	quit.init( button_t::box, "Beenden", koord( 124, pos_y ), koord( 112, BUTTON_HEIGHT) );
	quit.add_listener(this);
	add_komponente( &quit );

	pos_y += 6+BUTTON_HEIGHT+16;
	set_fenstergroesse( koord( 240, pos_y ) );
	set_min_windowsize( koord( 240, pos_y ) );
	set_resizemode( no_resize );
}


void server_frame_t::update_info()
{
	char temp[32];

	buf.clear();
	buf.printf( "%ux%u\n", gi.get_groesse_x(), gi.get_groesse_y() );
	if(  gi.get_clients()!=255  ) {
		uint8 player=0, locked = 0;
		for(  uint8 i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
			if(  gi.get_player_type(i)&~spieler_t::PASSWORD_PROTECTED  ) {
				player ++;
				if(  gi.get_player_type(i)&spieler_t::PASSWORD_PROTECTED  ) {
					locked ++;
				}
			}
		}
		buf.printf( translator::translate("%u Player (%u locked)\n"), player, locked );
		buf.printf( translator::translate("%u Client(s)\n"), (unsigned)gi.get_clients() );
	}
	buf.printf( "%s %u\n", translator::translate("Towns"), gi.get_anzahl_staedte() );
	number_to_string( temp, gi.get_einwohnerzahl(), 0 );
	buf.printf( "%s %s\n", translator::translate("citicens"), temp );
	buf.printf( "%s %u\n", translator::translate("Factories"), gi.get_industries() );
	buf.printf( "%s %u\n", translator::translate("Convoys"), gi.get_convoi_count() );
	buf.printf( "%s %u\n", translator::translate("Stops"), gi.get_halt_count() );

	revision_buf.clear();
	if(  serverlist.get_selection()>=0  ) {
		// need to compare with our world now
		gameinfo_t current(welt);

		revision_buf.printf( "%s %u", translator::translate( "Revision:" ), gi.get_game_engine_revision() );
		revision.set_text( revision_buf );
		revision.set_color( current.get_game_engine_revision()==gi.get_game_engine_revision() ? COL_BLACK : COL_RED );

		pak_version.set_text( gi.get_pak_name() );
		pak_version.set_color( strcmp(gi.get_pak_name(),current.get_pak_name())==0  &&  gi.get_with_private_paks()==false  ? COL_BLACK : COL_RED );

		join.enable();
	}
	else {
		revision_buf.printf( "%s %u", translator::translate( "Revision:" ), gi.get_game_engine_revision() );
		revision.set_text( revision_buf );
		revision.set_color( COL_BLACK );
		pak_version.set_text( gi.get_pak_name() );
		pak_version.set_color( COL_BLACK );
		join.disable();
	}

	time.clear();
	char const* const month = translator::get_month_name(gi.get_current_month());
	switch (umgebung_t::show_month) {
		case umgebung_t::DATE_FMT_GERMAN:
		case umgebung_t::DATE_FMT_MONTH:
		case umgebung_t::DATE_FMT_SEASON:
			time.printf( "%s %d", month, gi.get_current_year() );
			break;

		case umgebung_t::DATE_FMT_US:
		case umgebung_t::DATE_FMT_JAPANESE:
			time.printf( "%i/%s", gi.get_current_year(), month );
			break;
	}
	date.set_text( time );
}


bool server_frame_t::infowin_event(const event_t *ev)
{
	if(  ev->ev_class == EVENT_KEYBOARD  &&  ev->ev_code == SIM_KEY_ENTER  ) {
		action_triggered( &serverlist, value_t(serverlist.get_selection()) );
	}
	return gui_frame_t::infowin_event( ev );
}


bool server_frame_t::action_triggered( gui_action_creator_t *komp, value_t p )
{
	if(  &serverlist == komp  ) {
		if(  p.i==-1  ) {
			gi = gameinfo_t(welt);
			update_info();
			join.disable();
		}
		else {
			join.disable();
			if(  serverlist.get_element(p.i)->get_color()!=COL_WHITE  ) {
				const char *err = network_gameinfo( serverlist.get_element(p.i)->get_text(), &gi );
				if(  err==NULL  ) {
					serverlist.get_element(p.i)->set_color( COL_BLACK );
					update_info();
					join.enable();
				}
				else {
					serverlist.get_element(p.i)->set_color( COL_RED );
					join.disable();
				}
			}
		}
		set_dirty();
	}
	else if(  &add == komp  ) {
		serverlist.append_element( new gui_scrolled_list_t::var_text_scrollitem_t( "Enter name", COL_BLUE ) );
		serverlist.set_selection( serverlist.count_elements()-1 );
	}
	else if(  &loadlist == komp  ) {
		// download list from main server
//		network_download_http( "www.physik.tu-berlin.de:80", "/~prissi/simutrans/serverlist.txt", "serverlist.txt" );
		network_download_http( "simutrans-germany.com:80", "/serverlist/serverlist.txt", "serverlist.txt" );
		serverlist.clear_elements();
		// read the list
		FILE *fh = fopen( "serverlist.txt", "r" );
		if (fh) {
			while( !feof(fh) ) {
				char line[1024];
				fgets( line, sizeof(line), fh );
				for(  int i=strlen(line);  i>0  &&  line[i-1]<=32;  i--  ) {
					line[i-1] = 0;
				}
				if(  line[0]>32  ) {
					if(  line[0]!='#'  ) {
						// add new server address
						serverlist.append_element( new gui_scrolled_list_t::var_text_scrollitem_t( line, COL_BLUE ) );
					}
					else {
						// add new comment
						serverlist.append_element( new gui_scrolled_list_t::var_text_scrollitem_t( line+1, COL_WHITE ) );
					}
				}
			}
			fclose( fh );
			remove( "serverlist.txt" );
		}
		else {
			create_win( new news_img("Could not download server list!"), w_time_delete, magic_none);
		}
		serverlist.set_selection( 0 );
	}
	else if(  &join == komp  ) {
		std::string filename = "net:";
		filename += serverlist.get_element(serverlist.get_selection())->get_text();
		destroy_win(this);
		welt->get_message()->clear();
		welt->laden(filename.c_str());
	}
	else if(  &quit == komp  ) {
		welt->beenden( true );
		destroy_win(this);
	}
	return true;
}


void server_frame_t::zeichnen(koord pos, koord gr)
{
	gui_frame_t::zeichnen( pos, gr );

	sint16 pos_y = pos.y+16+14+BUTTON_HEIGHT*2;
	display_ddd_box_clip( pos.x+4, pos_y, 240-8, 0, MN_GREY0, MN_GREY4);

	pos_y += 6+LINESPACE;
	const koord mapsize( gi.get_map()->get_width(), gi.get_map()->get_height() );

	display_ddd_box_clip(pos.x+3, pos_y-1, mapsize.x+2, mapsize.y+2, MN_GREY0,MN_GREY4);
	display_array_wh(pos.x+4, pos_y, mapsize.x, mapsize.y, gi.get_map()->to_array() );

	display_multiline_text( pos.x+4+max(mapsize.x,proportional_string_width(date.get_text_pointer()))+2+10, date.get_pos().y+pos.y+16, buf, COL_BLACK );
	pos_y += 9*LINESPACE - 1;

	display_ddd_box_clip( pos.x+4, pos_y, 240-8, 0, MN_GREY0, MN_GREY4);

	// drawing twice, but otherwise it will not overlay image
	serverlist.zeichnen( pos+koord(0,16) );
}
