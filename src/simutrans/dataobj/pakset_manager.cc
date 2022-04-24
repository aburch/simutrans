/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "pakset_manager.h"

#include "../utils/searchfolder.h"
#include "../display/simgraph.h"
#include "../sys/simsys.h"
#include "../simskin.h"
#include "../simloadingscreen.h"
#include "../descriptor/ground_desc.h"
#include "../descriptor/reader/obj_reader.h"
#include "../descriptor/vehicle_desc.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer.h"
#include "../dataobj/translator.h"
#include "../gui/help_frame.h"
#include "../gui/simwin.h"
#include "../dataobj/environment.h"
#include "../network/pakset_info.h"


pakset_manager_t::obj_map_t*                                  pakset_manager_t::registered_readers;
inthashtable_tpl<obj_type, stringhashtable_tpl<obj_desc_t*> > pakset_manager_t::loaded;
pakset_manager_t::unresolved_map_t                            pakset_manager_t::unresolved;
ptrhashtable_tpl<obj_desc_t**, int>                           pakset_manager_t::fatals;
std::string                                                   pakset_manager_t::doublettes;
stringhashtable_tpl<missing_level_t>                          pakset_manager_t::missing_pak_names;


static bool wait_for_key()
{
	event_t ev;
	display_poll_event(&ev);
	if(  ev.ev_class==EVENT_KEYBOARD  ) {
		if(  ev.ev_code==SIM_KEY_ESCAPE  ||  ev.ev_code==SIM_KEY_SPACE  ||  ev.ev_code==SIM_KEY_BACKSPACE  ) {
			return true;
		}
	}
	if(  IS_LEFTRELEASE(&ev)  ) {
		return true;
	}
	event_t *nev = new event_t();
	*nev = ev;
	queue_event(nev);
	return false;
}


void pakset_manager_t::register_reader(obj_reader_t *reader)
{
	if(!registered_readers) {
		registered_readers = new obj_map_t;
	}

	registered_readers->put(reader->get_type(), reader);
	//printf("This program can read %s objects\n", get_type_name());
}


void pakset_manager_t::load_pakset(bool load_addons)
{
	dbg->message("pakset_manager_t::load_pakset", "Reading object data from %s...", env_t::pak_dir.c_str());

	if (!load_paks_from_directory( env_t::pak_dir.c_str(), translator::translate("Loading paks ...") )) {
		dbg->fatal("pakset_manager_t::load_pakset", "Failed to load pakset. Please re-download or select another pakset.");
	}

	std::string overlaid_warning; // more prominent handling of double objects

	if(  had_overlaid()  ) {
		overlaid_warning = translator::translate("<h1>Error</h1><p><strong>");
		overlaid_warning.append( env_t::pak_name + translator::translate("contains the following doubled objects:</strong><p>") + get_overlaid() + "<p>" );
		clear_overlaid();
	}

	if(  load_addons  ) {
		// try to read addons from private directory
		dr_chdir( env_t::user_dir );

		if(!load_paks_from_directory("addons/" + env_t::pak_name, translator::translate("Loading addon paks ..."))) {
			dbg->warning("pakset_manager_t::load_pakset", "Reading addon object data failed (disabling).");
			env_t::default_settings.set_with_private_paks( false );
		}

		dr_chdir( env_t::base_dir );

		if(  had_overlaid()  ) {
			overlaid_warning.append( translator::translate("<h1>Warning</h1><p><strong>addons for") + env_t::pak_name + translator::translate("contains the following doubled objects:</strong><p>") + get_overlaid() );
			clear_overlaid();
		}
	}

	if (!finish_loading()) {
		dbg->fatal("pakset_manager_t::load_pakset", "Failed to load pakset. Please re-download or select another pakset.");
	}

	pakset_info_t::calculate_checksum();

	if(  env_t::verbose_debug >= log_t::LEVEL_DEBUG  ) {
		pakset_info_t::debug();
	}

	if(  !overlaid_warning.empty()  ) {
		overlaid_warning.append( "<p>Continue by click, ESC, SPACE, or BACKSPACE.<br>" );
		help_frame_t *win = new help_frame_t();
		win->set_text( overlaid_warning.c_str() );
		modal_dialogue( win, magic_pakset_info_t, NULL, wait_for_key );
		destroy_all_win(true);
	}
}


