/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "replace_frame.h"

#include "../simcolor.h"
#include "simwin.h"
#include "../simworld.h"
#include "../player/simplay.h"
#include "../player/finance.h"
#include "../simline.h"

#include "../dataobj/translator.h"
#include "../dataobj/replace_data.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"
#include "../vehicle/simvehicle.h"

static bool _is_electrified(const karte_t* welt, const convoihandle_t& cnv)
{
	vehicle_t *veh = cnv->get_vehicle(0);
	grund_t* gr = welt->lookup(veh->get_pos());
	weg_t* way = gr->get_weg(veh->get_waytype());
	return way ? way->is_electrified() : false;
}

replace_frame_t::replace_frame_t(convoihandle_t cnv, const char *name):
	gui_frame_t(translator::translate("Replace"), cnv->get_owner()),
	cnv(cnv),
	replace_line(false), replace_all(false), depot(false),
	state(state_replace), replaced_so_far(0),
	lb_convoy(cnv, true, true),
	lb_to_be_replaced(NULL, SYSCOL_TEXT, gui_label_t::centered),
	lb_money(NULL, SYSCOL_TEXT, gui_label_t::money),
	lb_replace_cycle(NULL, SYSCOL_TEXT, gui_label_t::right),
	lb_replace(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_sell(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_skip(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_n_replace(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_n_sell(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_n_skip(NULL, SYSCOL_TEXT, gui_label_t::left),
	convoy_assembler(
		cnv->get_vehicle(0)->get_desc()->get_waytype(),  
		cnv->get_owner()->get_player_nr(), 
		_is_electrified(welt, cnv))
{	
	const uint32 a_button_height = 14;
	const uint32 margin = 6;
	txt_money[0] = 0;
	lb_money.set_text_pointer(txt_money);
	add_component(&lb_money);
	
	lb_convoy.set_text_pointer(name);
	add_component(&lb_convoy);

	lb_to_be_replaced.set_text_pointer(translator::translate("To be replaced by:"));
	add_component(&lb_to_be_replaced);

	lb_replace_cycle.set_text_pointer(translator::translate("Replace cycle:"));
	lb_replace.set_text_pointer(translator::translate("rpl_cnv_replace"));
	lb_sell.set_text_pointer(translator::translate("rpl_cnv_sell"));
	lb_skip.set_text_pointer(translator::translate("rpl_cnv_skip"));
	numinp[state_replace].set_value( 1 );
	numinp[state_replace].set_limits( 0, 999 );
	numinp[state_replace].set_increment_mode( 1 );
	numinp[state_replace].add_listener(this);
	numinp[state_sell].set_value( 0 );
	numinp[state_sell].set_limits( 0, 999 );
	numinp[state_sell].set_increment_mode( 1 );
	numinp[state_sell].add_listener(this);
	numinp[state_skip].set_value( 0 );
	numinp[state_skip].set_limits( 0, 999 );
	numinp[state_skip].set_increment_mode( 1 );
	numinp[state_skip].add_listener(this);
	txt_n_replace[0] = 0;
	txt_n_sell[0] = 0;
	txt_n_skip[0] = 0;
	lb_n_replace.set_text_pointer(txt_n_replace);
	lb_n_sell.set_text_pointer(txt_n_sell);
	lb_n_skip.set_text_pointer(txt_n_skip);
	add_component(&lb_replace_cycle);
	add_component(&lb_replace);
	add_component(&numinp[state_replace]);
	add_component(&lb_n_replace);
	add_component(&lb_sell);
	add_component(&numinp[state_sell]);
	add_component(&lb_n_sell);
	add_component(&lb_skip);
	add_component(&numinp[state_skip]);
	add_component(&lb_n_skip);

	const vehicle_t *lead_vehicle = cnv->get_vehicle(0);
	const waytype_t wt = lead_vehicle->get_waytype();
	const weg_t *way = welt->lookup(lead_vehicle->get_pos())->get_weg(wt);
	const bool weg_electrified = way == NULL ? false : way->is_electrified();
	convoy_assembler.set_electrified( weg_electrified );
	convoy_assembler.set_convoy_tabs_skip(-2*LINESPACE+3*LINESPACE+2*margin+a_button_height);
	convoy_assembler.add_listener(this);
	if(cnv.is_bound() && cnv->get_replace())
	{
		cnv->get_replace()->check_contained(cnv);
		convoy_assembler.set_vehicles(cnv->get_replace()->get_replacing_vehicles());
	}
	else
	{
		vector_tpl<const vehicle_desc_t*> *existing_vehicles = new vector_tpl<const vehicle_desc_t*>();
		uint8 count = cnv->get_vehicle_count();
		for(uint8 i = 0; i < count; i ++)
		{
			existing_vehicles->append(cnv->get_vehicle(i)->get_desc());
		}
		convoy_assembler.set_vehicles(existing_vehicles);
	}

	bt_replace_line.set_typ(button_t::square);
	bt_replace_line.set_text("replace all in line");
	bt_replace_line.set_tooltip("Replace all convoys like this belonging to this line");
	bt_replace_line.add_listener(this);
	add_component(&bt_replace_line);

	bt_replace_all.set_typ(button_t::square);
	bt_replace_all.set_text("replace all");
	bt_replace_all.set_tooltip("Replace all convoys like this");
	bt_replace_all.add_listener(this);
	add_component(&bt_replace_all);

	bt_autostart.set_typ(button_t::roundbox);
	bt_autostart.set_text("Full replace");
	bt_autostart.set_tooltip("Send convoy to depot, replace and restart it automatically");
	bt_autostart.add_listener(this);
	add_component(&bt_autostart);

	bt_clear.set_typ(button_t::roundbox);
	bt_clear.set_text("Clear");
	bt_clear.set_tooltip("Reset this replacing operation");
	bt_clear.add_listener(this);
	add_component(&bt_clear);

	bt_depot.set_typ(button_t::roundbox);
	bt_depot.set_text("Replace but stay");
	bt_depot.set_tooltip("Send convoy to depot, replace it and stay there");
	bt_depot.add_listener(this);
	add_component(&bt_depot);

	bt_mark.set_typ(button_t::roundbox);
	bt_mark.set_text("Mark for replacing");
	bt_mark.set_tooltip("Mark for replacing. The convoy will replace when manually sent to depot");
	bt_mark.add_listener(this);
	add_component(&bt_mark);

	bt_retain_in_depot.set_typ(button_t::square);
	bt_retain_in_depot.set_text("Retain in depot");
	bt_retain_in_depot.set_tooltip("Keep replaced vehicles in the depot for future use rather than selling or upgrading them.");
	bt_retain_in_depot.add_listener(this);
	add_component(&bt_retain_in_depot);

	bt_use_home_depot.set_typ(button_t::square);
	bt_use_home_depot.set_text("Use home depot");
	bt_use_home_depot.set_tooltip("Send the convoy to its home depot for replacing rather than the nearest depot.");
	bt_use_home_depot.add_listener(this);
	add_component(&bt_use_home_depot);

	bt_allow_using_existing_vehicles.set_typ(button_t::square);
	bt_allow_using_existing_vehicles.set_text("Use existing vehicles");
	bt_allow_using_existing_vehicles.set_tooltip("Use any vehicles already present in the depot, if available, instead of buying new ones or upgrading.");
	bt_allow_using_existing_vehicles.add_listener(this);
	add_component(&bt_allow_using_existing_vehicles);
	/* This must be *last* of add_component */
	add_component(&convoy_assembler);

	rpl = cnv->get_replace() ? new replace_data_t(cnv->get_replace()) : new replace_data_t();

	scr_size gr(0,0);
	layout(&gr);
	update_data();
	gui_frame_t::set_windowsize(gr);

	// Hajo: Trigger layouting
	set_resizemode(diagonal_resize);

	convoy_assembler.set_replace_frame(this);

	copy = false;
}


void replace_frame_t::update_total_height(uint32 height)
{
	total_height+=height;
	min_total_height+=height;
}


void replace_frame_t::update_total_width(uint32 width)
{
	total_width=max(total_width,width);
	min_total_width=max(min_total_width,width);
}


void replace_frame_t::layout(scr_size *gr)
{
	const uint32 margin=6;
	const uint32 a_button_width=96;
	const uint32 a_D_BUTTON_HEIGHT=14;

	/**
	 * Let's calculate the space and min space
	 */
	scr_size fgr = (gr!=NULL)? *gr : get_windowsize();
	min_total_width=0;
	total_width=fgr.w;
	total_height=margin;
	min_total_height=total_height;

	// Width at least to see labels ok
	update_total_width(400);

	// Convoy label: name+image+specs
	scr_size img_size=lb_convoy.get_size();
	update_total_width(img_size.w);
	update_total_height(img_size.h);

	// Label to be replaced
	update_total_height(LINESPACE);

	// 3 buttons
	update_total_width(2*margin+3*a_button_width);
	// No update height needed, convoy assembler

	// Rest of the vertical space, if any, for convoy_assembler
	update_total_width(convoy_assembler.get_convoy_image_width());
	convoy_assembler.set_panel_rows(gr  &&  gr->h==0?-1:fgr.h-total_height);
	total_height+=convoy_assembler.get_height();
	min_total_height+=convoy_assembler.get_min_height();

	set_min_windowsize(scr_size(min_total_width, min_total_height));
	if(fgr.w<0 || (uint32)fgr.w<total_width) {
		gui_frame_t::set_windowsize(scr_size(min_total_width, max(fgr.h,min_total_height) ));
	}
	if(gr  &&  gr->w==0) {
		gr->w = total_width;
	}
	if(gr  &&  gr->h==0) {
		gr->h = total_height;
	}

	/**
	 * Now do the layout
	 */
	uint32 current_y = margin;
	if (gr) {
		fgr = *gr;
	} else {
		fgr = scr_size(total_width,total_height);
	}

	lb_convoy.set_pos(scr_coord(fgr.w/2,current_y));
	current_y+=lb_convoy.get_size().h;

	lb_to_be_replaced.set_pos(scr_coord(fgr.w/2,current_y));
	current_y+=LINESPACE;

	convoy_assembler.set_pos(scr_coord(0,current_y));
	convoy_assembler.set_size(scr_size(fgr.w,convoy_assembler.get_height()));
	convoy_assembler.layout();

	uint32 buttons_y = current_y + convoy_assembler.get_convoy_height() - LINESPACE*2 + 24;
	uint32 buttons_width=(fgr.w-2*margin)/4;
	bt_autostart.set_size(scr_size(buttons_width, a_D_BUTTON_HEIGHT));
	bt_depot.set_size(scr_size(buttons_width, a_D_BUTTON_HEIGHT));
	bt_mark.set_size(scr_size(buttons_width, a_D_BUTTON_HEIGHT));
	bt_clear.set_size(scr_size(buttons_width, a_D_BUTTON_HEIGHT));
	bt_autostart.set_pos(scr_coord(margin,buttons_y));
	bt_depot.set_pos(scr_coord(margin+buttons_width,buttons_y));
	bt_mark.set_pos(scr_coord(margin+(buttons_width*2),buttons_y));
	bt_clear.set_pos(scr_coord(margin+(buttons_width*3),buttons_y));
	
	current_y=buttons_y+a_D_BUTTON_HEIGHT+margin;
	lb_money.set_pos(scr_coord(margin + (203 *2),current_y));
	lb_replace_cycle.set_pos(scr_coord(fgr.w-270,current_y));
	lb_replace.set_pos(scr_coord(fgr.w-166,current_y));

	numinp[state_replace].set_pos(scr_coord( fgr.w-110, current_y ) );
	numinp[state_replace].set_size(scr_size( 50, a_D_BUTTON_HEIGHT ) );
	lb_n_replace.set_pos(scr_coord( fgr.w-50, current_y ) );

	bt_replace_line.set_pos(scr_coord(margin,current_y));
	bt_retain_in_depot.set_pos(scr_coord(margin + 162,current_y));
	
	current_y+=LINESPACE+2;

	bt_allow_using_existing_vehicles.set_pos(scr_coord(margin + (162 *2),current_y));
	bt_replace_all.set_pos(scr_coord(margin,current_y));
	bt_use_home_depot.set_pos(scr_coord(margin + 162,current_y));
	numinp[state_sell].set_pos(scr_coord( fgr.w-110, current_y ) );
	numinp[state_sell].set_size(scr_size( 50, a_D_BUTTON_HEIGHT ) );
	lb_n_sell.set_pos(scr_coord( fgr.w-50, current_y ) );
	lb_sell.set_pos(scr_coord(fgr.w-166,current_y));
	current_y+=LINESPACE+2;
	lb_skip.set_pos(scr_coord(fgr.w-166,current_y));
	numinp[state_skip].set_pos(scr_coord( fgr.w-110, current_y ) );
	numinp[state_skip].set_size(scr_size( 50, a_D_BUTTON_HEIGHT ) );
	lb_n_skip.set_pos(scr_coord( fgr.w-50, current_y ) );

	current_y+=LINESPACE+margin;
}


void replace_frame_t::set_windowsize( scr_size gr )
{
	scr_size g=gr;
	layout(&g);
	update_data();
	gui_frame_t::set_windowsize(gr);
}


void replace_frame_t::update_data()
{
	convoy_assembler.update_data();

	txt_n_replace[0]='\0';
	txt_n_sell[0]='\0';
	txt_n_skip[0]='\0';
	uint32 n[3];
	n[0]=0;
	n[1]=0;
	n[2]=0;
	money = 0;
	sint64 base_total_cost = calc_total_cost();
	if (replace_line || replace_all) {
		start_replacing();
	} else {
		money -= base_total_cost;
	}
	if (replace_line) {
		linehandle_t line=cnv.is_bound()?cnv->get_line():linehandle_t();
		if (line.is_bound()) {
			for (uint32 i=0; i<line->count_convoys(); i++)
			{
				convoihandle_t cnv_aux=line->get_convoy(i);
				if (cnv->has_same_vehicles(cnv_aux))
				{
					uint8 present_state=get_present_state();
					if (present_state==(uint8)(-1))
					{
						continue;
					}

					money -= base_total_cost;
					n[present_state]++;
				}
			}
		}
	} else if (replace_all) {
		for (uint32 i=0; i<welt->convoys().get_count(); i++) {
			convoihandle_t cnv_aux=welt->convoys()[i];
			if (cnv_aux.is_bound() && cnv_aux->get_owner()==cnv->get_owner() && cnv->has_same_vehicles(cnv_aux)) 
			{
				uint8 present_state=get_present_state();
				if (present_state==(uint8)(-1))
				{
					continue;
				}

				money -= base_total_cost;
				n[present_state]++;
			}
		}
	}
	if (replace_all || replace_line) {
		sprintf(txt_n_replace,"%d",n[0]);
		sprintf(txt_n_sell,"%d",n[1]);
		sprintf(txt_n_skip,"%d",n[2]);
	}
	if (convoy_assembler.get_vehicles()->get_count()>0) {
		money_to_string(txt_money,money/100.0);
		lb_money.set_color(money>=0?MONEY_PLUS:MONEY_MINUS);
	} else {
		txt_money[0]='\0';
	}

}


uint8 replace_frame_t::get_present_state() {
	if (numinp[state_replace].get_value()==0 && numinp[state_sell].get_value()==0 && numinp[state_skip].get_value()==0) {
		return (uint8)(-1);
	}
	for (uint8 i=0; i<n_states; ++i) {
		if (replaced_so_far>=numinp[state].get_value()) {
			replaced_so_far=0;
			state=(state+1)%n_states;
		} else {
			break;
		}
	}
	replaced_so_far++;
		return state;
	}


bool replace_frame_t::replace_convoy(convoihandle_t cnv_rpl, bool mark)
{
	uint8 state=get_present_state();
	if (!cnv_rpl.is_bound() || cnv_rpl->in_depot() || state==(uint8)(-1)) 
	{
		return false;
	}

	switch (state) 
	{
	case state_replace:
	{
		if(convoy_assembler.get_vehicles()->get_count()==0)
		{
			break;
		}

		if(!welt->get_active_player()->can_afford(0 - money))
		{
			const char *err = NOTICE_INSUFFICIENT_FUNDS;
			news_img *box = new news_img(err);
			create_win(box, w_time_delete, magic_none);
			break;
		}

		if(!copy)
		{
			rpl->set_replacing_vehicles(convoy_assembler.get_vehicles());

			cbuffer_t buf;
			rpl->sprintf_replace(buf);
			
			cnv_rpl->call_convoi_tool('R', buf);
		}

		else
		{
			cbuffer_t buf;
			buf.append((long)master_convoy.get_id());
			cnv_rpl->call_convoi_tool('C', buf);
		}

		if(!mark && depot && !rpl->get_autostart())
		{
			cnv_rpl->call_convoi_tool('D', NULL);
		}

	}
		break;
	
	case state_sell:
		cnv_rpl->call_convoi_tool('T');
		break;
	case state_skip:
	break;
	}

	replaced_so_far++;
	return true;
}

bool replace_frame_t::action_triggered( gui_action_creator_t *comp,value_t /*p*/)
{
	if(comp != NULL) 
	{	// message from outside!
		if(comp == &bt_replace_line) 
		{
			replace_line =! replace_line;
			replace_all = false;
		}
		else if(comp == &bt_replace_all) 
		{
			replace_all =! replace_all;
			replace_line = false;
		}
		
		else if(comp == &bt_retain_in_depot) 
		{
			rpl->set_retain_in_depot(!rpl->get_retain_in_depot());
		}

		else if(comp == &bt_use_home_depot) 
		{
			rpl->set_use_home_depot(!rpl->get_use_home_depot());
		}

		else if(comp == &bt_allow_using_existing_vehicles) 
		{
			rpl->set_allow_using_existing_vehicles(!rpl->get_allow_using_existing_vehicles());
		}

		else if(comp == &bt_clear) 
		{
			cnv->call_convoi_tool('X', NULL);
			rpl = new replace_data_t();
			convoy_assembler.clear_convoy();
		}

		else if(comp==&bt_autostart || comp== &bt_depot || comp == &bt_mark) 
		{
			depot=(comp==&bt_depot);
			rpl->set_autostart((comp==&bt_autostart));

			start_replacing();
			if (!replace_line && !replace_all) 
			{
				replace_convoy(cnv, comp == &bt_mark);
			} 
			else if (replace_line) 
			{
				linehandle_t line = cnv.is_bound() ? cnv->get_line() : linehandle_t();
				if (line.is_bound()) 
				{
					bool first_success = false;
					for (uint32 i = 0; i < line->count_convoys(); i++) 
					{
						convoihandle_t cnv_aux = line->get_convoy(i);
						if (cnv->has_same_vehicles(cnv_aux))
						{
							first_success = replace_convoy(cnv_aux, comp == &bt_mark);
							if(copy == false)
							{
								master_convoy = cnv_aux;
							}
							if(first_success)
							{
								copy = true;
							}
						}
					}
				}
				else
				{
					replace_convoy(cnv, comp == &bt_mark);
				}
			} 
			else if (replace_all) 
			{
				bool first_success = false;
				for (uint32 i=0; i<welt->convoys().get_count(); i++) 
				{
					convoihandle_t cnv_aux=welt->convoys()[i];
					if (cnv_aux.is_bound() && cnv_aux->get_owner()==cnv->get_owner() && cnv->has_same_vehicles(cnv_aux)) 
					{
						first_success = replace_convoy(cnv_aux, comp == &bt_mark);
						if(copy == false)
						{
							master_convoy = cnv_aux;
						}
						if(first_success)
						{
							copy = true;
						}
					}
				}
			}
#ifndef DEBUG
			// FIXME: Oddly, this line causes crashes in 10.13 and over when
			// the replace window is closed automatically with "full replace".
			// The difficulty appears to relate to the objects comprising the
			// window being destroyed before they have finished being used by
			// the GUI system leading to access violations.
			destroy_win(this);
#endif
			copy = false;
			return true;
		}
	}
	convoy_assembler.build_vehicle_lists();
	update_data();
	layout(NULL);
	copy = false;
	return true;
}


bool replace_frame_t::infowin_event(const event_t *ev)
{
	gui_frame_t::infowin_event(ev);
	//if(IS_WINDOW_REZOOM(ev)) {
	//	koord gr = get_fenstergroesse();
	//	set_fenstergroesse(gr);
	//	return true;
	//} else 
	if(ev->ev_class == INFOWIN && ev->ev_code == WIN_OPEN) {
		convoy_assembler.build_vehicle_lists();
		update_data();
		layout(NULL);
		return true;
	}
	return false;
}


void replace_frame_t::draw(scr_coord pos, scr_size size)
{
	if (welt->get_active_player() != cnv->get_owner()) {
		destroy_win(this);
		return;
	}

	// Refresh button state.  Otherwise, they would not show pressed.
	bt_replace_line.pressed=replace_line;
	if (cnv.is_bound() && cnv->get_line().is_bound()) {
		bt_replace_line.enable();
	} else {
		bt_replace_line.disable();
		replace_line=false;
	}
	bt_replace_all.pressed=replace_all;
	bt_retain_in_depot.pressed = rpl->get_retain_in_depot();
	bt_use_home_depot.pressed = rpl->get_use_home_depot();
	bt_allow_using_existing_vehicles.pressed = rpl->get_allow_using_existing_vehicles();
	
	// Make replace cycle grey if not in use
	uint32 color=(replace_line||replace_all?SYSCOL_BUTTON_TEXT:SYSCOL_BUTTON_TEXT_DISABLED);
	lb_replace_cycle.set_color(color);
	lb_replace.set_color(color);
	lb_sell.set_color(color);
	lb_skip.set_color(color);

	gui_frame_t::draw(pos, size);
}

sint64 replace_frame_t::calc_total_cost()
{
	sint64 total_cost = 0;
	vector_tpl<const vehicle_t*> current_vehicles;
	vector_tpl<uint16> keep_vehicles;
	for(uint8 i = 0; i < cnv->get_vehicle_count(); i ++)
	{
		current_vehicles.append(cnv->get_vehicle(i));
	}
	ITERATE((*convoy_assembler.get_vehicles()),j)
	{
		const vehicle_desc_t* veh = NULL;
		//const vehicle_desc_t* test_new_vehicle = (*convoy_assembler.get_vehicles())[j]; // unused
		// First - check whether there are any of the required vehicles already
		// in the convoy (free)
		ITERATE(current_vehicles,k)
		{
			//const vehicle_desc_t* test_old_vehicle = current_vehicles[k]->get_desc(); // unused
			if(!keep_vehicles.is_contained(k) && current_vehicles[k]->get_desc() == (*convoy_assembler.get_vehicles())[j])
			{
				veh = current_vehicles[k]->get_desc();
				keep_vehicles.append_unique(k);
				// No change to price here.
				break;
			}
		}

		// We cannot look up the home depot here, so we cannot check whether there are any 
		// suitable vehicles stored there as is done when the actual replacing takes place.

		if (veh == NULL) 
		{
			// Second - check whether the vehicle can be upgraded (cheap).
			// But only if the user does not want to keep the vehicles for
			// something else.
			if(!rpl->get_retain_in_depot())
			{
				ITERATE(current_vehicles,l)
				{	
					for(uint8 c = 0; c < current_vehicles[l]->get_desc()->get_upgrades_count(); c++)
					{
						//const vehicle_desc_t* possible_upgrade_test = current_vehicles[l]->get_desc()->get_upgrades(c); // unused
						if(!keep_vehicles.is_contained(l) && (*convoy_assembler.get_vehicles())[j] == current_vehicles[l]->get_desc()->get_upgrades(c))
						{
							veh = current_vehicles[l]->get_desc();
							keep_vehicles.append_unique(l);
							total_cost += veh ? veh->get_upgrades(c)->get_upgrade_price() : 0;
							goto end_loop;
						}
					}
				}
			}
end_loop:	
			if(veh == NULL)
			{
				// Third - if all else fails, buy from new (expensive).
				total_cost += (*convoy_assembler.get_vehicles())[j]->get_value();
			}
		}
	}
	ITERATE(current_vehicles,m)
	{
		if(!keep_vehicles.is_contained(m))
		{
			// This vehicle will not be kept after replacing - 
			// deduct its resale value from the total cost.
			total_cost -= current_vehicles[m]->calc_sale_value();
		}
	}
	
	return total_cost;
}

replace_frame_t::~replace_frame_t()
{
	// TODO: Find why this causes crashes. Without it, there is a small memory leak.
	//delete rpl;
}

