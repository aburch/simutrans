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

/*
 *  includes
 */
#include "text_besch.h"
#include "../simtypes.h"

#define NO_SOUND (-1)
#define LOAD_SOUND (-2)

/*
 *  class:
 *      sound_besch_t()
 *
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

template <class T> class vector_tpl;
template <class T> class stringhashtable_tpl;
class cstring_t;

#define MAX_OLD_SOUNDS (16)


class sound_besch_t : public obj_besch_t {
    friend class sound_writer_t;
    friend class sound_reader_t;

private:
	static sint16 compatible_sound_id[MAX_OLD_SOUNDS];

	sint16 sound_id;
	sint16 nr;	// for old sounds/system sounds etc.

public:
	const char *gib_name() const {
		return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
	}
	const char *gib_copyright() const {
		return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
	}

	static sint16 gib_sound_id(const char *name);

	static bool register_besch(sound_besch_t *besch);

	static bool alles_geladen();

	static void init(const cstring_t & scenario_path);

	/* return old sound id from index */
	static sint16 get_compatible_sound_id(const sint8 nr) { return compatible_sound_id[nr&(15)]; }
};


#endif // __sound_besch_t
