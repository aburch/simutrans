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
 * Klasse für Schienen in Simutrans.
 * Auf den Schienen koennen Züge fahren.
 * Jede Schiene gehört zu einer Blockstrecke
 *
 * @author Hj. Malthaner
 */
class schiene_t : public weg_t
{
public:
	enum reservation_type : uint8 	{ block = 0, directional, priority };

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
public:
	static const weg_besch_t *default_schiene;

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
			return !reserved.is_bound() || c == reserved || (type == directional && (dir == direction || dir == ribi_t::alle || (is_junction() && (dir & direction)))) || (type == priority && true /*Insert real logic here*/); 
		}
		if(t == directional)
		{
			return !reserved.is_bound() || c == reserved || type == priority || (dir == direction || dir == ribi_t::alle) || (!check_directions_at_junctions && is_junction());
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
	bool is_reserved(reservation_type t = block) const { return reserved.is_bound() && t == type; }

	reservation_type get_reservation_type() const { return type; }

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

	/* called befor deletion;
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
		};
		return (show_reservations  &&  reserved.is_bound()) ? TRANSPARENT75_FLAG | OUTLINE_FLAG | reservation_colour : 0;
	}

	/*
	 * to show reservations if needed
	 */
	virtual image_id get_outline_image() const { return weg_t::get_image(); }
};


template<> inline schiene_t* obj_cast<schiene_t>(obj_t* const d)
{
	return dynamic_cast<schiene_t*>(d);
}

#endif
