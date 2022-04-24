/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_PAKSET_MANAGER_H
#define DATAOBJ_PAKSET_MANAGER_H


#include "../descriptor/objversion.h"
#include "../tpl/inthashtable_tpl.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/ptrhashtable_tpl.h"


class obj_desc_t;
class obj_reader_t;


/// Missing things during loading:
/// factories, vehicles, roadsigns or catenary may be severe
enum missing_level_t
{
	NOT_MISSING     = 0,
	MISSING_FACTORY = 1,
	MISSING_VEHICLE = 2,
	MISSING_SIGN    = 3,
	MISSING_WAYOBJ  = 4,
	MISSING_ERROR   = 4,
	MISSING_BRIDGE,
	MISSING_BUILDING,
	MISSING_WAY
};


class pakset_manager_t
{
	/// table of registered obj readers sorted by id
	typedef inthashtable_tpl<obj_type, obj_reader_t *> obj_map_t;
	static obj_map_t *registered_readers;

	//
	// object addresses needed for resolving xrefs later
	// - stored in a hash table with type and name
	//
	static inthashtable_tpl<obj_type, stringhashtable_tpl<obj_desc_t *> > loaded;
	typedef inthashtable_tpl<obj_type, stringhashtable_tpl<slist_tpl<obj_desc_t**> > > unresolved_map_t;
	static unresolved_map_t unresolved;
	static ptrhashtable_tpl<obj_desc_t **, int> fatals;

	/// Read a descriptor node.
	/// @param fp File to read from
	/// @param[out] data If reading is successful, contains descriptor for the object, else NULL.
	/// @param register_nodes Nesting level for desc-nodes, should normally be 0
	/// @param version File format version
	static bool read_nodes(FILE *fp, obj_desc_t *&data, int register_nodes, uint32 version);
	static bool skip_nodes(FILE *fp, uint32 version);

	static std::string doublettes;
	static stringhashtable_tpl<missing_level_t> missing_pak_names;

public:
	static void register_reader(obj_reader_t *reader);

	/// Loads pakset data from env_t::pak_dir, and also from env_t::user_dir/addons/env_t::pak_name if @p load_addons is true
	static void load_pakset(bool load_addons);

	/// Only for single files, must take care of all the cleanup/registering matrix themselves
	static bool load_pak_file(const std::string &filename);

	/// special error handling for double objects
	static void doubled(const char *what, const char *name);

	static void clear_missing_paks();

	/// For warning, when stuff had to be removed/replaced
	/// level must be >=1 (1: factory, 2: vehicles, >=4: not so important)
	/// may be refined later
	static void add_missing_paks(const char *name, missing_level_t critical_level);

	// Display and log a warning if there are missing paks after loading a saved game
	static void warn_if_paks_missing();

public:
	static void xref_to_resolve(obj_type type, const char *name, obj_desc_t **dest, bool fatal);
	static void obj_for_xref(obj_type type, const char *name, obj_desc_t *data);

private:
	/**
	 * Loads all pak files from a directory, displaying a progress bar if the display is initialized
	 * @param path Directory to be scanned for PAK files
	 * @param message Label to show over the progress bar
	 */
	static bool load_paks_from_directory(const std::string &path, const char *message);

	static bool finish_loading();

	static bool had_overlaid() { return !doublettes.empty(); }
	static void clear_overlaid() { doublettes.clear(); }
	static std::string get_overlaid() { return doublettes; }

	static void resolve_xrefs();
};


#endif
