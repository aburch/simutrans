/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_SIGNAL_H
#define OBJ_SIGNAL_H


#include "roadsign.h"

#include "simobj.h"

#include "way/schiene.h"

/**
 * Signals for rail tracks.
 *
 * @see blockstrecke_t
 * @see blockmanager
 */
class signal_t : public roadsign_t
{
public:
	signal_t(loadsave_t *file);
	signal_t(player_t *player, koord3d pos, ribi_t::ribi dir,const roadsign_desc_t *desc, bool preview = false) : roadsign_t(player,pos,dir,desc,preview) { state = STATE_RED;}

	const roadsign_desc_t* get_desc() const { return desc; }

	/// @copydoc obj_t::info
	void info(cbuffer_t & buf) const OVERRIDE;

	typ get_typ() const OVERRIDE { return obj_t::signal; }
	const char *get_name() const OVERRIDE {return "Signal";}

	void show_info() OVERRIDE;

	void calc_image() OVERRIDE;

	image_id get_image() const OVERRIDE {
		if (foreground_image == IMG_EMPTY && schiene_t::show_reservations) {
			return IMG_EMPTY;
		}
		return image;
	}

	/**
	* For the front image hiding vehicles (show also for reservation)
	*/
	image_id get_front_image() const OVERRIDE {
		if (foreground_image == IMG_EMPTY && schiene_t::show_reservations) {
			return image;
		}
		return foreground_image;
	}
};

#endif
