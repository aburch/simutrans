#include "player_merge_frame.h"
#include "components/gui_divider.h"
#include "../dataobj/translator.h"
#include "../player/simplay.h"
#include "../simmenu.h"
#include "../simworld.h"

player_merge_frame_t::player_merge_frame_t() :
	gui_frame_t( translator::translate("Player merger") )
{  
	selected_merged_player_num = selected_merger_player_num = -1;
	
  set_table_layout(1,0);
  
  lb_description.set_text("Select players to be merged.");
  add_component(&lb_description);
	
	add_table(2,0);
  
  lb_merged.set_text("merged player");
  add_component(&lb_merged);
  lb_merge_to.set_text("merge to");
  add_component(&lb_merge_to);
  for(  uint8 i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
    player_t* player = world()->get_player(i);
    if(  player==NULL || player->is_public_service()  ) {
      // empty player or public player, skip.
      continue;
    }
    
    button_t* bt_merged_player = new button_t();
    bt_merged_player->init(button_t::roundbox_state | button_t::flexible, player->get_name());
		bt_merged_player->pressed = false;
    bt_merged_player->add_listener(this);
		add_component(bt_merged_player);
		player_item_t merged_player_item;
		merged_player_item.button = bt_merged_player;
		merged_player_item.player_num = i;
    players_merged.append(merged_player_item);
    
    button_t* bt_to_player = new button_t();
    bt_to_player->init(button_t::roundbox_state | button_t::flexible, player->get_name());
		bt_to_player->pressed = false;
    bt_to_player->add_listener(this);
    bt_to_player->disable();
		add_component(bt_to_player);
		player_item_t to_player_item;
		to_player_item.button = bt_to_player;
		to_player_item.player_num = i;
    players_merge_to.append(to_player_item);
  }
	end_table();
	
	new_component<gui_divider_t>();
	bt_merge.init(button_t::roundbox_state | button_t::flexible, translator::translate("Merge"));
	bt_merge.add_listener(this);
	bt_merge.disable();
	add_component(&bt_merge);
  
  reset_min_windowsize();
  set_windowsize(get_min_windowsize());
}

bool player_merge_frame_t::action_triggered(gui_action_creator_t* comp, value_t) {
	if(  comp == &bt_merge  ) {
		if(  selected_merged_player_num==-1  ||  selected_merger_player_num==-1  ) {
			// should not happen...
			return true;
		}
		
		static char param[128];
		snprintf( param, 128, "%hhi,%hhi", selected_merged_player_num, selected_merger_player_num );
		tool_t::simple_tool[TOOL_MERGE_PLAYER]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_MERGE_PLAYER], welt->get_active_player() );
		destroy_win(this);
		return true;
	}
	
	// enable "merge_to" buttons when the merged players is selected.
	FOR(vector_tpl<player_item_t>, const item, players_merged) {
		if(  comp != item.button  ) { continue; }
		FOR(vector_tpl<player_item_t>, const i_item, players_merged) {
			i_item.button->pressed = (i_item.button == item.button);
		}
		FOR(vector_tpl<player_item_t>, const merge_to_item, players_merge_to) {
			if(  merge_to_item.player_num == item.player_num  ) {
				merge_to_item.button->disable();
			} else {
				merge_to_item.button->enable();
			}
		}
		selected_merged_player_num = item.player_num;
		selected_merger_player_num = -1;
		return true;
	}
	
	// enable the "merge" button when the merger player is selected.
	FOR(vector_tpl<player_item_t>, const item, players_merge_to) {
		if(  comp != item.button  ) { continue; }
		FOR(vector_tpl<player_item_t>, const i_item, players_merge_to) {
			i_item.button->pressed = (i_item.button == item.button);
		}
		bt_merge.enable();
		selected_merger_player_num = item.player_num;
		return true;
	}
	
  return true;
}
