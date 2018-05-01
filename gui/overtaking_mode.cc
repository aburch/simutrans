/*
 * Dialogue to set the overtaking_mode, when CTRL+clicking a way on toolbar
 * Used by tool_roadsign_t
 */

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"

#include "overtaking_mode.h"
#include "../simtool.h"

#define L_DIALOG_WIDTH (200)

overtaking_mode_t overtaking_mode_frame_t::overtaking_mode = twoway_mode;
char overtaking_mode_frame_t::mode_name[6][20] = {"halt mode", "oneway", "twoway", "only loading convoi", "prohibited", "inverted"};

overtaking_mode_frame_t::overtaking_mode_frame_t(player_t *player_, tool_build_way_t* tool_) :
	gui_frame_t( translator::translate("set overtaking mode") )
{
	tool_class = 0;
	tool_w = tool_;
	init(player_, tool_w->get_overtaking_mode());
}

overtaking_mode_frame_t::overtaking_mode_frame_t(player_t *player_, tool_build_bridge_t* tool_) :
	gui_frame_t( translator::translate("set overtaking mode") )
{
	tool_class = 1;
	tool_b = tool_;
	init(player_, tool_b->get_overtaking_mode());
}

overtaking_mode_frame_t::overtaking_mode_frame_t(player_t *player_, tool_build_tunnel_t* tool_) :
	gui_frame_t( translator::translate("set overtaking mode") )
{
	tool_class = 2;
	tool_tu = tool_;
	init(player_, tool_tu->get_overtaking_mode());
}

void overtaking_mode_frame_t::init( player_t* player_, overtaking_mode_t overtaking_mode_ ) {
	player = player_;
	overtaking_mode = overtaking_mode_;

	scr_coord cursor(D_MARGIN_LEFT, D_MARGIN_TOP);

	for(int i = 0 ; i < 6; i++){
		mode_button[i].init( button_t::square_state, mode_name[i], cursor );
		mode_button[i].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
		mode_button[i].add_listener(this);
		mode_button[i].pressed = false;
		add_component( &mode_button[i] );
		cursor.y += mode_button[i].get_size().h + D_V_SPACE;
	}

	if(  overtaking_mode==halt_mode          ) mode_button[0].pressed = true;
	if(  overtaking_mode==oneway_mode        ) mode_button[1].pressed = true;
	if(  overtaking_mode==twoway_mode        ) mode_button[2].pressed = true;
	if(  overtaking_mode==loading_only_mode  ) mode_button[3].pressed = true;
	if(  overtaking_mode==prohibited_mode    ) mode_button[4].pressed = true;
	if(  overtaking_mode==inverted_mode      ) mode_button[5].pressed = true;

	set_windowsize( scr_size( L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM ) );
}

bool overtaking_mode_frame_t::action_triggered( gui_action_creator_t *komp, value_t)
{
	uint8 num = 0;
	if(  komp==&mode_button[0]  ) {
		overtaking_mode = halt_mode;
		num = 0;
	}else if(  komp==&mode_button[1]  ) {
		overtaking_mode = oneway_mode;
		num = 1;
	}else if(  komp==&mode_button[2]  ) {
		overtaking_mode = twoway_mode;
		num = 2;
	}else if(  komp==&mode_button[3]  ) {
		overtaking_mode = loading_only_mode;
		num = 3;
	}else if(  komp==&mode_button[4]  ) {
		overtaking_mode = prohibited_mode;
		num = 4;
	}else if(  komp==&mode_button[5]  ) {
		overtaking_mode = inverted_mode;
		num = 5;
	}else{
		return false;
	}
	for(int i = 0; i < 6; i++){
		if(  num==i  ){
			mode_button[i].pressed = true;
		}else{
			mode_button[i].pressed = false;
		}
	}
	switch(  tool_class  ) {
		case 0:
		tool_w->set_overtaking_mode(overtaking_mode);
		break;
		case 1:
		tool_b->set_overtaking_mode(overtaking_mode);
		break;
		case 2:
		tool_tu->set_overtaking_mode(overtaking_mode);
		break;
		default:
		assert(false);
	}
	return true;
}
