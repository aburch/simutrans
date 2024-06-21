/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "records.h"
#include "loadsave.h"
#include "translator.h"
#include "../player/simplay.h"
#include "../simcolor.h"
#include "../simmesg.h"
#include "../simconvoi.h"
#include "../utils/cbuffer.h"
#include "../utils/simstring.h"
#include "../vehicle/vehicle.h"

#include "../macros.h"


void records_t::speed_record_t::rdwr(loadsave_t* f)
{
	f->rdwr_long(speed);
	f->rdwr_long(year_month);
	pos.rdwr(f);
	f->rdwr_byte(player_nr);
	if (f->is_loading()) {
		// for now it is not saved
		cnv = convoihandle_t();
	}
	f->rdwr_str(name, lengthof(name));
}


void records_t::rdwr(loadsave_t* f)
{
	max_rail_speed.rdwr(f);
	max_monorail_speed.rdwr(f);
	max_maglev_speed.rdwr(f);
	max_narrowgauge_speed.rdwr(f);
	max_road_speed.rdwr(f);
	max_ship_speed.rdwr(f);
	max_air_speed.rdwr(f);
}


sint32 records_t::get_record_speed( waytype_t w ) const
{
	switch(w) {
		case road_wt: return max_road_speed.speed;
		case track_wt:
		case tram_wt: return max_rail_speed.speed;
		case monorail_wt: return max_monorail_speed.speed;
		case maglev_wt: return max_maglev_speed.speed;
		case narrowgauge_wt: return max_narrowgauge_speed.speed;
		case water_wt: return max_ship_speed.speed;
		case air_wt: return max_air_speed.speed;
		default: return 0;
	}
}


void records_t::clear_speed_records()
{
	max_road_speed.speed = 0;
	max_rail_speed.speed = 0;
	max_monorail_speed.speed = 0;
	max_maglev_speed.speed = 0;
	max_narrowgauge_speed.speed = 0;
	max_ship_speed.speed = 0;
	max_air_speed.speed = 0;
}


void records_t::notify_record( convoihandle_t cnv, sint32 max_speed, koord3d k, uint32 current_month )
{
	speed_record_t *sr = NULL;
	switch (cnv->front()->get_waytype()) {
		case road_wt: sr = &max_road_speed; break;
		case track_wt:
		case tram_wt: sr = &max_rail_speed; break;
		case monorail_wt: sr = &max_monorail_speed; break;
		case maglev_wt: sr = &max_maglev_speed; break;
		case narrowgauge_wt: sr = &max_narrowgauge_speed; break;
		case water_wt: sr = &max_ship_speed; break;
		case air_wt: sr = &max_air_speed; break;
		default: assert(0);
	}

	// avoid the case of two convois with identical max speed ...
	if(cnv!=sr->cnv  &&  sr->speed+1==max_speed) {
		return;
	}

	// really new/faster?
	if(k!=sr->pos  ||  sr->speed+1<max_speed) {
		// update it first and notify if a repeat was achieved to avoid spamming messages
		sr->cnv = cnv;
		sr->speed = max_speed-1;
		sr->year_month = current_month;
		sr->pos = k;
		sr->player_nr = PLAYER_UNOWNED; // will be set, when accepted
	}
	else {
		// repeted same speed, same place
		sr->cnv = cnv;
		sr->speed = max_speed-1;
		sr->pos = k;

		// same convoi and same position
		if(sr->player_nr==PLAYER_UNOWNED  &&  current_month!=sr->year_month) {
			// notify the world of this new record
			sr->speed = max_speed-1;
			sr->player_nr = cnv->get_owner()->get_player_nr();
			tstrncpy(sr->name, cnv->get_internal_name(), lengthof(sr->name));
			const char* text;
			switch (cnv->front()->get_waytype()) {
				default: NOT_REACHED
				case road_wt:     text = "New world record for motorcars: %.1f km/h by %s."; break;
				case track_wt:
				case tram_wt:     text = "New world record for railways: %.1f km/h by %s.";  break;
				case monorail_wt: text = "New world record for monorails: %.1f km/h by %s."; break;
				case maglev_wt: text = "New world record for maglevs: %.1f km/h by %s."; break;
				case narrowgauge_wt: text = "New world record for narrowgauges: %.1f km/h by %s."; break;
				case water_wt:    text = "New world record for ship: %.1f km/h by %s.";      break;
				case air_wt:      text = "New world record for planes: %.1f km/h by %s.";    break;
			}
			cbuffer_t buf;
			buf.printf( translator::translate(text), speed_to_kmh(10*sr->speed)/10.0, sr->cnv->get_name() );

			msg->add_message( buf, sr->pos, message_t::new_vehicle, PLAYER_FLAG|sr->player_nr );
		}
	}
}
