/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_GAMEINFO_H
#define DATAOBJ_GAMEINFO_H


#include <string>
#include "../simtypes.h"
#include "../simconst.h"
#include "../simcolor.h"
#include "../tpl/array2d_tpl.h"
#include "../network/checksum.h"

class karte_t;
class loadsave_t;

/**
 * Info about the current game
 */
class gameinfo_t
{
private:
	sint32 size_x, size_y;
	/**
	 * Two pixel arrays: one with indexed colors (sent by servers),
	 * one with rgb colors (will be displayed)
	 */
	array2d_tpl<PIXVAL> map_idx, map_rgb;

	sint32 industries;
	sint32 tourist_attractions;
	sint32 city_count;
	sint32 citizen_count;

	uint16 convoi_count;
	uint16 halt_count;

	sint64 total_pass_transported;
	sint64 total_mail_transported;
	sint64 total_goods_transported;

	bool freeplay;
	bool use_timeline;
	sint64 current_starting_money;

	uint32 current_year_month;
	sint16 bits_per_month;

	// names of the stations ...
	char language_code_names[4];

	std::string game_comment;
	std::string file_name;
	std::string pak_name;

	uint32 game_engine_revision;
	checksum_t pakset_checksum;

	// 0 = empty, otherwise some value from simplay
	uint8 player_type[MAX_PLAYER_COUNT];
	uint8 clients; // currently connected players

public:
	gameinfo_t( karte_t *welt );
	gameinfo_t( loadsave_t *file );

	void rdwr( loadsave_t *file );

	sint32 get_size_x() const {return size_x;}
	sint32 get_size_y() const {return size_y;}
	const array2d_tpl<PIXVAL> *get_map() const { return &map_rgb; }

	sint32 get_industries() const {return industries;}
	sint32 get_tourist_attractions() const {return tourist_attractions;}
	sint32 get_city_count() const {return city_count;}
	sint32 get_citizen_count() const {return citizen_count;}

	sint32 get_convoi_count() const {return convoi_count;}
	sint32 get_halt_count() const {return halt_count;}

	uint8 get_use_timeline() const {return use_timeline;}
	sint16 get_current_year() const {return current_year_month/12;}
	sint16 get_current_month() const {return (current_year_month%12);}
	sint16 get_bits_per_month() const {return bits_per_month;}

	bool is_freeplay() const { return freeplay; }
	sint64 get_current_starting_money() const {return current_starting_money;}

	uint32 get_game_engine_revision() const { return game_engine_revision; }
	const char *get_name_language_iso() const { return language_code_names; }
	const char *get_pak_name() const { return pak_name.c_str(); }
	uint8 get_player_type(uint8 i) const { return player_type[i]; }
	uint8 get_clients() const { return clients; }
	const checksum_t & get_pakset_checksum() const { return pakset_checksum; }
};

#endif
