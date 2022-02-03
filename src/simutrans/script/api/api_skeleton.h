/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/** @file api_skeleton.h Documents the global variables in a scenario script */

#ifdef SQAPI_DOC // document global structs

struct {
	/// Short description of scenario, is shown for example in finance window.
	string short_description;
	/// Author(s) of the scenario and/or scripts.
	string author;
	/// Version of script.
	string version;
	/**
	 * Script requires this version of api.
	 * Corresponds to release numbers (e.g. 112.3).
	 * Set it to "*" to support nightly versions.
	 * If this string is not set, the default "112.3" will be used.
	 *
	 * @see get_api_version
	 */
	/// Required version of api.
	string api;
}
/**
 * Meta information about the scenario.
 */
scenario;

struct {
	/**
	 * Name of savegame. The scenario starts with the world saved there.
	 * If file == "<attach>" then do not load a saved, attach
	 * the scenario to the running world instead.
	 *
	 * @see get_map_file
	 */
	string file;
}
/**
 * Information about game map/world.
 */
map;

/**
 * Only this table is saved and reloaded with an savegame.
 * Only plain data is saved: no classes / functions, no cyclic references.
 *
 * Instances of classes can be saved if
 * (1) the class implements the function _save that
 * (2) returns something which can reconstruct the instance, i.e. an constructor call, embedded in a string.
 * @code
 *
 * class my_class_with_save {
 *     foo = 0
 *
 *     constructor(f)
 *     {
 *         foo = f;
 *     }
 *     function _save()
 *     {
 *         return "my_class_with_save("+ foo + ")"
 *     }
 * }
 * @endcode
 *
 * @brief
 * Persistent data should go into this table.
 */
table persistent;

#endif
