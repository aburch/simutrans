/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_OBJ_NODE_H
#define DESCRIPTOR_WRITER_OBJ_NODE_H


#include "../obj_node_info.h"
#include <stdio.h>


class  obj_writer_t;


class obj_node_t
{
private:
	/// Offset, from the beginning of the file, of the start of the next node header
	/// ( == number of bytes used for nodes so far)
	static uint32 next_node_header_offset;

	obj_node_info_t desc;

	uint32 body_offset;       ///< Start of node data in file (after node.desc)
	uint32 data_write_offset; ///< Additional running offset from @ref body_offset where to write the next data item for this node

	obj_node_t *parent;

public:
	// set_start_offset() - set offset of first node in file
	// ONLY CALL BEFORE ANY NODES ARE CREATED !!!
	static void set_start_offset(uint32 offset) { next_node_header_offset = offset; }

	// reads a node into a given obj_node_info_t
	static bool read_node(FILE* fp, obj_node_info_t &node );

	/// construct a new node.
	/// @param writer  object, that writes the node to the file
	/// @param size    space needed for node data
	/// @param parent  parent node
	obj_node_t(obj_writer_t* writer, uint32 size, obj_node_t* parent);

	// Write an array of bytes to the body of this node.
	// The caller is responsible that all areas are written by multiple calls
	// Throws obj_pak_exception_t on error
	/// @param[in,out] offset Offset from the beginning of @p fp to write the data.
	///                       This is increased by @p size once the data has been fully written.
	void write_bytes(FILE *fp, uint32 nbytes, const void *bytes);

	/// Writes the node version to @p fp. @p offset advances by 2 bytes.
	void write_version(FILE *fp, uint16 version);

	/// Writes uint8 value to @p fp at @p offset. @p offset advances by 1 byte.
	void write_uint8 (FILE *fp, uint8  data);
	void write_uint16(FILE *fp, uint16 data);
	void write_uint32(FILE *fp, uint32 data);

	void write_sint8 (FILE* fp, sint8  data) { this->write_uint8 (fp, (uint8) data); }
	void write_sint16(FILE* fp, sint16 data) { this->write_uint16(fp, (uint16)data); }
	void write_sint32(FILE* fp, sint32 data) { this->write_uint32(fp, (sint32)data); }

	// Write the internal node info to the file
	// DO THIS AFTER ALL CHILD NODES ARE WRITTEN !!!
	void check_and_write_header(FILE *fp);
};


#endif
