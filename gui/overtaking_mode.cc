/*
 * Dialogue to set the overtaking_mode, when CTRL+clicking a way on toolbar
 * Used by tool_roadsign_t
 */

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_divider.h"
#include "components/gui_numberinput.h"
#include "components/action_listener.h"

#include "overtaking_mode.h"
#include "../simtool.h"

#define L_DIALOG_WIDTH (200)

overtaking_mode_t overtaking_mode_frame_t::overtaking_mode = twoway_mode;
char overtaking_mode_frame_t::mode_name[6][20] = {"halt mode", "oneway", "twoway", "only loading convoi", "prohibited", "inverted"};

overtaking_mode_frame_t::overtaking_mode_frame_t(player_t *player_, tool_build_way_t* tool_, bool show_avoid_cityroad) :
	gui_frame_t( translator::translate("Road Configuration") )
{
	tool_class = 0;
	tool_w = tool_;
	init(player_, tool_w->get_overtaking_mode(), tool_w->get_street_flag(), show_avoid_cityroad);
}

overtaking_mode_frame_t::overtaking_mode_frame_t(player_t *player_, tool_build_bridge_t* tool_) :
	gui_frame_t( translator::translate("Road Configuration") )
{
	tool_class = 1;
	tool_b = tool_;
	init(player_, tool_b->get_overtaking_mode(), tool_b->get_street_flag(), false);
}

overtaking_mode_frame_t::overtaking_mode_frame_t(player_t *player_, tool_build_tunnel_t* tool_) :
	gui_frame_t( translator::translate("Road Configuration") )
{
	tool_class = 2;
	tool_tu = tool_;
	init(player_, tool_tu->get_overtaking_mode(), tool_tu->get_street_flag(), false);
}

void overtaking_mode_frame_t::init( player_t* player_, overtaking_mode_t overtaking_mode_, uint8 street_flag_, bool show_avoid_cityroad) {
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

	divider[0].set_pos( cursor );
	divider[0].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	add_component(&divider[0]);
	cursor.y += D_DIVIDER_HEIGHT;
	
	if(  tool_class==0  &&  show_avoid_cityroad  ) {
		avoid_cityroad_button.init( button_t::square_state, "avoid becoming cityroad", cursor );
		avoid_cityroad_button.set_width( L_DIALOG_WIDTH - D_MARGINS_X );
		avoid_cityroad_button.add_listener(this);
		avoid_cityroad_button.pressed = street_flag_&strasse_t::AVOID_CITYROAD;
		add_component(&avoid_cityroad_button);
		cursor.y += avoid_cityroad_button.get_size().h + D_V_SPACE;
	}
	
	citycar_no_entry_button.init( button_t::square_state, "citycars do not enter", cursor );
	citycar_no_entry_button.set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	citycar_no_entry_button.add_listener(this);
	citycar_no_entry_button.pressed = street_flag_&strasse_t::CITYCAR_NO_ENTRY;
	add_component(&citycar_no_entry_button);
	cursor.y += citycar_no_entry_button.get_size().h + D_V_SPACE;
	
	if(  tool_class==0  &&  !show_avoid_cityroad  ) {
		// the way is elevated.
		divider[1].set_pos( cursor );
		divider[1].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
		add_component(&divider[1]);
		cursor.y += D_DIVIDER_HEIGHT;
		
		// TODO: set value and limit correctly!
		height_offset.set_pos( cursor );
		height_offset.set_size( scr_size(52, D_EDIT_HEIGHT) );
		height_offset.set_limits( 0, 32 );
		height_offset.set_value( tool_w->get_height_offset() );
		height_offset.wrap_mode( false );
		height_offset.add_listener( this );
		add_component( &height_offset );
		cursor.y += height_offset.get_size().h + D_V_SPACE;
	}
	
	set_windowsize( scr_size( L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM ) );
}

bool overtaking_mode_frame_t::action_triggered( gui_action_creator_t *komp, value_t)
{
	sint8 num = -1;
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
	}else if(  komp==&avoid_cityroad_button  ) {
		avoid_cityroad_button.pressed = !(avoid_cityroad_button.pressed);
	}
	else if(  komp==&citycar_no_entry_button  ) {
		citycar_no_entry_button.pressed = !(citycar_no_entry_button.pressed);
	}
	else if(  komp==&height_offset  ) {
		if(  tool_class==0  ) {
			tool_w->set_height_offset(height_offset.get_value());
		}
	}
	else{
		return false;
	}
	if(num!=-1) {
		for(int i = 0; i < 6; i++){
			if(  num==i  ){
				mode_button[i].pressed = true;
			}else{
				mode_button[i].pressed = false;
			}
		}
	}
	uint8 flag = 0;
	if(  avoid_cityroad_button.pressed  ) { flag |= strasse_t::AVOID_CITYROAD; }
	if(  citycar_no_entry_button.pressed  ) { flag |= strasse_t::CITYCAR_NO_ENTRY; }
	switch(  tool_class  ) {
		case 0:
		tool_w->set_overtaking_mode(overtaking_mode);
		tool_w->set_street_flag(flag);
		break;
		case 1:
		tool_b->set_overtaking_mode(overtaking_mode);
		tool_b->set_street_flag(flag);
		break;
		case 2:
		tool_tu->set_overtaking_mode(overtaking_mode);
		tool_tu->set_street_flag(flag);
		break;
		default:
		dbg->fatal("overtaking_mode_frame_t::action_triggered()", "Illegal tool_class");
	}
	return true;
}
