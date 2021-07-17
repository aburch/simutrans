/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../utils/cbuffer_t.h"

#include "pakinstaller.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../sys/simsys.h"

#include "../paksetinfo.h"

bool pakinstaller_t::finish_install;

pakinstaller_t::pakinstaller_t() :
	gui_frame_t(translator::translate("Install graphits")),
	paks(gui_scrolled_list_t::listskin),
	obsolete_paks(gui_scrolled_list_t::listskin)
{
	finish_install = false;

	set_table_layout(1, 0);

	new_component<gui_label_t>( "Select one or more graphics to install (Ctrl+click):" );

	for (int i = 0; i < 10; i++) {
		paks.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(pakinfo[i*2+1], i<10?SYSCOL_TEXT: SYSCOL_TEXT_SHADOW);
	}
	paks.enable_multiple_selection();
	scr_coord_val paks_h = paks.get_max_size().h;
	add_component(&paks);

	new_component<gui_label_t>( "The following graphics are unmaintained:" );

	for (int i = 10; i < PAKSET_COUNT; i++) {
		obsolete_paks.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(pakinfo[i*2+1], i<10?SYSCOL_TEXT: SYSCOL_TEXT_SHADOW);
	}
	obsolete_paks.enable_multiple_selection();
	scr_coord_val obsolete_paks_h = obsolete_paks.get_max_size().h;
	add_component(&obsolete_paks);

	install.init(button_t::roundbox_state, "Install");
	add_component(&install);
	install.add_listener(this);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize()+scr_size(0,paks_h-paks.get_size().h + obsolete_paks_h- obsolete_paks.get_size().h));
}


/**
 * This method is called if an action is triggered
 */
bool pakinstaller_t::action_triggered(gui_action_creator_t*, value_t)
{
	// now install
	dr_chdir( env_t::data_dir );
	FOR(vector_tpl<sint32>, i, paks.get_selections()) {
		cbuffer_t param;
#ifdef _WIN32
		param.append( "powershell .\\get_pak.ps1 \"" );
#else
		param.append( "./get_pak.sh \"" );
#endif
		param.append(pakinfo[i*2]);
		param.append("\"");

		const int retval = system( param );
		dbg->debug("pakinstaller_t::action_triggered", "Command '%s' returned %d", param.get_str(), retval);
	}

	FOR(vector_tpl<sint32>, i, obsolete_paks.get_selections()) {
		cbuffer_t param;
#ifdef _WIN32
		param.append( "powershell .\\get_pak.ps1 \"" );
#else
		param.append( "./get_pak.sh \"" );
#endif
		param.append(pakinfo[i*2+20]);
		param.append("\"");

		const int retval = system( param );
		dbg->debug("pakinstaller_t::action_triggered", "Command '%s' returned %d", param.get_str(), retval);
	}

	finish_install = true;
	return false;
}
