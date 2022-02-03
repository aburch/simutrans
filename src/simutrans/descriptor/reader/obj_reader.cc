/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <string.h>

// for the progress bar
#include "../../simcolor.h"
#include "../../display/simimg.h"
#include "../../simtypes.h"
#include "../../simloadingscreen.h"
#include "../../sys/simsys.h"

#include "../skin_desc.h"   // just for the logo
#include "../ground_desc.h" // for the error message!
#include "../../simskin.h"

// normal stuff
#include "../../dataobj/translator.h"
#include "../../dataobj/environment.h"

#include "../../utils/searchfolder.h"
#include "../../utils/simstring.h"

#include "../../tpl/inthashtable_tpl.h"
#include "../../tpl/ptrhashtable_tpl.h"
#include "../../tpl/stringhashtable_tpl.h"
#include "../../simdebug.h"

#include "../obj_desc.h"
#include "../obj_node_info.h"
#include "../vehicle_desc.h"

#include "obj_reader.h"


obj_reader_t::obj_map*                                        obj_reader_t::obj_reader;
inthashtable_tpl<obj_type, stringhashtable_tpl<obj_desc_t*> > obj_reader_t::loaded;
obj_reader_t::unresolved_map                                  obj_reader_t::unresolved;
ptrhashtable_tpl<obj_desc_t**, int>                           obj_reader_t::fatals;

void obj_reader_t::register_reader()
{
	if(!obj_reader) {
		obj_reader = new obj_map;
	}
	obj_reader->put(get_type(), this);
	//printf("This program can read %s objects\n", get_type_name());
}

/*
 * Do the last loading procedures
 * Resolve all xrefs
 */
bool obj_reader_t::finish_loading()
{
	// vehicle to follow to mark something cannot lead a convoi (prev[0]=any) or cannot end a convoi (next[0]=any)
	vehicle_desc_t::any_vehicle = new vehicle_desc_t(ignore_wt, 1, vehicle_desc_t::unknown);

	// first we add the any_vehicle to xrefs
	obj_for_xref( obj_vehicle, "any", vehicle_desc_t::any_vehicle );

	resolve_xrefs();

	for(auto const& i : *obj_reader) {
		DBG_MESSAGE("obj_reader_t::finish_loading()","Checking %s objects...", i.value->get_type_name());

		if (!i.value->successfully_loaded()) {
			dbg->warning("obj_reader_t::finish_loading()","... failed!");
			return false;
		}
	}
	return true;
}


bool obj_reader_t::load(const char *path, const char *message)
{
	searchfolder_t find;
	std::string name = find.complete(path, "dat");
	size_t i;
	const bool drawing = is_display_init();

	if(name.at(name.size() - 1) != '/') {
		// very old style ... (I think unused by now)

		if (FILE* const listfp = dr_fopen(name.c_str(), "r")) {
			while(!feof(listfp)) {
				char buf[256];

				if (fgets(buf, sizeof(buf), listfp) == 0) {
					continue;
				}

				if(*buf == '#') {
					continue;
				}
				i = strlen(buf);
				while(i && buf[i] < 32) {
					buf[i--] = '\0';
				}
				if(!i) {
					continue;
				}

				find.search(buf, "pak");
				for(const char* const& i : find) {
					if (!read_file(i)) {
						return false;
					}
				}
			}

			fclose(listfp);
			return true;
		}

		return false;
	}
	else {
		// No dat-file? Then is the list a directory?
		// step is a bitmask to decide when it's time to update the progress bar.
		// It takes the biggest power of 2 less than the number of elements and
		// divides it in 256 sub-steps at most (the -7 comes from here)

		const sint32 max = find.search(path, "pak");
		sint32 step = -7;
		for(  sint32 bit = 1;  bit < max;  bit += bit  ) {
			step++;
		}
		if(  step < 0  ) {
			step = 0;
		}
		step = (2<<step)-1;

		if(drawing  &&  skinverwaltung_t::biglogosymbol==NULL) {
			display_fillbox_wh_rgb( 0, 0, display_get_width(), display_get_height(), color_idx_to_rgb(COL_BLACK), true );
			if (!read_file((name+"symbol.BigLogo.pak").c_str())) {
				dbg->warning("obj_reader_t::load()", "File 'symbol.BigLogo.pak' cannot be read, startup logo will not be displayed!");
			}
		}

		loadingscreen_t ls( message, max, true );

		if(  ground_desc_t::outside==NULL  ) {
			// defining the pak tile width ....
			if (!read_file((name+"ground.Outside.pak").c_str())) {
				return false;
			}

			if(ground_desc_t::outside==NULL) {
				dbg->warning("obj_reader_t::load()", "ground.Outside.pak not found, cannot guess tile size! (driving on left will not work!)");
			}
			else if (char const* const copyright = ground_desc_t::outside->get_copyright()) {
				ls.set_info(copyright);
			}
		}

DBG_MESSAGE("obj_reader_t::load()", "reading from '%s'", name.c_str());

		uint n = 0;
		for(char* const& i : find) {
			if (!read_file(i)) {
				dbg->warning("obj_reader_t::load()", "Cannot load '%s', some objects might be unavailable!", i);
			}

			if ((n++ & step) == 0 && drawing) {
				ls.set_progress(n);
			}
		}
		ls.set_progress(max);

		return find.begin()!=find.end();
	}
}


