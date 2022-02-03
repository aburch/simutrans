/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"
#include "../sys/simsys.h"

#include "ai_selector.h"
#include "messagebox.h"

#include "simwin.h"
#include "../world/simworld.h"

#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../player/ai_scripted.h"

#include "../utils/cbuffer.h"

ai_selector_t::ai_selector_t(uint8 plnr_) : savegame_frame_t(NULL, true, NULL, false)
{
	plnr = plnr_;
	cbuffer_t buf;
	buf.printf("%s/ai/", env_t::data_dir);

	this->add_path("addons/ai/");
	this->add_path(buf);

	title.clear();
	title.printf("%s - %d", translator::translate("Load Scripted AI"), plnr);
	set_name(title);
	set_focus(NULL);
}


/**
 * Action, started after button pressing.
 */
bool ai_selector_t::item_action(const char *fullpath)
{
	ai_scripted_t *ai = dynamic_cast<ai_scripted_t*>(welt->get_player(plnr));
	if (ai == NULL  ||  ai->has_script()) {
		return true;
	}

	const char* err = ai->init(this->get_basename(fullpath).c_str(), this->get_filename(fullpath).c_str());

	if (err == NULL) {
		return true;
	}
	else {
		create_win(new news_img(err), w_info, magic_none);
		return false; // keep window open
	}
}


const char *ai_selector_t::get_info(const char *filename)
{
	static char info[PATH_MAX];

	sprintf(info,"%s",this->get_filename(filename, false).c_str());

	return info;
}


bool ai_selector_t::check_file( const char *filename, const char * )
{
	char buf[PATH_MAX];
	sprintf( buf, "%s/ai.nut", filename );
	if (FILE* const f = dr_fopen(buf, "r")) {
		fclose(f);
		return true;
	}
	return false;
}
