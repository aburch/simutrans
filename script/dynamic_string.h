/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_DYNAMIC_STRING_H
#define SCRIPT_DYNAMIC_STRING_H


/** @file dynamic_string.h strings that can be asynchronously changed by scripts */

#include "../utils/plainstring.h"

class script_vm_t;
class player_t;
class loadsave_t;

/**
 * Class of strings to hold result of text-returning script functions.
 *
 * Can communicate with server to update scenario text on clients, which
 * do not know the script. Used in scenario_t and scenario_info_t.
 *
 * Results will be cached, frequent calls to update() will only update
 * the text if some time threshold is exceeded.
 */
class dynamic_string
{
private:
	/// holds result of script function
	plainstring str;

	/// true if text changed since last call to clear_changed()
	bool changed;

	/// script method to be called to retrieve text
	const char* method;
public:
	dynamic_string(const char *method_) : str(NULL), changed(false), method(method_) { }
	~dynamic_string();

	/**
	 * Update the string, calls fetch_result().
	 * @param script the scripting engine, if script == NULL then this needs server communication
	 * @param force_update will force to update even if cache is still valid
	 */
	void update(script_vm_t *script, player_t *player, bool force_update=false);

	operator const plainstring& () const { return str; }
	operator const char* () const { return (const char*)str; }

	bool has_changed() const { return changed; }
	void clear_changed() { changed = false; }

	/**
	 * At client: fetch result from server.
	 * At server: call script to get immediate result.
	 *
	 * Results will be stored into a hashtable, fetch requests will be send only if
	 * some time threshold is exceeded (or force_update is true).
	 *
	 * @param function script function incl arguments to be called
	 * @param listener will be updated should the result change in future
	 * @param force_update force an update, do not use cached string
	 * @returns string or NULL if nothing available (yet)
	 */
	static const char* fetch_result(const char* function, script_vm_t *script, dynamic_string *listener=NULL, bool force_update=false);

	/**
	 * Clear internal cache. Registers callback method.
	 */
	static void init(script_vm_t *script);

	/**
	 * Cache result of script at server,
	 * immediately update the listening dynamic_string.
	 * @returns dummy value
	 */
	static bool record_result(const char* function, plainstring result);

	static void rdwr_cache(loadsave_t *file);

private:
	/**
	 * Calls a script
	 * @param function is the full function call including integer parameters
	 * @returns true if call was successfull
	 */
	static bool call_script(const char* function, script_vm_t* script, plainstring& result);
};

#endif
