/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_PAKINSTALLER_H
#define GUI_PAKINSTALLER_H


#include "gui_frame.h"
#include "components/action_listener.h"

#include "components/gui_label.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_button.h"

#include "simwin.h"

#include "../dataobj/pakset_downloader.h"

class pakinstaller_t : public gui_frame_t, private action_listener_t
{
private:
	gui_scrolled_list_t paks;
	gui_scrolled_list_t obsolete_paks;
	button_t install, cancel;

	vector_tpl<paksetinfo_t*> paklist;

public:
	static bool finish_install;

	pakinstaller_t();

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
