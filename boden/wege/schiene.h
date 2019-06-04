/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef boden_wege_schiene_h
#define boden_wege_schiene_h


#include "weg.h"
#include "../../convoihandle_t.h"

class vehicle_t;

/**
 * Class for Rails in Simutrans.
 * Trains can run over rails.
 * Every rail belongs to a section block
 *
 * @author Hj. Malthaner
 */
class schiene_t : public weg_t
{
public:
	enum reservation_type : uint8 	{ block = 0, directional, priority, stale_block, end };

protected:
	/**
	* Bound when this block was successfully reserved by the convoi
	* @author prissi
	*/
	convoihandle_t reserved;

	// The type of reservation
	reservation_type type;

	// Additional data for reservations, such as the priority level or direction.
	ribi_t::ribi direction; 

	schiene_t(waytype_t waytype);

	uint8 textlines_in_info_window;
	
	bool is_type_rail_type(waytype_t wt) { return wt == track_wt || wt == monorail_wt || wt == maglev_wt || wt == tram_wt || wt == narrowgauge_wt; }

public:
	static const way_desc_t *default_schiene;

	static bool show_reservations;

	/**
	* File loading constructor.
	* @author Hj. Malthaner
	*/
	schiene_t(loadsave_t *file);

	schiene_t();

	//virtual waytype_t get_waytype() const {return track_wt;}

	/**
	* @return additional info is reservation!
	* @author prissi
	*/
	void info(cbuffer_t & buf, bool is_bridge = false) const;

	/**
	* true, if this rail can be reserved
	* @author prissi
	*/
	bool can_reserve(convoihandle_t c, ribi_t::ribi dir, reservation_type t = block, bool check_directions_at_junctions = false) const 
	{ 
		if(t == block)
		{
			return !reserved.is_bound() || c == reserved || (type == directional && (dir == direction || dir == ribi_t::all || ((is_diagonal() || ribi_t::is_bend(direction)) && (dir & direction)) || (is_junction() && (dir & direction)))) || (type == priority && true /*Insert real logic here*/); 
		}
		if(t == directional)
		{
			return !reserved.is_bound() || c == reserved || type == priority || (dir == direction || dir == ribi_t::all) || (!check_directions_at_junctions && is_junction());
		}
		if(t == priority)
		{
			return !reserved.is_bound() || c == reserved; // TODO: Obtain the priority data from the convoy here and comapre it.
		}

		// Fail with non-standard reservation type
		return false;
	}

	/**
	* true, if this rail can be reserved
	* @author prissi
	*/
	bool is_reserved(reservation_type t = block) const { return reserved.is_bound() && (t == type || (t == block && type == stale_block) || type >= end); }
	bool is_reserved_directional(reservation_type t = directional) const { return reserved.is_bound() && t == type; }
	bool is_reserved_priority(reservation_type t = priority) const { return reserved.is_bound() && t == type; }

	void set_stale() { type == block ? type = stale_block : type == block /* null code for ternery */ ; }
	bool is_stale() { return type == stale_block; }

	reservation_type get_reservation_type() const { return type != stale_block ? type : block; }

	ribi_t::ribi get_reserved_direction() const { return direction; }

	/**
	* true, then this rail was reserved
	* @author prissi
	*/
	bool reserve(convoihandle_t c, ribi_t::ribi dir, reservation_type t = block, bool check_directions_at_junctions = false);

	/**
	* releases previous reservation
	* @author prissi
	*/
	bool unreserve(convoihandle_t c);

	/**
	* releases previous reservation
	* @author prissi
	*/
	bool unreserve(vehicle_t *);

	/* called before deletion;
	 * last chance to unreserve tiles ...
	 */
	virtual void cleanup(player_t *player);

	/**
	* gets the related convoi
	* @author prissi
	*/
	convoihandle_t get_reserved_convoi() const { return reserved; }

	void rdwr(loadsave_t *file);

	void rotate90();

	void show_info();

	/**
	 * if a function return here a value with TRANSPARENT_FLAGS set
	 * then a transparent outline with the color form the lower 8 Bit is drawn
	 * @author kierongreen
	 */
	virtual PLAYER_COLOR_VAL get_outline_colour() const 
	{ 
		uint8 reservation_colour;
		switch(type)
		{
		case block:
		default:
			reservation_colour = COL_RED;
			break;

		case directional:
			reservation_colour = COL_BLUE;
			break;
			
		case priority:
			reservation_colour = COL_YELLOW;
			break;
#ifdef DEBUG
		case stale_block:
			reservation_colour = COL_DARK_RED;
			break;
#endif
		};
		return (show_reservations  &&  reserved.is_bound()) ? TRANSPARENT75_FLAG | OUTLINE_FLAG | reservation_colour : 0;
	}

	/*
	 * to show reservations if needed
	 */
	virtual image_id get_outline_image() const { return weg_t::get_image(); }

	uint8 get_textlines() const { return textlines_in_info_window; }

	static const char* get_working_method_name(working_method_t wm)
	{
		switch (wm)
		{
		case drive_by_sight:
			return "drive_by_sight";
		case time_interval:
			return "time_interval";
		case absolute_block:
			return "absolute_block";
		case token_block:
			return "token_block";
		case track_circuit_block:
			return "track_circuit_block";
		case cab_signalling:
			return "cab_signalling";
		case moving_block:
			return "moving_block";
		case one_train_staff:
			return "one_train_staff";
		case time_interval_with_telegraph:
			return "time_interval_with_telegraph";
		default:
			return "unknown";
		};
	}

	static const char* get_reservation_type_name(reservation_type rt)
	{
		switch (rt)
		{
		case 0:
		default: 
			return "block_reservation";
		case 1:
			return "directional_reservation";
		case 2:
			return "priority_reservation";
		};
	}
	static const char* get_directions_name(ribi_t::ribi dir)
	{
		switch (dir)
		{
		case 1:
			return "north";
		case 2:
			return "east";
		case 3:
			return "north_east";
		case 4:
			return "south";
		case 5:
			return "north_south";
		case 6:
			return "south_east";
		case 8:
			return "west";
		case 9:
			return "north_west";
		case 10:
			return "east_west";
		case 12:
			return "south_west";
		default:
			return "unknown";
		};
	}
};


template<> inline schiene_t* obj_cast<schiene_t>(obj_t* const d)
{
	return dynamic_cast<schiene_t*>(d);
}

#endif
