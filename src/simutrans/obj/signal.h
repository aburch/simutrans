/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_SIGNAL_H
#define OBJ_SIGNAL_H


#include "roadsign.h"

#include "simobj.h"


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

	/// @copydoc obj_t::info
	void info(cbuffer_t & buf) const OVERRIDE;

	typ get_typ() const OVERRIDE { return obj_t::signal; }
	const char *get_name() const OVERRIDE {return "Signal";}

	void show_info() OVERRIDE;

	void calc_image() OVERRIDE;
};

#endif
