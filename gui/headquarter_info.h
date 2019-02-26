#ifndef headquarter_info_h
#define headquarter_info_h


#include "base_info.h"
#include "components/gui_button.h"
#include "components/action_listener.h"
#include "components/gui_location_view_t.h"

#include "../utils/cbuffer_t.h"

class gebaeude_t;
class player_t;

class headquarter_info_t : public base_infowin_t, private action_listener_t
{
	button_t upgrade;
	cbuffer_t headquarter_tooltip;
	player_t *player;
	location_view_t headquarter_view;

	void update();
public:
	headquarter_info_t(player_t* player);

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t *comp, value_t extra) OVERRIDE;
};

#endif
