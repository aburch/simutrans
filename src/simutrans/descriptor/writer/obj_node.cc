/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "obj_pak_exception.h"
#include "obj_writer.h"
#include "obj_node.h"
#include "../obj_desc.h"

#include <cassert>


uint32 obj_node_t::next_node_header_offset; // next free offset in file


obj_node_t::obj_node_t(obj_writer_t* writer, uint32 size, obj_node_t* parent)
{
	this->parent = parent;
	this->data_write_offset = 0;

	desc.type = writer->get_type();
	desc.nchildren = 0;
	desc.size = size;

	if(  size<LARGE_RECORD_SIZE  ) {
		this->body_offset = obj_node_t::next_node_header_offset + OBJ_NODE_INFO_SIZE; // put size of dis here!
		obj_node_t::next_node_header_offset = this->body_offset + this->desc.size;
	}
	else {
		this->body_offset = obj_node_t::next_node_header_offset + EXT_OBJ_NODE_INFO_SIZE; // put size of dis here!
		obj_node_t::next_node_header_offset = this->body_offset + this->desc.size;
	}
}


// reads a node and advance the file pointer accordingly
bool obj_node_t::read_node(FILE* fp, obj_node_info_t &node )
{
	uint32 u32 = 0;
	uint16 u16 = 0;

	if (fread(&u32, sizeof(uint32), 1, fp) != 1) {
		return false;
	}
	node.type = endian(u32);

	if (fread(&u16, sizeof(uint16), 1, fp) != 1) {
		return false;
	}
	node.nchildren = endian(u16);

	if (fread(&u16, sizeof(uint16), 1, fp) != 1) {
		return false;
	}
	node.size = endian(u16);

	if(  node.size==LARGE_RECORD_SIZE  ) {
		if (fread(&u32, sizeof(uint32), 1, fp) != 1) {
			return false;
		}
		node.size = endian(u32);
	}

	return true;
}


void obj_node_t::check_and_write_header(FILE *fp)
{
	assert(this->data_write_offset == this->desc.size);

	if(  desc.size<LARGE_RECORD_SIZE  ) {
		fseek(fp, body_offset - OBJ_NODE_INFO_SIZE, SEEK_SET);
		uint32 type     = endian(desc.type);
		uint16 children = endian(desc.nchildren);
		uint16 size16   = endian(uint16(desc.size));
		fwrite(&type,     sizeof(type),     1, fp);
		fwrite(&children, sizeof(children), 1, fp);
		fwrite(&size16,   sizeof(size16),   1, fp);
	}
	else {
		// extended length record
		fseek(fp, body_offset - EXT_OBJ_NODE_INFO_SIZE, SEEK_SET);
		uint32 type     = endian(desc.type);
		uint16 children = endian(desc.nchildren);
		uint16 size16   = endian(uint16(LARGE_RECORD_SIZE));
		uint32 size     = endian(desc.size);

		fwrite(&type,     sizeof(type),     1, fp);
		fwrite(&children, sizeof(children), 1, fp);
		fwrite(&size16,   sizeof(size16),   1, fp); // 0xFFFF
		fwrite(&size,     sizeof(size),     1, fp);
	}

	if (parent) {
		parent->desc.nchildren++;
	}
}


void obj_node_t::write_bytes(FILE* fp, uint32 size, const void *data)
{
	// beware of overflow
	if (UINT32_MAX - size < this->data_write_offset ||
		this->data_write_offset + size > desc.size)
	{
		char reason[1024];
		sprintf(reason, "invalid parameters (%u + %u > %u)", this->data_write_offset, size, desc.size);
		throw obj_pak_exception_t("obj_node_t", reason);
	}

	fseek(fp, body_offset + this->data_write_offset, SEEK_SET);
	fwrite(data, size, 1, fp);
	this->data_write_offset += size;
}


void obj_node_t::write_version(FILE *fp, uint16 version)
{
	// Version needs high bit set as trigger -> this is required
	// as marker because formerly nodes were unversionend
	version |= UINT16_C(0x8000);
	write_uint16(fp, version);
}


void obj_node_t::write_uint8(FILE* fp, uint8 data)
{
	this->write_bytes(fp, sizeof(data), &data);
}


void obj_node_t::write_uint16(FILE* fp, uint16 data)
{
	uint16 data2 = endian(data);
	this->write_bytes(fp, sizeof(data2), &data2);
}


void obj_node_t::write_uint32(FILE *fp, uint32 data)
{
	uint32 data2 = endian(data);
	this->write_bytes(fp, sizeof(data2), &data2);
}
