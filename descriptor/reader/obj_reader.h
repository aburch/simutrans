/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_OBJ_READER_H
#define DESCRIPTOR_READER_OBJ_READER_H


#include <stdio.h>

#include "../obj_node_info.h"
#include "../objversion.h"
#include "../../simdebug.h"
#include "../../simtypes.h"


class obj_desc_t;
template<class key_t, class value_t> class inthashtable_tpl;
template<class value_t> class stringhashtable_tpl;
template<class key_t, class value_t> class ptrhashtable_tpl;
template<class T> class slist_tpl;



/**
 * Reads uint8 from memory area. Advances pointer by 1 byte.
 */
inline uint8 decode_uint8(char * &data)
{
	const sint8 v = *((sint8 *)data);
	data ++;
	return v;
}

#define decode_sint8(data)  (sint8)decode_uint8(data)


/**
 * Reads uint16 from memory area. Advances pointer by 2 bytes.
 */
inline uint16 decode_uint16(char * &data)
{
	uint16 const v = (uint16)(uint8)data[0] | (uint16)(uint8)data[1] << 8;
	data += sizeof(v);
	return v;
}

#define decode_sint16(data)  (sint16)decode_uint16(data)


/**
 * Reads uint32 from memory area. Advances pointer by 4 bytes.
 */
inline uint32 decode_uint32(char * &data)
{
	uint32 const v = (uint32)(uint8)data[0] | (uint32)(uint8)data[1] << 8 | (uint32)(uint8)data[2] << 16 | (uint32)(uint8)data[3] << 24;
	data += sizeof(v);
	return v;
}

#define decode_sint32(data)  (sint32)decode_uint32(data)


#define OBJ_READER_DEF(classname, ty, ty_name) \
	public: \
		classname() { register_reader(); } \
		obj_type get_type() const OVERRIDE { return ty; } \
		const char *get_type_name() const OVERRIDE { return ty_name; } \
		static classname *instance() { return &the_instance; } \
	private: \
		static classname the_instance


class obj_reader_t
{
	//
	// table of registered obj readers sorted by id
	//
	typedef inthashtable_tpl<obj_type, obj_reader_t*> obj_map;
	static obj_map* obj_reader;
	//
	// object addresses needed for resolving xrefs later
	// - stored in a hash table with type and name
	//
	static inthashtable_tpl<obj_type, stringhashtable_tpl<obj_desc_t *> > loaded;
	typedef inthashtable_tpl<obj_type, stringhashtable_tpl<slist_tpl<obj_desc_t**> > > unresolved_map;
	static unresolved_map unresolved;
	static ptrhashtable_tpl<obj_desc_t **, int>  fatals;

	/// Read a descriptor node.
	/// @param fp File to read from
	/// @param[out] data If reading is successful, contains descriptor for the object, else NULL.
	/// @param register_nodes Nesting level for desc-nodes, should normally be 0
	/// @param version File format version
	static bool read_nodes(FILE* fp, obj_desc_t *&data, int register_nodes, uint32 version);
	static bool skip_nodes(FILE *fp, uint32 version);

protected:
	obj_reader_t() { /* Beware: Cannot register here! */}
	virtual ~obj_reader_t() {}

	static void obj_for_xref(obj_type type, const char *name, obj_desc_t *data);
	static void xref_to_resolve(obj_type type, const char *name, obj_desc_t **dest, bool fatal);
	static void resolve_xrefs();

	/// Read a descriptor from @p fp. Does version check and compatibility transformations.
	/// @returns The descriptor on success, or NULL on failure
	virtual obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) = 0;

	/// Register descriptor so the object described by the descriptor can be built in-game.
	virtual void register_obj(obj_desc_t *&/*desc*/) {}

	/// Does post-loading checks.
	/// @returns true if everything ok
	virtual bool successfully_loaded() const { return true; }

	void register_reader();

	template<typename T> static T* read_node(obj_node_info_t const& node)
	{
		if (node.size != 0) {
			dbg->fatal("obj_reader_t::read_node()", "node of type %.4s must have size 0, but has size %u", reinterpret_cast<char const*>(&node.type), node.size);
		}

		return new T();
	}

public:
	virtual obj_type get_type() const = 0;
	virtual const char *get_type_name() const = 0;

	static bool finish_loading();

	/**
	 * Loads all pak files from a directory, displaying a progress bar if the display is initialized
	 * @param path Directory to be scanned for PAK files
	 * @param message Label to show over the progress bar
	 */
	static bool load(const char *path, const char *message);

	// Only for single files, must take care of all the cleanup/registering matrix themselves
	static bool read_file(const char *name);
};

#endif
