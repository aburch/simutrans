#include <string>
#include <string.h>

// for the progress bar
#include "../../simcolor.h"
#include "../../display/simimg.h"
#include "../../simsys.h"
#include "../../simtypes.h"
#include "../../simloadingscreen.h"

#include "../skin_besch.h"	// just for the logo
#include "../grund_besch.h"	// for the error message!
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

#include "../obj_besch.h"
#include "../obj_node_info.h"

#include "obj_reader.h"


obj_reader_t::obj_map*                                         obj_reader_t::obj_reader;
inthashtable_tpl<obj_type, stringhashtable_tpl<obj_besch_t*> > obj_reader_t::loaded;
obj_reader_t::unresolved_map                                   obj_reader_t::unresolved;
ptrhashtable_tpl<obj_besch_t**, int>                           obj_reader_t::fatals;

void obj_reader_t::register_reader()
{
	if(!obj_reader) {
		obj_reader = new obj_map;
	}
	obj_reader->put(get_type(), this);
	//printf("This program can read %s objects\n", get_type_name());
}


bool obj_reader_t::laden_abschliessen()
{
	resolve_xrefs();

	FOR(obj_map, const& i, *obj_reader) {
		DBG_MESSAGE("obj_reader_t::laden_abschliessen()","Checking %s objects...", i.value->get_type_name());

		if (!i.value->successfully_loaded()) {
			dbg->warning("obj_reader_t::laden_abschliessen()","... failed!");
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
	const bool drawing=is_display_init();

	if(name.at(name.size() - 1) != '/') {
		// very old style ... (I think unused by now)

		if (FILE* const listfp = fopen(name.c_str(), "r")) {
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
				FOR(searchfolder_t, const& i, find) {
					read_file(i);
				}
			}
			fclose(listfp);
			return true;
		}
	}
	else {
		// Keine dat-file? dann ist liste ein Verzeichnis?
		// step is a bitmask to decide when it's time to update the progress bar.
		// It takes the biggest power of 2 less than the number of elements and
		// divides it in 256 sub-steps at most (the -7 comes from here)

		const int max = find.search(path, "pak");
		int step = -7;
		for(long bit=1;  bit<max;  bit+=bit) {
			step ++;
		}
		if(step<0) {
			step = 0;
		}
		step = (2<<step)-1;

		if(drawing  &&  skinverwaltung_t::biglogosymbol==NULL) {
			display_fillbox_wh( 0, 0, display_get_width(), display_get_height(), COL_BLACK, true );
			read_file((name+"symbol.BigLogo.pak").c_str());
DBG_MESSAGE("obj_reader_t::load()","big logo %p", skinverwaltung_t::biglogosymbol);
		}

		loadingscreen_t ls( message, max, true );

		if(  grund_besch_t::ausserhalb==NULL  ) {
			// defining the pak tile witdh ....
			read_file((name+"ground.Outside.pak").c_str());
			if(grund_besch_t::ausserhalb==NULL) {
				dbg->warning("obj_reader_t::load()","ground.Outside.pak not found, cannot guess tile size! (driving on left will not work!)");
			}
			else {
				if (char const* const copyright = grund_besch_t::ausserhalb->get_copyright()) {
					ls.set_info(copyright);
				}
			}
		}

DBG_MESSAGE("obj_reader_t::load()", "reading from '%s'", name.c_str());

		uint n = 0;
		FORX(searchfolder_t, const& i, find, ++n) {
			read_file(i);
			if ((n & step) == 0 && drawing) {
				ls.set_progress(n);
			}
		}

		return find.begin()!=find.end();
	}
	return false;
}


void obj_reader_t::read_file(const char *name)
{
	// Hajo: added trace
	DBG_DEBUG("obj_reader_t::read_file()", "filename='%s'", name);

	if (FILE* const fp = fopen(name, "rb")) {
		sint32 n = 0;

		// This is the normal header reading code
		int c;
		do {
			c = fgetc(fp);
			n ++;
		} while(!feof(fp) && c != 0x1a);

		if(feof(fp)) {
			// Hajo: added error check
			dbg->error("obj_reader_t::read_file()",	"unexpected end of file after %d bytes while reading '%s'!",n, name);
		}
		else {
//			DBG_DEBUG("obj_reader_t::read_file()", "skipped %d header bytes", n);
		}

		// Compiled Verison
		uint32 version = 0;
		char dummy[4], *p;
		p = dummy;

		n = fread(dummy, 4, 1, fp);
		version = decode_uint32(p);

		DBG_DEBUG("obj_reader_t::read_file()", "read %d blocks, file version is %x", n, version);

		if(version <= COMPILER_VERSION_CODE) {
			obj_besch_t *data = NULL;
			read_nodes(fp, data, 0, version );
		}
		else {
			DBG_DEBUG("obj_reader_t::read_file()","version of '%s' is too old, %d instead of %d", name, version, COMPILER_VERSION_CODE );
		}
		fclose(fp);
	}
	else {
		// Hajo: added error check
		dbg->error("obj_reader_t::read_file()", "reading '%s' failed!", name);
	}
}


void obj_reader_t::read_nodes(FILE* fp, obj_besch_t*& data, int register_nodes, uint32 version )
{
	obj_node_info_t node;
	char load_dummy[EXT_OBJ_NODE_INFO_SIZE], *p;

	p = load_dummy;
	if(  version==COMPILER_VERSION_CODE_11  ) {
		fread(p, OBJ_NODE_INFO_SIZE, 1, fp);
		node.type = decode_uint32(p);
		node.children = decode_uint16(p);
		node.size = decode_uint16(p);
	}
	else {
		// can have larger records
		fread(p, OBJ_NODE_INFO_SIZE, 1, fp);
		node.type = decode_uint32(p);
		node.children = decode_uint16(p);
		node.size = decode_uint16(p);
		if(  node.size==LARGE_RECORD_SIZE  ) {
			fread(p, 4, 1, fp);
			node.size = decode_uint32(p);
		}
	}

	obj_reader_t *reader = obj_reader->get(static_cast<obj_type>(node.type));
	if(reader) {

//DBG_DEBUG("obj_reader_t::read_nodes()","Reading %.4s-node of length %d with '%s'",	reinterpret_cast<const char *>(&node.type),	node.size,	reader->get_type_name());
		data = reader->read_node(fp, node);
		for(int i = 0; i < node.children; i++) {
			read_nodes(fp, data->node_info[i], register_nodes+1, version);
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
		fseek(fp, node.size, SEEK_CUR);
		for(int i = 0; i < node.children; i++) {
			skip_nodes(fp,version);
		}
		data = NULL;
	}
}


obj_besch_t *obj_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	obj_besch_t* besch = new(node.size) obj_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	if(node.size>0) {
		// not 32/64 Bit compatible for everything but char!
		dbg->warning("obj_reader_t::read_node()","native called on type %.4s (size %i), will break on 64Bit if type!=ASCII",reinterpret_cast<const char *>(&node.type),node.size);
		fread(besch + 1, node.size, 1, fp);
	}

	return besch;
}


void obj_reader_t::skip_nodes(FILE *fp,uint32 version)
{
	obj_node_info_t node;
	char load_dummy[OBJ_NODE_INFO_SIZE], *p;

	p = load_dummy;
	if(  version==COMPILER_VERSION_CODE_11  ) {
		fread(p, OBJ_NODE_INFO_SIZE, 1, fp);
		node.type = decode_uint32(p);
		node.children = decode_uint16(p);
		node.size = decode_uint16(p);
	}
	else {
		// can have larger records
		fread(p, OBJ_NODE_INFO_SIZE, 1, fp);
		node.type = decode_uint32(p);
		node.children = decode_uint16(p);
		node.size = decode_uint16(p);
		if(  node.size==LARGE_RECORD_SIZE  ) {
			fread(p, 4, 1, fp);
			node.size = decode_uint32(p);
		}
	}
//DBG_DEBUG("obj_reader_t::skip_nodes", "type %.4s (size %d)",reinterpret_cast<const char *>(&node.type),node.size);

	fseek(fp, node.size, SEEK_CUR);
	for(int i = 0; i < node.children; i++) {
		skip_nodes(fp,version);
	}
}


void obj_reader_t::delete_node(obj_besch_t *data)
{
	delete [] data->node_info;
	delete data;
}


void obj_reader_t::resolve_xrefs()
{
	slist_tpl<obj_besch_t *> xref_nodes;
	FOR(unresolved_map, const& u, unresolved) {
		FOR(stringhashtable_tpl<slist_tpl<obj_besch_t**> >, const& i, u.value) {
			obj_besch_t *obj_loaded = NULL;

			if (!strempty(i.key)) {
				if (stringhashtable_tpl<obj_besch_t*>* const objtype_loaded = loaded.access(u.key)) {
					obj_loaded = objtype_loaded->get(i.key);
				}
			}

			FOR(slist_tpl<obj_besch_t**>, const x, i.value) {
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
		delete_node(xref_nodes.remove_first());
	}

	loaded.clear();
	unresolved.clear();
	fatals.clear();
}


void obj_reader_t::obj_for_xref(obj_type type, const char *name, obj_besch_t *data)
{
	stringhashtable_tpl<obj_besch_t *> *objtype_loaded = loaded.access(type);

	if(!objtype_loaded) {
		loaded.put(type);
		objtype_loaded = loaded.access(type);
	}
	objtype_loaded->remove(name);
	objtype_loaded->put(name, data);
}


void obj_reader_t::xref_to_resolve(obj_type type, const char *name, obj_besch_t **dest, bool fatal)
{
	stringhashtable_tpl< slist_tpl<obj_besch_t **> > *typeunresolved = unresolved.access(type);

	if(!typeunresolved) {
		unresolved.put(type);
		typeunresolved = unresolved.access(type);
	}
	slist_tpl<obj_besch_t **> *list = typeunresolved->access(name);
	if(!list) {
		typeunresolved->put(name);
		list = typeunresolved->access(name);
	}
	list->insert(dest);
	if(fatal) {
		fatals.put(dest, 1);
	}
}
