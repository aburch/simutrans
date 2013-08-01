#include "gui_map_preview.h"
#include "../../simworld.h"

gui_map_preview_t::gui_map_preview_t(void) :
	gui_komponente_t()
{
	map_data = NULL;
	map_size = koord(0,0);
	set_groesse (koord( MAP_PREVIEW_SIZE_X,MAP_PREVIEW_SIZE_Y ));
}
