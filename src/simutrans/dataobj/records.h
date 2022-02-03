/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_RECORDS_H
#define DATAOBJ_RECORDS_H


#include "koord.h"
#include "../convoihandle.h"

class message_t;
class player_t;

/**
 * World record speed management.
 * Keeps track of the fastest vehicles in game.
 */
class records_t {

public:
	records_t(message_t *_msg) : msg(_msg) {}
	~records_t() {}

	/** Returns the current speed record for the given way type. */
	sint32 get_record_speed( waytype_t w ) const;

	/** Posts a message that a new speed record has been set. */
	void notify_record( convoihandle_t cnv, sint32 max_speed, koord k, uint32 current_month );

	/** Resets all speed records. */
	void clear_speed_records();

private:
	// Destination for world record notifications.
	message_t *msg;

	/**
	 * Class representing a word speed record.
	 */
	class speed_record_t {
	public:
		convoihandle_t cnv;
		sint32    speed;
		koord     pos;
		player_t *owner;  // Owner
		uint32    year_month;

		speed_record_t() : cnv(), speed(0), pos(koord::invalid), owner(NULL), year_month(0) {}
	};

	/// World rail speed record
	speed_record_t max_rail_speed;
	/// World monorail speed record
	speed_record_t max_monorail_speed;
	/// World maglev speed record
	speed_record_t max_maglev_speed;
	/// World narrowgauge speed record
	speed_record_t max_narrowgauge_speed;
	/// World road speed record
	speed_record_t max_road_speed;
	/// World ship speed record
	speed_record_t max_ship_speed;
	/// World air speed record
	speed_record_t max_air_speed;
};

#endif
