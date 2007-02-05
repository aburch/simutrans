/*
 *
 *  grund_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#ifndef __sound_besch_t
#define __sound_besch_t

#include "obj_besch_std_name.h"
#include "../simtypes.h"

#define NO_SOUND (0xFFFFu)
#define LOAD_SOUND (0xFFFEu)

/*
 *  Autor:
 *      prissi
 *
 *  Beschreibung:
 *      Sounds in the game; name is the file name
 *      ingame, sounds are referred to by their number
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 */

#define SFX_CASH sound_besch_t::get_compatible_sound_id(15)
#define SFX_REMOVER sound_besch_t::get_compatible_sound_id(14)
#define SFX_DOCK sound_besch_t::get_compatible_sound_id(13)
#define SFX_GAVEL sound_besch_t::get_compatible_sound_id(12)
#define SFX_JACKHAMMER sound_besch_t::get_compatible_sound_id(11)
#define SFX_FAILURE sound_besch_t::get_compatible_sound_id(10)
#define SFX_SELECT sound_besch_t::get_compatible_sound_id(9)

class cstring_t;

#define MAX_OLD_SOUNDS (16)


class sound_besch_t : public obj_besch_std_name_t {
    friend class sound_writer_t;
    friend class sound_reader_t;

private:
	static sint16 compatible_sound_id[MAX_OLD_SOUNDS];

	sint16 sound_id;
	sint16 nr;	// for old sounds/system sounds etc.

public:
	static sint16 gib_sound_id(const char *name);

	static bool register_besch(sound_besch_t *besch);

	static bool alles_geladen();

	static void init(const cstring_t & scenario_path);

	/* return old sound id from index */
	static sint16 get_compatible_sound_id(const sint8 nr) { return compatible_sound_id[nr&(15)]; }
};


#endif