bool obj_reader_t::read_file(const char *name)
{
	// added trace
	DBG_DEBUG("obj_reader_t::read_file()", "filename='%s'", name);

	FILE* const fp = dr_fopen(name, "rb");
	if (!fp) {
		dbg->error("obj_reader_t::read_file()", "reading '%s' failed!", name);
		return false;
	}

	// This is the normal header reading code
	int c;
	uint32 n = 0;

	do {
		c = fgetc(fp);
		n ++;
	} while(c != EOF && c != 0x1a);

	if(c == EOF) {
		dbg->error("obj_reader_t::read_file()", "unexpected end of file after %u bytes while reading '%s'!",n, name);
		fclose(fp);
		return false;
	}

	// Compiled Version
	char dummy[4];
	n = fread(dummy, 4, 1, fp);
	if (n != 1) {
		fclose(fp);
		return false;
	}

	char *p = dummy;
	const uint32 version = decode_uint32(p);

	DBG_DEBUG("obj_reader_t::read_file()", "read %u blocks, file version is %x", n, version);

	if(version <= COMPILER_VERSION_CODE) {
		obj_desc_t *data = NULL;
		if (!read_nodes(fp, data, 0, version)) {
			fclose(fp);
			return false;
		}
	}
	else {
		DBG_DEBUG("obj_reader_t::read_file()","version of '%s' is too old, %u instead of %u", name, version, COMPILER_VERSION_CODE );
		fclose(fp);
		return false;
	}

	fclose(fp);
	return true;
}


static bool read_node_info(obj_node_info_t& node, FILE* const f, uint32 const version)
{
	char data[EXT_OBJ_NODE_INFO_SIZE];
	if (fread(data, OBJ_NODE_INFO_SIZE, 1, f) != 1) {
		return false;
	}

	char *p = data;
	node.type     = decode_uint32(p);
	node.children = decode_uint16(p);
	node.size     = decode_uint16(p);

	// can have larger records
	if (version != COMPILER_VERSION_CODE_11 && node.size == LARGE_RECORD_SIZE) {
		if (fread(p, sizeof(data) - OBJ_NODE_INFO_SIZE, 1, f) != 1) {
			return false;
		}
		node.size = decode_uint32(p);
	}

	return true;
}


