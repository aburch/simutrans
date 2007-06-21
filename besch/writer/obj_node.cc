#include "../../utils/cstring_t.h"
#include "obj_pak_exception.h"
#include "obj_writer.h"
#include "obj_node.h"
#include "../obj_besch.h"


uint32 obj_node_t::free_offset;	    // next free offset in file


obj_node_t::obj_node_t(obj_writer_t* writer, int size, obj_node_t* parent, bool adjust)
{
	this->parent = parent;
	this->adjust = adjust;

	desc.type = writer->get_type();
	desc.children = 0;
	desc.size = adjust ? (size - sizeof(obj_besch_t)) : size;
	write_offset = free_offset + sizeof(desc);
	free_offset = write_offset + desc.size;
}


void obj_node_t::write(FILE* fp)
{
	fseek(fp, write_offset - sizeof(desc), SEEK_SET);
	fwrite(&desc, sizeof(desc), 1, fp);
	if (parent) {
		parent->desc.children++;
	}
}


void obj_node_t::write_data(FILE* fp, const void* data)
{
	write_data_at(fp, data, 0, adjust ? desc.size + 4 : desc.size);
}


void obj_node_t::write_data_at(FILE* fp, const void* data, int offset, int size)
{
	if (adjust) {
		if (offset < sizeof(obj_besch_t)) {
			data = static_cast<const char*>(data) + sizeof(obj_besch_t) - offset;
			size -= sizeof(obj_besch_t) - offset;
			offset = 0;
		} else {
			offset -= sizeof(obj_besch_t);
		}
	}
	if (offset < 0 || size < 0 || offset + size > desc.size) {
		cstring_t reason;
		reason.printf("invalid parameters (offset=%d, size=%d, obj_size=%d)", offset, size, desc.size);
		throw obj_pak_exception_t("obj_node_t", reason);
	}
	fseek(fp, write_offset + offset, SEEK_SET);
	fwrite(data, size, 1, fp);
}
