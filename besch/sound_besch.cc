/*
 *
 *  grund_besch.cpp
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


#include <stdio.h>

#include "../simdebug.h"

#include "../dataobj/tabfile.h"

#include "../sound/sound.h"

#include "../utils/cstring_t.h"
#include "../utils/simstring.h"

#include "../tpl/vector_tpl.h"
#include "../tpl/stringhashtable_tpl.h"

#include "spezial_obj_tpl.h"
#include "sound_besch.h"

/* sound of the program *
 * @author prissi
 */

class sound_ids {
public:
	sint16 id;
	sound_ids() { id=NO_SOUND; }
	sound_ids(sint16 i) { id=i; }
};

/*
 *  static data
 */
static stringhashtable_tpl<sound_ids> name_sound;
static bool sound_on=false;
static cstring_t sound_path;

sint16 sound_besch_t::compatible_sound_id[MAX_OLD_SOUNDS];

/* init sounds */
/* standard sounds and old sounds are found in the file pak/sound/sound.tab */
void
sound_besch_t::init(const cstring_t & scenario_path)
{
	// ok, now init
	dr_init_sound();
	sound_on = true;
	for( unsigned i=0;  i<MAX_OLD_SOUNDS;  i++ ) {
		compatible_sound_id[i] = NO_SOUND;
	}
	sound_path = cstring_t(scenario_path.chars());
	sound_path = sound_path + "sound/";
	// process sound.tab
	cstring_t filename=scenario_path;
	tabfile_t soundconf;
	if(soundconf.open(filename + "sound/sound.tab")) {
DBG_MESSAGE("sound_besch_t::gib_sound_id()","successfully opened sound/sound.tab"  );
		tabfileobj_t contents;
		soundconf.read(contents);
		// max. 16 old sounds ...
		for( unsigned i=0;  i<MAX_OLD_SOUNDS;  i++ ) {
			char buf[1024];
			sprintf(buf, "sound[%i]", i);
			cstring_t wavename(ltrim(contents.get(buf)));
			if(wavename.len()>0) {
DBG_MESSAGE("sound_besch_t::gib_sound_id()","reading sound %s", wavename.chars()  );
				compatible_sound_id[i] = gib_sound_id( wavename.chars() );
			}
		}
	}
}




/* return sound id from index */
sint16
sound_besch_t::gib_sound_id(const char *name)
{
	if(!sound_on) {
		return NO_SOUND;
	}
	sound_ids s = name_sound.get(name);
	if(s.id==NO_SOUND  &&  *name!=0) {
		// ty to load it ...
		cstring_t fn(sound_path);
		s.id  = dr_load_sample( fn+name );
		if(s.id!=NO_SOUND) {
			name_sound.put(name, s );
DBG_MESSAGE("sound_besch_t::gib_sound_id()","successfully loaded sound %s internal id %i", name, s.id );
		}
	}
	return s.id;
}



/*
 *  member function:
 *      sound_besch_t::register_besch()
 *
 * if there is already such a sound => fail, else success and get an internal sound id
 *
 *  Argumente:
 *      const sound_besch_t *besch
 */
bool
sound_besch_t::register_besch(sound_besch_t *besch)
{
	if(!sound_on) {
		delete besch;
		return false;
	}
	// already there
	if(name_sound.get(besch->gib_name()).id!=NO_SOUND) {
		delete besch;
		return false;
	}
	// ok, new sound ...
	besch->sound_id = dr_load_sample( besch->gib_name() );
	if(besch->sound_id!=NO_SOUND) {
		name_sound.put(besch->gib_name(), sound_ids(besch->sound_id));
		if(besch->nr>=0  &&  besch->nr<=8) {
			compatible_sound_id[besch->nr] = besch->sound_id;
		}
	}
	delete besch;
	return true;
}

/*
 *  member function:
 *      grund_besch_t::alles_geladen()
 *  Return type:
 *      bool (if all needed objects loaded)
 */
bool sound_besch_t::alles_geladen()
{
	DBG_MESSAGE("sound_besch_t::alles_geladen()","sounds");
	return true;	// no mandatory objects here
}
