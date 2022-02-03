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
	static uint32 free_offset; // next free offset in file

	obj_node_info_t desc;

	uint32 write_offset; // Start of node data in file (after node.desc)
	obj_node_t* parent;

public:
	// set_start_offset() - set offset of first node in file
	// ONLY CALL BEFORE ANY NODES ARE CREATED !!!
	static void set_start_offset(uint32 offset) { free_offset = offset; }

	// reads a node into a given obj_node_info_t
	static void read_node(FILE* fp, obj_node_info_t &node );

	/// construct a new node.
	/// @param writer  object, that writes the node to the file
	/// @param size    space needed for node data
	/// @param parent  parent node
	obj_node_t(obj_writer_t* writer, uint32 size, obj_node_t* parent);

	// Write the complete node data to the file
	void write_data(FILE* fp, const void* data);

	// Write part of the node data to the file
	// The caller is responsible that all areas are written y multiple calls
	// Throws obj_pak_exception_t
	void write_data_at(FILE* fp, const void* data, int offset, int size);

	void write_uint8(FILE* fp, uint8 data, int offset);
	void write_uint16(FILE* fp, uint16 data, int offset);
	void write_uint32(FILE* fp, uint32 data, int offset);

	void write_sint8(FILE* fp, sint8 data, int offset) {
		this->write_uint8(fp, (uint8) data, offset);
	}
	void write_sint16(FILE* fp, sint16 data, int offset) {
		this->write_uint16(fp, (uint16) data, offset);
	}
	void write_sint32(FILE* fp, sint32 data, int offset) {
		this->write_uint32(fp, (sint32) data, offset);
	}

	// Write the internal node info to the file
	// DO THIS AFTER ALL CHILD NODES ARE WRITTEN !!!
	void write(FILE* fp);
};


#endif