bool pakset_manager_t::load_paks_from_directory(const std::string &path, const char *message)
{
	const bool drawing = is_display_init();

	// step is a bitmask to decide when it's time to update the progress bar.
	// It takes the biggest power of 2 less than the number of elements and
	// divides it in 256 sub-steps at most (the -7 comes from here)

	searchfolder_t find;
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
		if (!load_pak_file(path + "symbol.BigLogo.pak")) {
			dbg->warning("pakset_manager_t::load_paks_from_directory", "File 'symbol.BigLogo.pak' cannot be read, startup logo will not be displayed!");
		}
	}

	loadingscreen_t ls( message, max, true );

	if(  ground_desc_t::outside==NULL  ) {
		// define the pak tile width
		if (!load_pak_file(path + "ground.Outside.pak")) {
			return false;
		}

		if(ground_desc_t::outside==NULL) {
			dbg->warning("pakset_manager_t::load_paks_from_directory", "File ground.Outside.pak not found, cannot guess tile size! (driving on left will not work!)");
		}
		else if (char const* const copyright = ground_desc_t::outside->get_copyright()) {
			ls.set_info(copyright);
		}
	}

DBG_MESSAGE("pakset_manager_t::load_paks_from_directory", "Reading from '%s'", path.c_str());

	uint n = 0;
	for (char* const& pak_filename : find) {
		if (!load_pak_file(pak_filename)) {
			dbg->warning("pakset_manager_t::load_paks_from_directory", "Cannot load '%s', some objects might be unavailable!", pak_filename);
		}

		if ((n++ & step) == 0 && drawing) {
			ls.set_progress(n);
		}
	}

	ls.set_progress(max);
	return find.begin()!=find.end();
}


