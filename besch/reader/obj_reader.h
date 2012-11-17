#ifndef __OBJ_READER_H
#define __OBJ_READER_H

#include <stdio.h>
#include <string.h>

#include "../obj_besch.h"
#include "../objversion.h"

#include "../../simtypes.h"


struct obj_node_info_t;
template<class key_t, class value_t> class inthashtable_tpl;
template<class value_t> class stringhashtable_tpl;
template<class key_t, class value_t> class ptrhashtable_tpl;
template<class T> class slist_tpl;



/**
 * Reads uint8 from memory area. Advances pointer by 1 byte.
 * @author Hj. Malthaner
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
 * @author Hj. Malthaner
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
 * @author Hj. Malthaner
 */
inline uint32 decode_uint32(char * &data)
{
	uint32 const v = (uint32)(uint8)data[0] | (uint32)(uint8)data[1] << 8 | (uint32)(uint8)data[2] << 16 | (uint32)(uint8)data[3] << 24;
	data += sizeof(v);
	return v;
}

#define decode_sint32(data)  (sint32)decode_uint32(data)



class obj_reader_t
{
	//
	// table of registered obj readers sorted by id
	//
	typedef inthashtable_tpl<obj_type, obj_reader_t*> obj_map;
	static obj_map* obj_reader;
	//
	// object addresses needed for resolving xrefs later
	// - stored in a hashhash table with type and name
	//
	static inthashtable_tpl<obj_type, stringhashtable_tpl<obj_besch_t *> > loaded;
	typedef inthashtable_tpl<obj_type, stringhashtable_tpl<slist_tpl<obj_besch_t**> > > unresolved_map;
	static unresolved_map unresolved;
	static ptrhashtable_tpl<obj_besch_t **, int>  fatals;

	static void read_file(const char *name);
	static void read_nodes(FILE* fp, obj_besch_t*& data, int register_nodes,uint32 version);
	static void skip_nodes(FILE *fp,uint32 version);

protected:
	static void delete_node(obj_besch_t *node);

	obj_reader_t() { /* Beware: Cannot register here! */}
	virtual ~obj_reader_t() {}

	static void obj_for_xref(obj_type type, const char *name, obj_besch_t *data);
	static void xref_to_resolve(obj_type type, const char *name, obj_besch_t **dest, bool fatal);
	static void resolve_xrefs();

	/**
	 * Hajo 11-Oct-03:
	 * this method reads a node. I made this virtual to
	 * allow subclasses to define their own strategies how to
	 * read nodes.
	 */
	virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
	virtual void register_obj(obj_besch_t *&/*data*/) {}
	virtual bool successfully_loaded() const { return true; }

	void register_reader();

public:
	virtual obj_type get_type() const = 0;
	virtual const char *get_type_name() const = 0;

	static bool init();
	static bool laden_abschliessen();
	/**
	 * Loads all pak files from a directory, displaying a progress bar if the display is initialized
	 * @param path Directory to be scanned for PAK files
	 * @param message Label to show over the progress bar
	 */
	static bool load(const char *path, const char *message);
};

#endif
