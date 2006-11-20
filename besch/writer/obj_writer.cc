#include "../../dataobj/tabfile.h"
#include "../../utils/cstring_t.h"
#include "../../tpl/stringhashtable_tpl.h"
#include "../../tpl/inthashtable_tpl.h"
#include "obj_node.h"
#include "obj_writer.h"
#include "text_writer.h"
#include "xref_writer.h"


void obj_writer_t::register_writer(bool main_obj)
{
	if (!writer_by_name) {
		writer_by_name = new stringhashtable_tpl<obj_writer_t*>;
		writer_by_type = new inthashtable_tpl<obj_type, obj_writer_t*>;
	}
	if (main_obj) {
		writer_by_name->put(get_type_name(), this);
	}
	writer_by_type->put(get_type(), this);

	///    printf("This program can compile %s objects\n", get_type_name());
}


void obj_writer_t::write(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	cstring_t type = obj.get("obj");
	cstring_t name = obj.get("name");

	obj_writer_t *writer = writer_by_name->get(type);
	if (!writer) {
		printf("skipping unknown %s object %s\n", (const char*)type, (const char*)name);
		return;
	}
	printf("      packing %s.%s\n", (const char*)type, (const char*)name);
	writer->write_obj(fp, parent, obj);
}


void obj_writer_t::write_head(FILE* fp, obj_node_t& node, tabfileobj_t& obj)
{
	const char* name = obj.get("name");
	const char* msg = obj.get("copyright");

	text_writer_t::instance()->write_obj(fp, node, name);
	text_writer_t::instance()->write_obj(fp, node, msg);
}


void obj_writer_t::dump_nodes(FILE* infp, int level)
{
	obj_node_info_t node;

	fread(&node, sizeof(node), 1, infp);
	long next_pos = ftell(infp) + node.size;

	obj_writer_t* writer = writer_by_type->get((obj_type)node.type);
	if (writer) {
		printf("%*s%4.4s-node (%s)", 3 * level, " ", (const char*)&node.type, writer->get_type_name());
		writer->dump_node(infp, node);
		printf("\n");
	}
	fseek(infp, next_pos, SEEK_SET);
	for (int i = 0; i < node.children; i++) {
		dump_nodes(infp, level + 1);
	}
}


void obj_writer_t::list_nodes(FILE* infp)
{
	obj_node_info_t node;

	fread(&node, sizeof(node), 1, infp);
	long next_pos = ftell(infp) + node.size;

	obj_writer_t* writer = writer_by_type->get((obj_type)node.type);
	if (writer) {
		fseek(infp, node.size, SEEK_CUR);
		printf("%-16s %s\n", writer->get_type_name(), (const char*)(writer->get_node_name(infp)));
	} else {
		printf("(unknown %4.4s)\n", (const char*)&node.type);
	}
	fseek(infp, next_pos, SEEK_SET);
	for (int i = 0; i < node.children; i++) {
		skip_nodes(infp);
	}
}


void obj_writer_t::show_capabilites()
{
	slist_tpl<obj_writer_t*> liste;
	cstring_t min_s = "";

	while (true) {
		stringhashtable_iterator_tpl<obj_writer_t *> iter(writer_by_name);
		cstring_t max_s = "zzz";
		while (iter.next()) {
			if (STRICMP(iter.get_current_key(), (const char*)min_s) > 0 && STRICMP(iter.get_current_key(), (const char*)max_s) < 0) {
				max_s = iter.get_current_key();
			}
		}
		if (max_s == "zzz") break;
		printf("   %s\n", (const char*)max_s);
		min_s = max_s;
	}
}


cstring_t obj_writer_t::name_from_next_node(FILE* fp) const
{
	cstring_t ret;
	char* buf;
	obj_node_info_t node;

	fread(&node, sizeof(node), 1, fp);
	buf = new char[node.size];
	fread(buf, node.size, 1, fp);
	ret = buf;
	delete buf;

	return ret;
}


void obj_writer_t::skip_nodes(FILE* fp)
{
	obj_node_info_t node;

	fread(&node, sizeof(node), 1, fp);

	fseek(fp, node.size, SEEK_CUR);
	for (int i = 0; i < node.children; i++) {
		skip_nodes(fp);
	}
}


void obj_writer_t::dump_node(FILE* /*infp*/, const obj_node_info_t& node)
{
	printf(" %d bytes", node.size);
}


const char* obj_writer_t::node_writer_name(FILE* infp) const
{
	obj_node_info_t node;
	fread(&node, sizeof(node), 1, infp);
	fseek(infp, node.size, SEEK_CUR);
	obj_writer_t* writer = writer_by_type->get((obj_type)node.type);
	if (writer) {
		return writer->get_type_name();
	}
	return "unknown";
}
