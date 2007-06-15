#ifndef OBJ_NODE_H
#define OBJ_NODE_H

#include "../obj_node_info.h"
#include <stdio.h>


class  obj_writer_t;


class obj_node_t {
	private:
		static uint32 free_offset; // next free offset in file

		obj_node_info_t desc;

		uint32 write_offset; // Start of node data in file (after node.desc)
		obj_node_t* parent;
		bool adjust; // a normal besch structure start with 4 extra bytes

	public:
		// set_start_offset() - set offset of first node in file
		// ONLY CALL BEFORE ANY NODES ARE CREATED !!!
		static void set_start_offset(uint32 offset) { free_offset = offset; }

		// construct a new node, parameters:
		//	    writer  object, that writes the node to the file
		//	    size    space needed for node data
		//	    parent  parent node
		obj_node_t(obj_writer_t* writer, int size, obj_node_t* parent, bool adjust);

		// Write the complete node data to the file
		void write_data(FILE* fp, const void* data);

		// Write part of the node data to the file
		// The caller is responsible that all areas are written y multiple calls
		// Throws obj_pak_exception_t
		void write_data_at(FILE* fp, const void* data, int offset, int size);

		// Write the internal node info to the file
		// DO THIS AFTER ALL CHILD NODES ARE WRITTEN !!!
		void write(FILE* fp);
};

#endif
