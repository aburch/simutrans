/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <stdio.h>
#include "../simdebug.h"

#include "../dataobj/tabfile.h"
#include "../dataobj/environment.h"

#include "../macros.h"
#include "../sound/sound.h"

#include "../utils/simstring.h"

#include "../tpl/stringhashtable_tpl.h"

#include "spezial_obj_tpl.h"
#include "sound_desc.h"
#include "ground_desc.h"

/*
 * sound of the program
 */
class sound_ids {
public:
	std::string filename;
	sint16 id;
	sound_ids() { id=NO_SOUND; }
	sound_ids(sint16 i) { id=i;  }
	sound_ids(sint16 i, const char* fn) : filename(fn), id(i) {}
};


static stringhashtable_tpl<sound_ids *> name_sound;
static bool sound_on=false;
static std::string sound_path;

sint16 sound_desc_t::compatible_sound_id[MAX_OLD_SOUNDS]=
{
	NO_SOUND, NO_SOUND, NO_SOUND, NO_SOUND,
	NO_SOUND, NO_SOUND, NO_SOUND, NO_SOUND,
	NO_SOUND, NO_SOUND, NO_SOUND, NO_SOUND,
	NO_SOUND, NO_SOUND, NO_SOUND, NO_SOUND
};

// sound with the names of climates and "beach" and "forest" are reserved for ambient noises
sint16 sound_desc_t::beach_sound = NO_SOUND;
sint16 sound_desc_t::forest_sound = NO_SOUND;
sint16 sound_desc_t::climate_sounds[MAX_CLIMATES]=
{
	NO_SOUND, NO_SOUND, NO_SOUND, NO_SOUND,
	NO_SOUND, NO_SOUND, NO_SOUND, NO_SOUND
};


/* init sounds */
/* standard sounds and old sounds are found in the file pak/sound/sound.tab */
void sound_desc_t::init()
{
	// ok, now init
	sound_on = true;
	sound_path = env_t::base_dir;
	sound_path= sound_path + env_t::pak_name + "sound/";
	// process sound.tab
	tabfile_t soundconf;
	if (soundconf.open((sound_path + "sound.tab").c_str())) {
DBG_MESSAGE("sound_desc_t::init()","successfully opened sound/sound.tab"  );
		tabfileobj_t contents;
		soundconf.read(contents);
		// max. 16 old sounds ...
		for( unsigned i=0;  i<MAX_OLD_SOUNDS;  i++ ) {
			char buf[1024];
			sprintf(buf, "sound[%i]", i);
			const char *fn=ltrim(contents.get(buf));
			if(fn[0]>0) {
DBG_MESSAGE("sound_desc_t::init()","reading sound %s", fn  );
				compatible_sound_id[i] = get_sound_id( fn );
DBG_MESSAGE("sound_desc_t::init()","assigned system sound %d to sound %s (id=%i)", i, (const char *)fn, compatible_sound_id[i] );
			}
		}
		// now assign special sounds for climates, beaches and forest
		beach_sound = get_sound_id( "beaches.wav" );
		forest_sound = get_sound_id( "forest.wav" );
		for(  int i=0;  i<MAX_CLIMATES;  i++  ) {
			char name[64];
			sprintf( name, "%s.wav", ground_desc_t::get_climate_name_from_bit((climate)i) );
			climate_sounds[i] = get_sound_id( name );
		}
	}
}




/* return sound id from index */
sint16 sound_desc_t::get_sound_id(const char *name)
{
	if(  !sound_on  ||  name==NULL  ||  *name==0  ) {
		return NO_SOUND;
	}

	sound_ids *s = name_sound.get(name);
	if(s!=NULL  &&  s->id!=NO_SOUND) {
		DBG_MESSAGE("sound_desc_t::get_sound_id()", "Successfully retrieved sound \"%s\" with internal id %hi", s->filename.c_str(), s->id );
		return s->id;
	}

	// not loaded: try to load it
	const sint16 sample_id = dr_load_sample((sound_path + name).c_str());

	if(sample_id==NO_SOUND) {
		dbg->warning("sound_desc_t::get_sound_id()", "Sound \"%s\" not found", name );
		return NO_SOUND;
	}

	s = new sound_ids(sample_id, name);
	name_sound.put(s->filename.c_str(), s );
DBG_MESSAGE("sound_desc_t::get_sound_id()", "Successfully loaded sound \"%s\" with internal id %hi", s->filename.c_str(), s->id );
	return s->id;
}



/*
 * if there is already such a sound => fail, else success and get an internal sound id
 * do not store desc as it will be deleted anyway
 */
bool sound_desc_t::register_desc(sound_desc_t *desc)
{
	if(  !sound_on  ) {
		return false;
	}

	// register, if not there (all done by this one here)
	desc->sound_id = get_sound_id( desc->get_name() );
	if(desc->sound_id!=NO_SOUND) {
		if(desc->nr>=0  &&  desc->nr<=8) {
			compatible_sound_id[desc->nr] = desc->sound_id;
DBG_MESSAGE("sound_desc_t::register_desc", "Successfully registered sound %s internal id %i as compatible sound %i", desc->get_name(), desc->sound_id, desc->nr );
			return true;
		}
	}

	dbg->warning("sound_desc_t::register_desc", "Failed to register sound %s internal id %i", desc->get_name(), desc->sound_id);
	return false;
}
