/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "obj_pak_exception.h"
#include "obj_writer.h"
#include "obj_node.h"
#include "../obj_desc.h"


uint32 obj_node_t::free_offset; // next free offset in file


obj_node_t::obj_node_t(obj_writer_t* writer, uint32 size, obj_node_t* parent)
{
	this->parent = parent;

	desc.type = writer->get_type();
	desc.nchildren = 0;
	desc.size = size;
	if(  size<LARGE_RECORD_SIZE  ) {
		write_offset = free_offset + OBJ_NODE_INFO_SIZE; // put size of dis here!
		free_offset = write_offset + desc.size;
	}
	else {
		write_offset = free_offset + EXT_OBJ_NODE_INFO_SIZE; // put size of dis here!
		free_offset = write_offset + desc.size;
	}
}


// reads a node and advance the file pointer accordingly
void obj_node_t::read_node(FILE* fp, obj_node_info_t &node )
{
	uint32 u32;
	uint16 u16;

	fread(&u32, 4, 1, fp);
	node.type = endian(u32);
	fread(&u16, 2, 1, fp);
	node.nchildren = endian(u16);
	fread(&u16, 2, 1, fp);
	node.size = endian(u16);
	if(  node.size==LARGE_RECORD_SIZE  ) {
		fread(&u32, 4, 1, fp);
		node.size = endian(u32);
	}
}


void obj_node_t::write(FILE* fp)
{
	if(  desc.size<LARGE_RECORD_SIZE  ) {
		fseek(fp, write_offset - OBJ_NODE_INFO_SIZE, SEEK_SET);
		uint32 type     = endian(desc.type);
		uint16 children = endian(desc.nchildren);
		uint16 size16   = endian(uint16(desc.size));
		fwrite(&type, 4, 1, fp);
		fwrite(&children, 2, 1, fp);
		fwrite(&size16, 2, 1, fp);
	}
	else {
		// extended length record
		fseek(fp, write_offset - EXT_OBJ_NODE_INFO_SIZE, SEEK_SET);
		uint32 type     = endian(desc.type);
		uint16 children = endian(desc.nchildren);
		uint16 size16   = endian(uint16(LARGE_RECORD_SIZE));
		uint32 size     = endian(desc.size);
		fwrite(&type, 4, 1, fp);
		fwrite(&children, 2, 1, fp);
		fwrite(&size16, 2, 1, fp); // 0xFFFF
		fwrite(&size, 4, 1, fp);
	}
	if (parent) {
		parent->desc.nchildren++;
	}
}


void obj_node_t::write_data(FILE* fp, const void* data)
{
	write_data_at(fp, data, 0, desc.size);
}


void obj_node_t::write_data_at(FILE* fp, const void* data, int offset, int size)
{
	if (offset < 0 || size < 0 || (uint32)(offset + size) > desc.size) {
		char reason[1024];
		sprintf(reason, "invalid parameters (offset=%d, size=%d, obj_size=%d)", offset, size, desc.size);
		throw obj_pak_exception_t("obj_node_t", reason);
	}
	fseek(fp, write_offset + offset, SEEK_SET);
	fwrite(data, size, 1, fp);
}


void obj_node_t::write_uint8(FILE* fp, uint8 data, int offset)
{
	this->write_data_at(fp, &data, offset, 1);
}


void obj_node_t::write_uint16(FILE* fp, uint16 data, int offset)
{
	uint16 data2 = endian(data);
	this->write_data_at(fp, &data2, offset, 2);
}


void obj_node_t::write_uint32(FILE* fp, uint32 data, int offset)
{
	uint32 data2 = endian(data);
	this->write_data_at(fp, &data2, offset, 4);
}
