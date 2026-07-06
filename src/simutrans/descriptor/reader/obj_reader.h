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
#include "../../tpl/array_tpl.h"
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
	uint32 const v =
		(uint32)(uint8)data[0] <<  0 |
		(uint32)(uint8)data[1] <<  8 |
		(uint32)(uint8)data[2] << 16 |
		(uint32)(uint8)data[3] << 24;
	data += sizeof(v);
	return v;
}

#define decode_sint32(data)  (sint32)decode_uint32(data)


inline uint64 decode_uint64(char *&data)
{
	const uint64 v =
		(uint64)(uint8)data[0] <<  0 |
		(uint64)(uint8)data[1] <<  8 |
		(uint64)(uint8)data[2] << 16 |
		(uint64)(uint8)data[3] << 24 |
		(uint64)(uint8)data[4] << 32 |
		(uint64)(uint8)data[5] << 40 |
		(uint64)(uint8)data[6] << 48 |
		(uint64)(uint8)data[7] << 56;
	data += sizeof(v);
	return v;
}

#define decode_sint64(data)  (sint64)decode_uint64(data)



#define OBJ_READER_DEF(classname, ty, ty_name) \
	public: \
		classname() { pakset_manager_t::register_reader(this); } \
		obj_type get_type() const OVERRIDE { return ty; } \
		const char *get_type_name() const OVERRIDE { return ty_name; } \
		static classname *instance() { return &the_instance; } \
	private: \
		static classname the_instance


/// Owns one pak node's bytes and a bounds-checked cursor over them.
/// decode_*(node_body_t&) overloads read through it.
class node_body_t
{
public:
	/// fread @p size bytes from @p fp; short read leaves a bool-false cursor.
	node_body_t(FILE* fp, size_t size, const char* type_name)
	{
		type_name_ = type_name;
		if (size > 0) {
			if (size > buf_size) {
				buf_size = size | 0x0FFF;
				buf = (uint8*)realloc(buf,buf_size);
			}
			ptr = buf;
			size_t n = fread(ptr, 1, size, fp);
			if (n != size) {
				dbg->fatal("node_body_t()", "Cannot read %lu (only %lu) for %s", size, n, type_name);
			}
			end = ptr + size;
#ifdef DEBUG
			usage++;
#endif
		}
		else {
			end = ptr = buf;
			dbg->error("node_body_t()", "Node of size 0 requested for %s", type_name);
		}
	}

#ifdef DEBUG
	~node_body_t() {
		usage--;
		if (usage) {
			dbg->fatal("node_body_t()", "is not reentrant for %s", type_name_);
		}
	}
#endif

	/// false on short fread.
	explicit operator bool() const { return ptr != NULL; }

	/// Bounds-checked `p += n`.
	inline node_body_t& operator+=(size_t n) { ptr += n; return *this; }

	/// Bounds-checked single-byte skip (`p++`); return value unused.
	inline node_body_t& operator++(int) { ++ptr; return *this; }

	void seek(size_t off)
	{
		if (buf + off > end) {
			dbg->fatal("node_body_t()", "seek to offset %zu past buffer end %zu for %s", off, (size_t)(ptr-end), type_name_);
		}
		ptr = buf + off;
	}

	/// Bounds-checked: returns the cursor, then advances @p n bytes (tail reads).
	char* read_bytes(size_t n)
	{
		if (ptr + n <= end) {
			char* p = (char *)ptr;
			ptr += n;
			return p;
		}
		complain(n);
		return 0;
	}

	void read_uint16_block(uint16 *dest, size_t n)
	{
		uint8* cpy_end = ptr + 2 * n;
		// normal one
		if (cpy_end <= end) {
#ifndef SIM_BIG_ENDIAN
			memcpy(dest, ptr, 2 * n);
			ptr = cpy_end;
#else
			uint8* p = ptr;
			while (p < cpy_end) {
				uint16 v = *p++;
				v |= (uint16)*p++ << 8;
				*dest++ = v;
			}
			ptr = p;
#endif
			return;
		}
		complain(2*n);
	}

	inline uint8 read_uint8()
	{
		if (ptr < end) {
			return *ptr++;
		}
		return complain(1);
	}

	inline uint16 read_uint16()
	{
		if (ptr+1 < end) {
			uint16 v = (uint16)ptr[0] | (uint16)ptr[1] << 8;
			ptr += 2;
			return v;
		}
		return complain(2);
	}

	inline uint32 read_uint32()
	{
		if (ptr + 3 < end) {
			const uint32 v =
				(uint32)ptr[0]       |
				(uint32)ptr[1] <<  8 |
				(uint32)ptr[2] << 16 |
				(uint32)ptr[3] << 24;
			ptr += 4;
			return v;
		}
		return complain(4);
	}

	inline uint64 read_uint64()
	{
		if (ptr + 7 < end) {
			const uint64 v =
				(uint64)(uint8)ptr[0] << 0 |
				(uint64)(uint8)ptr[1] << 8 |
				(uint64)(uint8)ptr[2] << 16 |
				(uint64)(uint8)ptr[3] << 24 |
				(uint64)(uint8)ptr[4] << 32 |
				(uint64)(uint8)ptr[5] << 40 |
				(uint64)(uint8)ptr[6] << 48 |
				(uint64)(uint8)ptr[7] << 56;
			ptr += 8;
			return v;
		}
		return complain(8);
	}

private:
	uint8 complain(size_t n) const
	{
		dbg->fatal("node_body_t()", "Requested read beyond buffer (%lu was %lu too much) for %s", n, (size_t)(end - ptr), type_name_);
		return 0;
	}

#ifdef DEBUG
	static uint8 usage;
#endif
	static size_t buf_size;
	static uint8* buf;
	static uint8* ptr;
	static uint8* end;
	static const char* type_name_;
};

/// decode_*(node_body_t&) overloads, chosen over the char*& ones by
/// argument type so reader call sites stay `decode_uint16(p)`.
inline uint8  decode_uint8(node_body_t& b)  { return b.read_uint8(); }
inline uint16 decode_uint16(node_body_t& b) { return b.read_uint16(); }
inline uint32 decode_uint32(node_body_t& b) { return b.read_uint32(); }
inline uint64 decode_uint64(node_body_t& b) { return b.read_uint64(); }


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
