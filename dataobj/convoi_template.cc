/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "convoi_template.h"
#include "environment.h"
#include "tabfile.h"
#include "../utils/searchfolder.h"
#include "../simdebug.h"
#include "../sys/simsys.h"


static void load_from_dir(const std::string &dir, vector_tpl<convoi_template_t> &out)
{
	searchfolder_t sf;
	if (sf.search(dir, "tab") == 0) {
		return;
	}
	for (searchfolder_t::const_iterator it = sf.begin(); it != sf.end(); ++it) {
		tabfile_t tf;
		if (!tf.open(*it)) {
			dbg->warning("convoi_template_load", "Cannot open %s", *it);
			continue;
		}
		tabfileobj_t obj;
		while (tf.read(obj)) {
			const char *name = obj.get("name");
			if (!*name) continue;

			convoi_template_t tmpl;
			tmpl.name = name;
			const char *sep = strrchr(*it, '/');
			tmpl.source_file = sep ? sep + 1 : *it;

			// vehicle[0], vehicle[1], ... — one descriptor name per entry
			char key[32];
			for (int i = 0; ; i++) {
				snprintf(key, sizeof(key), "vehicle[%d]", i);
				const char *veh = obj.get(key);
				if (!*veh) break;
				tmpl.vehicles.push_back(veh);
			}

			if (!tmpl.vehicles.empty()) {
				out.append(tmpl);
			} else {
				dbg->error("convoi_template_load", "Convoy template \"%s\" (%s) has no vehicle entries.",
					tmpl.name.c_str(), tmpl.source_file.c_str());
			}
		}
	}
}


void convoi_template_load(const std::string &pak_dir, bool load_addons, vector_tpl<convoi_template_t> &out)
{
	out.clear();
	load_from_dir(pak_dir + "convoy_template/", out);
	if (load_addons) {
		dr_chdir(env_t::user_dir);
		load_from_dir("addons/" + env_t::objfilename + "convoy_template/", out);
		dr_chdir(env_t::data_dir);
	}
}
