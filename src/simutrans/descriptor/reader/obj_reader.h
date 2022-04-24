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
#include "../../dataobj/pakset_manager.h"


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
		classname() { pakset_manager_t::register_reader(this); } \
		obj_type get_type() const OVERRIDE { return ty; } \
		const char *get_type_name() const OVERRIDE { return ty_name; } \
		static classname *instance() { return &the_instance; } \
	private: \
		static classname the_instance


class obj_reader_t
{
public:
	obj_reader_t() { /* Beware: Cannot register_reader() here! */ }
	virtual ~obj_reader_t() {}

public:
	/// Read a descriptor from @p fp. Does version check and compatibility transformations.
	/// @returns The descriptor on success, or NULL on failure
	virtual obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) = 0;

	/// Register descriptor so the object described by the descriptor can be built in-game.
	virtual void register_obj(obj_desc_t *&/*desc*/) {}

	/// Does post-loading checks.
	/// @returns true if everything ok
	virtual bool successfully_loaded() const { return true; }

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
};

#endif
