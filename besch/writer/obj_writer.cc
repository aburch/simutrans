#include <string>
#include "../../utils/simstring.h"
#include "../../dataobj/tabfile.h"
#include "../../tpl/stringhashtable_tpl.h"
#include "../../tpl/inthashtable_tpl.h"
#include "obj_node.h"
#include "obj_writer.h"
#include "text_writer.h"
#include "xref_writer.h"

using std::string;

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
	string type = obj.get("obj");
	string name = obj.get("name");

	obj_writer_t *writer = writer_by_name->get(type.c_str());
	if (!writer) {
		printf("skipping unknown %s object %s\n", type.c_str(), name.c_str());
		return;
	}
	printf("      packing %s.%s\n", type.c_str(), name.c_str());
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

	obj_node_t::read_node( infp, node );
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

	obj_node_t::read_node( infp, node );
	long next_pos = ftell(infp) + node.size;

	obj_writer_t* writer = writer_by_type->get((obj_type)node.type);
	if (writer) {
		fseek(infp, node.size, SEEK_CUR);
		printf("%-16s %s\n", writer->get_type_name(), (writer->get_node_name(infp)).c_str());
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
	string min_s = "";

	while (true) {
		stringhashtable_iterator_tpl<obj_writer_t *> iter(writer_by_name);
		string max_s = "zzz";
		while (iter.next()) {
			if (STRICMP(iter.get_current_key(), min_s.c_str()) > 0 && STRICMP(iter.get_current_key(), max_s.c_str()) < 0) {
				max_s = iter.get_current_key();
			}
		}
		if (max_s == "zzz") break;
		printf("   %s\n", max_s.c_str());
		min_s = max_s;
	}
}


string obj_writer_t::name_from_next_node(FILE* fp) const
{
	string ret;
	char* buf;
	obj_node_info_t node;

	obj_node_t::read_node( fp, node );
	if(node.type==obj_text) {
		buf = new char[node.size];
		fread(buf, node.size, 1, fp);
		ret = buf;
		delete buf;

		return ret;
	}
	else {
		return "";
	}
}


void obj_writer_t::skip_nodes(FILE* fp)
{
	obj_node_info_t node;

	obj_node_t::read_node( fp, node );
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
	obj_node_t::read_node( infp, node );
	fseek(infp, node.size, SEEK_CUR);
	obj_writer_t* writer = writer_by_type->get((obj_type)node.type);
	if (writer) {
		return writer->get_type_name();
	}
	return "unknown";
}
