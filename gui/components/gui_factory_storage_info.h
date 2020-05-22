/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_FACTORY_STORAGE_INFO_H
#define GUI_FACTORY_STORAGE_INFO_H

#include "gui_container.h"
#include "gui_scrollpane.h"
#include "gui_speedbar.h"

#include "../../simfab.h"
#include "../simwin.h"

class fabrik_t;

// display factory storage info
class gui_factory_storage_info_t : public gui_container_t
{
private:
	fabrik_t *fab;

public:
	gui_factory_storage_info_t(fabrik_t *factory);

	void set_fab(fabrik_t *f) { fab = f; }

	void draw(scr_coord offset);
};

#endif
