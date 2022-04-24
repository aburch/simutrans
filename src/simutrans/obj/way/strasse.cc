/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "strasse.h"
#include "../../world/simworld.h"
#include "../../dataobj/loadsave.h"
#include "../../descriptor/way_desc.h"
#include "../../builder/wegbauer.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/pakset_manager.h"


const way_desc_t *strasse_t::default_strasse=NULL;


void strasse_t::set_gehweg(bool janein)
{
	weg_t::set_gehweg(janein);
	if(janein  &&  get_desc()  &&  get_desc()->get_topspeed()>50) {
		set_max_speed(50);
	}
}



strasse_t::strasse_t(loadsave_t *file) : weg_t()
{
	rdwr(file);
}


strasse_t::strasse_t() : weg_t()
{
	set_gehweg(false);
	set_desc(default_strasse);
}



void strasse_t::rdwr(loadsave_t *file)
{
	xml_tag_t s( file, "strasse_t" );

	weg_t::rdwr(file);

	if(file->is_version_less(89, 0)) {
		bool gehweg;
		file->rdwr_bool(gehweg);
		set_gehweg(gehweg);
	}

	if(file->is_saving()) {
		const char *s = get_desc()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));

		const way_desc_t *desc = way_builder_t::get_desc(bname);
		int old_max_speed = get_max_speed();
		if(desc==NULL) {
			desc = way_builder_t::get_desc(translator::compatibility_name(bname));
			if(desc==NULL) {
				desc = default_strasse;
				pakset_manager_t::add_missing_paks( bname, MISSING_WAY );
			}
			dbg->warning("strasse_t::rdwr()", "Unknown street %s replaced by %s (old_max_speed %i)", bname, desc->get_name(), old_max_speed );
		}
		set_desc(desc);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
		if(desc->get_topspeed()>50  &&  hat_gehweg()) {
			set_max_speed(50);
		}
	}
}