bool pakset_manager_t::load_pak_file(const std::string &filename)
{
	// added trace
	DBG_DEBUG("pakset_manager_t::load_pak_file", "filename='%s'", filename.c_str());

	FILE* const fp = dr_fopen(filename.c_str(), "rb");
	if (!fp) {
		dbg->error("pakset_manager_t::load_pak_file", "Reading '%s' failed!", filename.c_str());
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
		dbg->error("pakset_manager_t::load_pak_file", "Unexpected end of file after %u bytes while reading '%s'!", n, filename.c_str());
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

	DBG_DEBUG("pakset_manager_t::load_pak_file", "Read %u blocks, file version is %x", n, version);

	if(version <= COMPILER_VERSION_CODE) {
		obj_desc_t *data = NULL;
		if (!read_nodes(fp, data, 0, version)) {
			fclose(fp);
			return false;
		}
	}
	else {
		DBG_DEBUG("pakset_manager_t::load_pak_file", "Version of '%s' is too old, %u instead of %u", filename.c_str(), version, COMPILER_VERSION_CODE );
		fclose(fp);
		return false;
	}

	fclose(fp);
	return true;
}


/*
 * Do the last loading procedures
 * Resolve all xrefs
 */
bool pakset_manager_t::finish_loading()
{
	// vehicle to follow to mark something cannot lead a convoi (prev[0]=any) or cannot end a convoi (next[0]=any)
	vehicle_desc_t::any_vehicle = new vehicle_desc_t(ignore_wt, 1, vehicle_desc_t::unknown);

	// first we add the any_vehicle to xrefs
	obj_for_xref( obj_vehicle, "any", vehicle_desc_t::any_vehicle );

	resolve_xrefs();

	for(auto const& elem : *registered_readers) {
		DBG_MESSAGE("pakset_manager_t::finish_loading", "Checking %s objects...", elem.value->get_type_name());

		if (!elem.value->successfully_loaded()) {
			dbg->warning("pakset_manager_t::finish_loading", "... failed!");
			return false;
		}
	}

	return true;
}


void pakset_manager_t::clear_missing_paks()
{
	missing_pak_names.clear();
}

// store missing obj during load and their severity
void pakset_manager_t::add_missing_paks( const char *name, missing_level_t level )
{
	if(  missing_pak_names.get( name )==NOT_MISSING  ) {
		missing_pak_names.put( strdup(name), level );
	}
}


void pakset_manager_t::warn_if_paks_missing()
{
	if(  missing_pak_names.empty()  ) {
		return; // everything OK
	}

	cbuffer_t msg;
	msg.append("<title>");
	msg.append(translator::translate("Missing pakfiles"));
	msg.append("</title>\n");

	cbuffer_t error_paks;
	cbuffer_t warning_paks;

	cbuffer_t paklog;
	paklog.append( "\n" );
	for(auto const& i : missing_pak_names) {
		if (i.value <= MISSING_ERROR) {
			error_paks.append(translator::translate(i.key));
			error_paks.append("<br>\n");
			paklog.append( i.key );
			paklog.append("\n" );
		}
		else {
			warning_paks.append(translator::translate(i.key));
			warning_paks.append("<br>\n");
		}
	}

	if(  error_paks.len()>0  ) {
		msg.append("<h1>");
		msg.append(translator::translate("Pak which may cause severe errors:"));
		msg.append("</h1><br>\n");
		msg.append("<br>\n");
		msg.append( error_paks );
		msg.append("<br>\n");
		dbg->warning( "The following paks are missing and may cause errors", paklog );
	}

	if(  warning_paks.len()>0  ) {
		msg.append("<h1>");
		msg.append(translator::translate("Pak which may cause visual errors:"));
		msg.append("</h1><br>\n");
		msg.append("<br>\n");
		msg.append( warning_paks );
		msg.append("<br>\n");
	}

	help_frame_t *win = new help_frame_t();
	win->set_text( msg );
	create_win(win, w_info, magic_pakset_info_t);
}


static bool read_node_info(obj_node_info_t& node, FILE* const f, uint32 const version)
{
	char data[EXT_OBJ_NODE_INFO_SIZE];
	if (fread(data, OBJ_NODE_INFO_SIZE, 1, f) != 1) {
		return false;
	}

	char *p = data;
	node.type      = decode_uint32(p);
	node.nchildren = decode_uint16(p);
	node.size      = decode_uint16(p);

	// can have larger records
	if (version != COMPILER_VERSION_CODE_11 && node.size == LARGE_RECORD_SIZE) {
		if (fread(p, sizeof(data) - OBJ_NODE_INFO_SIZE, 1, f) != 1) {
			return false;
		}
		node.size = decode_uint32(p);
	}

	return true;
}


bool pakset_manager_t::read_nodes(FILE *fp, obj_desc_t *&data, int node_depth, uint32 version)
{
	obj_node_info_t node;
	if (!read_node_info(node, fp, version)) {
		return false;
	}

	obj_reader_t *reader = registered_readers->get(static_cast<obj_type>(node.type));

	if(reader) {
//dbg->debug("pakset_manager_t::read_nodes", "Reading %.4s-node of length %d with '%s'", reinterpret_cast<const char *>(&node.type), node.size, reader->get_type_name());
		data = reader->read_node(fp, node);

		if (!data) {
			return false;
		}

		if (node.nchildren != 0) {
			data->children = new obj_desc_t *[node.nchildren];

			for (int i = 0; i < node.nchildren; i++) {
				if (!read_nodes(fp, data->children[i], node_depth + 1, version)) {
					// Note: cannot delete siblings of data->children[i], since equal images point to the same desc
					delete data; // data->children is delete[]'d by the destructor
					data = NULL;
					return false;
				}
			}
		}

//DBG_DEBUG("obj_reader_t","registering with '%s'", reader->get_type_name());
		if(node_depth<2  ||  node.type!=obj_cursor) {
			// since many buildings are with cursors that do not need registration
			reader->register_obj(data);
		}
	}
	else {
		// no reader found ...
		dbg->warning("pakset_manager_t::read_nodes", "Skipping unknown %.4s-node\n", reinterpret_cast<const char *>(&node.type));
		if (fseek(fp, node.size, SEEK_CUR) != 0) {
			return false;
		}

		for(int i = 0; i < node.nchildren; i++) {
			if (!skip_nodes(fp,version)) {
				return false;
			}
		}

		data = NULL;
	}

	return true;
}


bool pakset_manager_t::skip_nodes(FILE *fp,uint32 version)
{
	obj_node_info_t node;
	if (!read_node_info(node, fp, version)) {
		return false;
	}

	if (fseek(fp, node.size, SEEK_CUR) != 0) {
		return false;
	}

	for(int i = 0; i < node.nchildren; i++) {
		if (!skip_nodes(fp,version)) {
			return false;
		}
	}

	return true;
}


void pakset_manager_t::resolve_xrefs()
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
					dbg->fatal("pakset_manager_t::resolve_xrefs", "Cannot resolve '%4.4s-%s'", &u.key, i.key);
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


void pakset_manager_t::obj_for_xref(obj_type type, const char *name, obj_desc_t *data)
{
	stringhashtable_tpl<obj_desc_t *> *objtype_loaded = loaded.access(type);

	if(!objtype_loaded) {
		loaded.put(type);
		objtype_loaded = loaded.access(type);
	}
	objtype_loaded->remove(name);
	objtype_loaded->put(name, data);
}


void pakset_manager_t::xref_to_resolve(obj_type type, const char *name, obj_desc_t **dest, bool fatal)
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


void pakset_manager_t::doubled(const char *what, const char *name)
{
	dbg->warning("pakset_manager_t::doubled", "Object %s::%s is overlaid, using data of new object", what, name);
	doublettes.append( (std::string)what+"::"+name+"<br/>" );
}