bool obj_reader_t::read_nodes(FILE *fp, obj_desc_t *&data, int register_nodes, uint32 version)
{
	obj_node_info_t node;
	if (!read_node_info(node, fp, version)) {
		return false;
	}

	obj_reader_t *reader = obj_reader->get(static_cast<obj_type>(node.type));

	if(reader) {
//DBG_DEBUG("obj_reader_t::read_nodes()","Reading %.4s-node of length %d with '%s'", reinterpret_cast<const char *>(&node.type), node.size, reader->get_type_name());
		data = reader->read_node(fp, node);
		if (!data) {
			return false;
		}

		if (node.children != 0) {
			data->children = new obj_desc_t *[node.children];

			for (int i = 0; i < node.children; i++) {
				if (!read_nodes(fp, data->children[i], register_nodes + 1, version)) {
					// Note: cannot delete siblings of data->children[i], since equal images point to the same desc
					delete data; // data->children is delete[]'d by the destructor
					data = NULL;
					return false;
				}
			}
		}

//DBG_DEBUG("obj_reader_t","registering with '%s'", reader->get_type_name());
		if(register_nodes<2  ||  node.type!=obj_cursor) {
			// since many buildings are with cursors that do not need registration
			reader->register_obj(data);
		}
	}
	else {
		// no reader found ...
		dbg->warning("obj_reader_t::read_nodes()","skipping unknown %.4s-node\n",reinterpret_cast<const char *>(&node.type));
		if (fseek(fp, node.size, SEEK_CUR) != 0) {
			return false;
		}

		for(int i = 0; i < node.children; i++) {
			if (!skip_nodes(fp,version)) {
				return false;
			}
		}

		data = NULL;
	}

	return true;
}


bool obj_reader_t::skip_nodes(FILE *fp,uint32 version)
{
	obj_node_info_t node;
	if (!read_node_info(node, fp, version)) {
		return false;
	}

	if (fseek(fp, node.size, SEEK_CUR) != 0) {
		return false;
	}

	for(int i = 0; i < node.children; i++) {
		if (!skip_nodes(fp,version)) {
			return false;
		}
	}

	return true;
}


void obj_reader_t::resolve_xrefs()
{
	slist_tpl<obj_desc_t *> xref_nodes;
	for(auto const& u : unresolved) {
		for(auto const& i : u.value) {
			obj_desc_t *obj_loaded = NULL;

			if (!strempty(i.key)) {
				if (stringhashtable_tpl<obj_desc_t*>* const objtype_loaded = loaded.access(u.key)) {
					obj_loaded = objtype_loaded->get(i.key);
				}
			}

			for(obj_desc_t** const x : i.value) {
				if (!obj_loaded && fatals.get(x)) {
					dbg->fatal("obj_reader_t::resolve_xrefs", "cannot resolve '%4.4s-%s'", &u.key, i.key);
				}
				// delete old xref-node
				xref_nodes.append(*x);
				*x = obj_loaded;
			}
		}
	}

	while (!xref_nodes.empty()) {
		delete xref_nodes.remove_first();
	}

	loaded.clear();
	unresolved.clear();
	fatals.clear();
}


void obj_reader_t::obj_for_xref(obj_type type, const char *name, obj_desc_t *data)
{
	stringhashtable_tpl<obj_desc_t *> *objtype_loaded = loaded.access(type);

	if(!objtype_loaded) {
		loaded.put(type);
		objtype_loaded = loaded.access(type);
	}
	objtype_loaded->remove(name);
	objtype_loaded->put(name, data);
}


void obj_reader_t::xref_to_resolve(obj_type type, const char *name, obj_desc_t **dest, bool fatal)
{
	stringhashtable_tpl< slist_tpl<obj_desc_t **> > *typeunresolved = unresolved.access(type);

	if(!typeunresolved) {
		unresolved.put(type);
		typeunresolved = unresolved.access(type);
	}
	slist_tpl<obj_desc_t **> *list = typeunresolved->access(name);
	if(!list) {
		typeunresolved->put(name);
		list = typeunresolved->access(name);
	}
	list->insert(dest);
	if(fatal) {
		fatals.put(dest, 1);
	}
}
