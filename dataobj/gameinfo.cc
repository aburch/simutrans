/*
 * Info about the current game
 *
 * prissi
 *
 * 8/2010
 */

#include "gameinfo.h"
#include "network.h"
#include "network_socket_list.h"
#include "einstellungen.h"
#include "translator.h"
#include "umgebung.h"
#include "../simtools.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "../simcity.h"
#include "../simhalt.h"
#include "../besch/grund_besch.h"
#include "../player/simplay.h"
#include "../gui/karte.h"
#include "../utils/simstring.h"
#include "loadsave.h"
#include "pakset_info.h"
#include "../simversion.h"


#define MINIMAP_SIZE (64)


gameinfo_t::gameinfo_t(karte_t *welt) :
	map(MINIMAP_SIZE,MINIMAP_SIZE),
	game_comment(""),
	file_name(""),
	pak_name("")
{
	groesse_x = welt->get_size().x;
	groesse_y = welt->get_size().y;

	// create a minimap


	industries = welt->get_fab_list().get_count();
	tourist_attractions = welt->get_ausflugsziele().get_count();
	anzahl_staedte = welt->get_staedte().get_count();
	einwohnerzahl = 0;
	FOR(weighted_vector_tpl<stadt_t*>, const i, welt->get_staedte()) {
		einwohnerzahl += i->get_einwohner();
	}

	const int gr_x = welt->get_size().x;
	const int gr_y = welt->get_size().y;
	for( uint16 i = 0; i < MINIMAP_SIZE; i++ ) {
		for( uint16 j = 0; j < MINIMAP_SIZE; j++ ) {
			const koord pos(i * gr_x / MINIMAP_SIZE, j * gr_y / MINIMAP_SIZE);
			const grund_t* gr = welt->lookup_kartenboden(pos);
			map.at(i,j) = reliefkarte_t::calc_relief_farbe(gr);
		}
	}

	total_pass_transported = welt->get_finance_history_month(1,karte_t::WORLD_PAS_RATIO);
	total_mail_transported = welt->get_finance_history_month(1,karte_t::WORLD_MAIL_RATIO);
	total_goods_transported = welt->get_finance_history_month(1,karte_t::WORLD_GOODS_RATIO);
	convoi_count = welt->convoys().get_count();

	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++ ) {
		spieler_type[i] = spieler_t::EMPTY;
		if(  spieler_t *sp = welt->get_spieler(i)  ) {
			spieler_type[i] = sp->get_ai_id();
			if(  !sp->access_password_hash().empty()  ) {
				spieler_type[i] |= spieler_t::PASSWORD_PROTECTED;
			}
		}
	}
	clients = socket_list_t::get_playing_clients();

	halt_count = haltestelle_t::get_alle_haltestellen().get_count();

	settings_t const& s = welt->get_settings();
	freeplay = s.is_freeplay();
	use_timeline = welt->get_timeline_year_month()!=0;
	current_starting_money = s.get_starting_money(welt->get_last_year());

	current_year_month = welt->get_current_month();
	bits_per_month = s.get_bits_per_month();

	// names of the stations ...
	memcpy(language_code_names, translator::get_langs()[s.get_name_language_id()].iso, lengthof(language_code_names));

	// will contain server-IP/name for network games
	file_name = s.get_filename();

	// comment currently not used
	char const* const copyright = grund_besch_t::ausserhalb->get_copyright();
	if (copyright && STRICMP("none", copyright) != 0) {
		// construct from outside object copyright string
		pak_name = copyright;
	}
	else {
		// construct from pak name
		pak_name = umgebung_t::objfilename;
		pak_name.erase( pak_name.length()-1 );
	}

#ifdef REVISION
	game_engine_revision = atol( QUOTEME(REVISION) );
#else
	game_engine_revision = 0;
#endif
	pakset_checksum = *(pakset_info_t::get_checksum());
}


gameinfo_t::gameinfo_t(loadsave_t *file) :
	map(MINIMAP_SIZE,MINIMAP_SIZE),
	game_comment(""),
	file_name(""),
	pak_name("")
{
	rdwr( file );
}


void gameinfo_t::rdwr(loadsave_t *file)
{
	xml_tag_t e( file, "gameinfo_t" );

	file->rdwr_long( groesse_x );
	file->rdwr_long( groesse_y );
	for( int y=0;  y<MINIMAP_SIZE;  y++  ) {
		for( int x=0;  x<MINIMAP_SIZE;  x++  ) {
			file->rdwr_byte( map.at(x,y) );
		}
	}

	file->rdwr_long( industries );
	file->rdwr_long( tourist_attractions );
	file->rdwr_long( anzahl_staedte );
	file->rdwr_long( einwohnerzahl );

	file->rdwr_short( convoi_count );
	file->rdwr_short( halt_count );

	file->rdwr_longlong( total_pass_transported );
	file->rdwr_longlong( total_mail_transported );
	file->rdwr_longlong( total_goods_transported );

	file->rdwr_longlong( total_goods_transported );

	file->rdwr_bool( freeplay );
	file->rdwr_bool( use_timeline );
	file->rdwr_longlong( current_starting_money );

	file->rdwr_long( current_year_month );
	file->rdwr_short( bits_per_month );

	file->rdwr_str(language_code_names, lengthof(language_code_names) );

	char temp[1024];
	tstrncpy( temp, game_comment.c_str(), lengthof(temp) );
	file->rdwr_str( temp, lengthof(temp) );	// game_comment
	if(  file->is_loading()  ) {
		game_comment = temp;
	}
	tstrncpy( temp, file_name.c_str(), lengthof(temp) );
	file->rdwr_str( temp, lengthof(temp) ); // file_name
	if(  file->is_loading()  ) {
		file_name = temp;
	}
	tstrncpy( temp, pak_name.c_str(), lengthof(temp) );
	file->rdwr_str( temp, lengthof(temp) ); // pak_name
	if(  file->is_loading()  ) {
		pak_name = temp;
	}
	file->rdwr_long( game_engine_revision );

	for(  int i=0;  i<16;  i++  ) {
		file->rdwr_byte( spieler_type[i] );
	}
	file->rdwr_byte( clients );

	pakset_checksum.rdwr(file);
}
