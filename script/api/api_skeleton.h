/** @file api_skeleton.h Documents the global variables in a scenario script */

#ifdef SQAPI_DOC // document global structs

struct {
	/// Short description of scenario, is shown for example in finance window.
	string short_description;
	/// Author(s) of the scenario and/or scripts.
	string author;
	/// Version of script.
	string version;
}
/// Meta information about the scenario.
scenario;

struct {
	/// Name of savegame. The scenario starts with the world saved there.
	string file;
}
/// Information about game map/world.
map;

/// @brief
/// Persistent data should go into this table.
/// @details
/// Only this table is saved and reloaded with an savegame.
/// Only plain data is saved: no classes / instances / functions, no cyclic references
table persistent;

#endif
