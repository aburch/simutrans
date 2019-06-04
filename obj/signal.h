/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef obj_signal_h
#define obj_signal_h

#include "roadsign.h"

#include "../simobj.h"

/**
 * Signals for rail tracks.
 *
 * @see blockstrecke_t
 * @see blockmanager
 * @author Hj. Malthaner
 */
class signal_t : public roadsign_t
{
private:
	koord3d signalbox;

	bool no_junctions_to_next_signal;

	// Used for time interval signalling
	sint64 train_last_passed;
protected:

 uint8 textlines_in_signal_window;

public:
	signal_t(loadsave_t *file);
	signal_t(player_t *player, koord3d pos, ribi_t::ribi dir,const roadsign_desc_t *desc, koord3d sb, bool preview = false);
	~signal_t();

	void rdwr_signal(loadsave_t *file);

	void rotate90();

	/**
	* @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @author Hj. Malthaner
	*/
	virtual void info(cbuffer_t & buf, bool dummy = false) const;

#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return obj_t::signal; }
#endif
	const char *get_name() const { return desc->get_name(); }

	uint8 get_textlines() const { return textlines_in_signal_window; }

	/**
	* Calculate actual image
	*/
	void calc_image();

	void set_signalbox(koord3d k) { signalbox = k; }
	koord3d get_signalbox() const { return signalbox; }

	bool get_no_junctions_to_next_signal() const { return no_junctions_to_next_signal; }
	void set_no_junctions_to_next_signal(bool value) { no_junctions_to_next_signal = value; } 

	bool is_bidirectional() const { return ((dir & ribi_t::east) && (dir & ribi_t::west)) || ((dir & ribi_t::south) && (dir & ribi_t::north)) || ((dir & ribi_t::northeast) && (dir & ribi_t::southwest)) || ((dir & ribi_t::northwest) && (dir & ribi_t::southeast)); }

	void set_train_last_passed(sint64 value) { train_last_passed = value; }
	sint64 get_train_last_passed() const { return train_last_passed; }

	void show_info();


};

#endif
