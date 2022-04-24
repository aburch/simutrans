/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../utils/cbuffer.h"

#include "pakinstaller.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../sys/simsys.h"
#include "../world/simworld.h"

#include "../dataobj/pakset_downloader.h"
#include "../simloadingscreen.h"

#include "../../paksetinfo.h"


bool pakinstaller_t::finish_install;


pakinstaller_t::pakinstaller_t() :
	gui_frame_t(translator::translate("Install graphics")),
	paks(gui_scrolled_list_t::listskin),
	obsolete_paks(gui_scrolled_list_t::listskin)
{
	finish_install = false;

	set_table_layout(2, 0);

	new_component_span<gui_label_t>( "Select one or more graphics to install (Ctrl+click):", 2 );

	for (int i = 0; i < OBSOLETE_FROM; i++) {
		if (pak_can_download(pakinfo + i)) {
			paklist.append(pakinfo + i);
			paks.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(pakinfo[i].name, SYSCOL_TEXT);
			paks.get_element(paks.get_count()-1)->selected = pak_need_update(pakinfo + i);
		}
	}
	paks.enable_multiple_selection();
	scr_coord_val paks_h = paks.get_max_size().h;
	add_component(&paks,2);

	new_component_span<gui_label_t>( "The following graphics are unmaintained:", 2 );

	for (int i = OBSOLETE_FROM; i < PAKSET_COUNT; i++) {
		if (pak_can_download(pakinfo + i)) {
			paklist.append(pakinfo + i);
			obsolete_paks.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(pakinfo[i].name, SYSCOL_TEXT_SHADOW);
			obsolete_paks.get_element(obsolete_paks.get_count()-1)->selected = pak_need_update(pakinfo + i);
		}
	}
	obsolete_paks.enable_multiple_selection();
	scr_coord_val obsolete_paks_h = obsolete_paks.get_max_size().h;
	add_component(&obsolete_paks,2);

	bool is_world = world();

	install.init(button_t::roundbox_state | button_t::flexible, "Install");
	add_component(&install,is_world+1);
	install.add_listener(this);

	if( !is_world ) {
		cancel.init( button_t::roundbox_state | button_t::flexible, "Cancel" );
		cancel.add_listener( this );
		add_component( &cancel );
	}

	reset_min_windowsize();
	set_windowsize(get_min_windowsize()+scr_size(0,paks_h-paks.get_size().h + obsolete_paks_h- obsolete_paks.get_size().h));
}


// compile list of paks to be installed by pakset_downloader
bool pakinstaller_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if (comp == &cancel) {
		finish_install = true;
		return false;
	}

	// now install
	dr_chdir( env_t::install_dir );
	vector_tpl<paksetinfo_t*>install_paks;

	for (sint32 i : paks.get_selections()) {
		install_paks.append(paklist[i]);
	}
	for (sint32 i : obsolete_paks.get_selections()) {
		install_paks.append(paklist[i+paks.get_count()]);
	}

	pak_download(install_paks);

	finish_install = true;
	return false;
}
