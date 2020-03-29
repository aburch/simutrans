/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef PLAYER_AI_SCRIPTED_H
#define PLAYER_AI_SCRIPTED_H


#include "ai.h"
#include "../utils/plainstring.h"

class script_vm_t;

class ai_scripted_t : public ai_t
{
	/// name of ai, files are searched in ai/ai_name/...
	/// e.g. my_ai
	plainstring ai_name;

	/// path to ai directory (relative to env_t::user_dir)
	/// e.g. ai/my_ai/
	plainstring ai_path;

	/**
	 * loads ai file with the given name
	 * @param filename name ai script file (including .nut extension)
	 */
	bool load_script(const char* filename);

	/// pointer to virtual machine
	script_vm_t *script;

public:
	ai_scripted_t(uint8 nr);

	~ai_scripted_t();

	/**
	 * Initializes scripted ai
	 */
	const char* init( const char *ai_base, const char *ai_name);

	/**
	 * Reloads script
	 * @returns error message
	 */
	const char* reload_script();

	bool has_script() const { return script; }

	uint8 get_ai_id() const OVERRIDE { return AI_SCRIPTED; }

	void step() OVERRIDE;

	/**
	 * Called monthly by simworld.cc during simulation
	 * @returns false if player has to be removed (bankrupt/inactive)
	 */
	bool new_month() OVERRIDE;

	/**
	 * Called yearly by simworld.cc during simulation
	 */
	void new_year() OVERRIDE;

	/**
	 * Stores/loads the player state
	 * @param file where the data will be saved/loaded
	 */
	void rdwr(loadsave_t *file) OVERRIDE;

	/**
	 * Called after game is fully loaded;
	 */
	void finish_rd() OVERRIDE;
};


#endif
