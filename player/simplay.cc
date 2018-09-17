/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Renovation in dec 2004 for other vehicles, timeline
 * @author prissi
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../simconvoi.h"
#include "../simdebug.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../simintr.h"
#include "../simline.h"
#include "../simmesg.h"
#include "../simsound.h"
#include "../simticker.h"
#include "../simtool.h"
#include "../gui/simwin.h"
#include "../simworld.h"
#include "../simsignalbox.h"

#include "../display/viewport.h"

#include "../bauer/brueckenbauer.h"
#include "../bauer/hausbauer.h"
#include "../bauer/tunnelbauer.h"

#include "../descriptor/tunnel_desc.h"
#include "../descriptor/way_desc.h"

#include "../boden/grund.h"

#include "../bauer/wegbauer.h"

#include "../dataobj/settings.h"
#include "../dataobj/scenario.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/schedule.h"

#include "../obj/bruecke.h"
#include "../obj/gebaeude.h"
#include "../obj/leitung2.h"
#include "../obj/tunnel.h"

#include "../gui/messagebox.h"
#include "../gui/player_frame_t.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "../vehicle/simvehicle.h"

#include "simplay.h"
#include "finance.h"

karte_t *player_t::welt = NULL;

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t load_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

player_t::player_t(karte_t *wl, uint8 nr) :
	simlinemgmt()
{
	finance = new finance_t(this, wl);
	welt = wl;
	player_nr = nr;
	player_age = 0;
	active = false;			// Don't start as an AI player
	locked = false;			// allowed to change anything
	unlock_pending = false;
	has_been_warned_about_no_money_for_renewals = false;
	selected_signalbox = NULL;

	headquarter_pos = koord::invalid;
	headquarter_level = 0;

	welt->get_settings().set_default_player_color(this);

	// we have different AI, try to find out our type:
	sprintf(player_name_buf,"player %i",player_nr-1);

	const bool allow_access_by_default = player_nr == 1;

	for(int i = 0; i < MAX_PLAYER_COUNT; i ++)
	{
		access[i] = allow_access_by_default;
	}

	// By default, allow access to the public player.
	// In most cases, the public player can override the absence of 
	// access in any event, but this is relevant to whether private
	// cars may use player roads.
	access[1] = true; 
}


player_t::~player_t()
{
	while(  !messages.empty()  ) {
		delete messages.remove_first();
	}
	destroy_win(magic_finances_t + get_player_nr());
	delete finance;
	finance = NULL;
}


void player_t::book_construction_costs(player_t * const player, const sint64 amount, const koord k, const waytype_t wt)
{
	if(player!=NULL) {
		player->finance->book_construction_costs(amount, wt);
		if(k != koord::invalid) {
			player->add_money_message(amount, k);
		}
	}
}


/**
 * Adds some amount to the maintenance costs.
 * @param change the change
 * @return the new maintenance costs
 * @author Hj. Malthaner
 */
sint64 player_t::add_maintenance(sint64 change, waytype_t const wt)
{
	int tmp = 0;
#ifdef MULTI_THREAD
		pthread_mutex_lock( &load_mutex  );
#endif
	tmp = finance->book_maintenance(change, wt);
#ifdef MULTI_THREAD
		pthread_mutex_unlock( &load_mutex  );
#endif
	return tmp;
}


void player_t::add_money_message(const sint64 amount, const koord pos)
{
	if(amount != 0  &&  player_nr != 1) {
		if(  koord_distance(welt->get_viewport()->get_world_position(),pos)<2*(uint32)(display_get_width()/get_tile_raster_width())+3  ) {
			// only display, if near the screen ...
			add_message(amount, pos);

			// and same for sound too ...
#ifdef GDI_SOUND
			if(  amount>=10000  &&  !welt->is_fast_forward() && vehicle_t::sound_ticks < world()->get_ticks() ) {
#else
			if (amount >= 10000 && !welt->is_fast_forward()) {
#endif
				welt->play_sound_area_clipped(pos, SFX_CASH);
			}
		}
	}
}


/**
 * amount has negative value = buy vehicle, positive value = vehicle sold
 */
void player_t::book_new_vehicle(const sint64 amount, const koord k, const waytype_t wt)
{
	finance->book_new_vehicle(amount, wt);
	add_money_message(amount, k);
}


void player_t::book_revenue(const sint64 amount, const koord k, const waytype_t wt, sint32 index)
{
	finance->book_revenue(amount, wt, index);
	add_money_message(amount, k);
}

void player_t::book_way_renewal(const sint64 amount, const waytype_t wt)
{
	finance->book_way_renewal(-amount, wt);
}


void player_t::book_running_costs(const sint64 amount, const waytype_t wt)
{
	finance->book_running_costs(amount, wt);
}

void player_t::book_vehicle_maintenance(const sint64 amount, const waytype_t wt)
{
	finance->book_vehicle_maintenance_with_bits(amount, wt);
	// Consider putting messages in here
}


void player_t::book_toll_paid(const sint64 amount, const waytype_t wt)
{
	finance->book_toll_paid(amount, wt);
}


void player_t::book_toll_received(const sint64 amount, const waytype_t wt)
{
	finance->book_toll_received(amount, wt);
}


void player_t::book_transported(const sint64 amount, const waytype_t wt, int index)
{
	finance->book_transported(amount, wt, index);
}

void player_t::book_delivered(const sint64 amount, const waytype_t wt, int index)
{
	finance->book_delivered(amount, wt, index);
}

bool player_t::can_afford(const sint64 price) const
{
	return (is_public_service() || finance->can_afford(price));
}

bool player_t::can_afford(player_t* player, sint64 price)
{
	if (!player) {
		// If there is no player involved, it can be afforded
		return true;
	} else {
		return player->can_afford(price);
	}
}

/* returns the name of the player; "player -1" sits in front of the screen
 * @author prissi
 */
const char* player_t::get_name() const
{
	return translator::translate(player_name_buf);
}


void player_t::set_name(const char *new_name)
{
	tstrncpy( player_name_buf, new_name, lengthof(player_name_buf) );

	// update player window
	if (ki_kontroll_t *frame = dynamic_cast<ki_kontroll_t *>( win_get_magic(magic_ki_kontroll_t) ) ) {
		frame->update_data();
	}
}


/**
 * floating massages for all players here
 */
player_t::income_message_t::income_message_t( sint64 betrag, koord p )
{
	money_to_string(str, betrag/100.0);
	alter = 127;
	pos = p;
	amount = betrag;
}


void *player_t::income_message_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(player_t::income_message_t));
}


void player_t::income_message_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(player_t::income_message_t),p);
}


/**
 * Show income messages
 * @author prissi
 */
void player_t::display_messages()
{
	const viewport_t *vp = welt->get_viewport();

	FOR(slist_tpl<income_message_t*>, const m, messages) {

		const scr_coord scr_pos = vp->get_screen_coord(koord3d(m->pos,welt->lookup_hgt(m->pos)),koord(0,m->alter >> 4));

		display_shadow_proportional( scr_pos.x, scr_pos.y, PLAYER_FLAG|(player_color_1+3), SYSCOL_TEXT_SHADOW, m->str, true);
		if(  m->pos.x < 3  ||  m->pos.y < 3  ) {
			// very close to border => renew background
			welt->set_background_dirty();
		}
	}
}


/**
 * Age messages (move them upwards), delete too old ones
 * @author prissi
 */
void player_t::age_messages(uint32 /*delta_t*/)
{
	for(slist_tpl<income_message_t *>::iterator iter = messages.begin(); iter != messages.end(); ) {
		income_message_t *m = *iter;
		m->alter -= 5;

		if(m->alter<-80) {
			iter = messages.erase(iter);
			delete m;
		}
		else {
			++iter;
		}
	}
}


void player_t::add_message(sint64 betrag, koord k)
{
	if(  !messages.empty()  &&  messages.back()->pos==k  &&  messages.back()->alter==127  ) {
		// last message exactly at same place, not aged
		messages.back()->amount += betrag;
		money_to_string(messages.back()->str, messages.back()->amount/100.0);
	}
	else {
		// otherwise new message
		income_message_t *m = new income_message_t(betrag,k);
		messages.append( m );
	}
}


void player_t::set_player_color(uint8 col1, uint8 col2)
{
	if(player_color_1 != col1 && welt->get_player(player_nr))
	{
		// Only show a change of colour scheme message if the primary colour changes.
		cbuffer_t message;
		const char* player_name = welt->get_player(player_nr)->get_name();
		message.printf(player_name);
		welt->get_message()->add_message(message, koord::invalid, message_t::ai, player_color_1);
		message.clear();
		message.printf(translator::translate("has changed its colour scheme."));
		welt->get_message()->add_message(message, koord::invalid, message_t::ai, col1);
	}
	set_player_color_no_message(col1, col2);
}

void player_t::set_player_color_no_message(uint8 col1, uint8 col2)
{
	player_color_1 = col1;
	player_color_2 = col2;
	display_set_player_color_scheme( player_nr, col1, col2 );
}


/**
 * Any action goes here (only need for AI at the moment)
 * @author Hj. Malthaner
 */
void player_t::step()
{
	/*
	NOTE: This would need updating to the new FOR iterators to work now.
	// die haltestellen müssen die Fahrpläne rgelmaessig pruefen
	uint8 i = (uint8)(welt->get_steps()+player_nr);
	//slist_iterator_tpl <nearby_halt_t> iter( halt_list );
	//while(iter.next()) {
	for(sint16 j = halt_list.get_count() - 1; j >= 0; j --)
	{
		if( (i & 31) == 0 ) {
			//iter.get_current().halt->step();
			halt_list[j].halt->step();
			INT_CHECK("simplay 280");
		}
	}
	*/
}


/**
 * wird von welt nach jedem monat aufgerufen
 * @author Hj. Malthaner
 */
bool player_t::new_month()
{
	// since the messages must remain on the screen longer ...
	static cbuffer_t buf;

	// This handles rolling the finance records,
	// interest charges,
	// and infrastructure maintenance
	finance->new_month();

	// new month has started => recalculate vehicle value
	calc_assets();

	// After recalculating the assets, recalculate the credit limits
	// Credit limits are NEGATIVE numbers
	finance->calc_credit_limits();

	finance->calc_finance_history(); // Recalc after calc_assets

	simlinemgmt.new_month();


	// Insolvency settings.
	// Modified by jamespetts, February 2009
	// Bankrupt ?
	sint64 account_balance = finance->get_account_balance();
	if(  account_balance < 0  )
	{
		finance->increase_account_overdrawn();
		if(!welt->get_settings().is_freeplay() && player_nr != 1 /* public player*/ )
		{
			if(welt->get_active_player_nr() == player_nr) 
			{
				if(  account_balance < finance->get_hard_credit_limit() && welt->get_settings().bankruptcy_allowed() && !env_t::networkmode )
				{
					destroy_all_win(true);
					create_win( display_get_width()/2-128, 40, new news_img("Bankrott:\n\nDu bist bankrott.\n"), w_info, magic_none);
					ticker::add_msg( translator::translate("Bankrott:\n\nDu bist bankrott.\n"), koord::invalid, PLAYER_FLAG + player_color_1 + 1 );
					welt->stop(false);
				}
				else
				{
					// Warnings about financial problems
					buf.clear();
					enum message_t::msg_typ warning_message_type = message_t::warnings;
					// Plural detection for the months. 
					// Different languages pluralise in different ways, so whole string must
					// be re-translated.
					if(finance->get_account_overdrawn() > 1)
					{
						buf.printf(translator::translate("You have been overdrawn\nfor %i months"), finance->get_account_overdrawn() );
					}
					else
					{
						buf.printf("%s", translator::translate("You have been overdrawn\nfor one month"));
					}
					if(welt->get_settings().get_interest_rate_percent() > 0)
					{
						buf.printf(translator::translate("\n\nInterest on your debt is\naccumulating at %i %%"),welt->get_settings().get_interest_rate_percent() );
					}
					if(  account_balance < finance->get_hard_credit_limit()  ) {
						buf.printf( translator::translate("\n\nYou are insolvent!") );
						// Only in network mode, freeplay, or no-bankruptcy
						// This is a more serious problem than the interest
						warning_message_type = message_t::problems;
					}
					else if(  account_balance < finance->get_soft_credit_limit()  ) {
						buf.printf( translator::translate("\n\nYou have exceeded your credit limit!") );
						// This is a more serious problem than the interest
						warning_message_type = message_t::problems;
					}
					welt->get_message()->add_message( buf, koord::invalid, warning_message_type, player_nr, IMG_EMPTY );
				}
			}
			
			if(welt->get_active_player_nr() != player_nr || env_t::networkmode)  // Not the active player or a multi-player game
			{
				// AI players play by the same rules as human players regarding bankruptcy.
				if(  account_balance < finance->get_hard_credit_limit() && welt->get_settings().bankruptcy_allowed() )
				{
					ai_bankrupt();
				}
			}
		}
	}
	else {
		finance->set_account_overdrawn( 0 );
	}

	if(  env_t::networkmode  &&  player_nr>1  &&  !active  ) {
		// find out dummy companies (i.e. no vehicle running within x months)
		if(  welt->get_settings().get_remove_dummy_player_months()  &&  player_age >= welt->get_settings().get_remove_dummy_player_months()  )  {
			bool no_cnv = true;
			const uint16 months = min( MAX_PLAYER_HISTORY_MONTHS,  welt->get_settings().get_remove_dummy_player_months() );
			for(  uint16 m=0;  m<months  &&  no_cnv;  m++  ) {
				no_cnv &= finance->get_history_com_month(m, ATC_ALL_CONVOIS) ==0;
			}
			const uint16 years = max( MAX_PLAYER_HISTORY_YEARS,  (welt->get_settings().get_remove_dummy_player_months() - 1) / 12 );
			for(  uint16 y=0;  y<years  &&  no_cnv;  y++  ) {
				no_cnv &= finance->get_history_com_year(y, ATC_ALL_CONVOIS)==0;
			}
			// never run a convoi => dummy
			if(  no_cnv  ) {
				return false; // remove immediately
			}
		}

		// find out abandoned companies (no activity within x months)
		if(  welt->get_settings().get_unprotect_abandoned_player_months()  &&  player_age >= welt->get_settings().get_unprotect_abandoned_player_months()  )  {
			bool abandoned = true;
			const uint16 months = min( MAX_PLAYER_HISTORY_MONTHS,  welt->get_settings().get_unprotect_abandoned_player_months() );
			for(  uint16 m = 0;  m < months  &&  abandoned;  m++  ) {
				abandoned &= finance->get_history_veh_month(TT_ALL, m, ATV_NEW_VEHICLE)==0  &&  finance->get_history_veh_month(TT_ALL, m, ATV_CONSTRUCTION_COST)==0;
			}
			const uint16 years = min( MAX_PLAYER_HISTORY_YEARS, (welt->get_settings().get_unprotect_abandoned_player_months() - 1) / 12);
			for(  uint16 y = 0;  y < years  &&  abandoned;  y++  ) {
				abandoned &= finance->get_history_veh_year(TT_ALL, y, ATV_NEW_VEHICLE)==0  &&  finance->get_history_veh_year(TT_ALL, y, ATV_CONSTRUCTION_COST)==0;
			}
			// never changed convoi, never built => abandoned
			if(  abandoned  ) {
				pwd_hash.clear();
				locked = false;
				unlock_pending = false;
			}
		}
	}

	// subtract maintenance after bankruptcy check
	finance->book_account( -finance->get_maintenance_with_bits(TT_ALL) );
	// company gets older ...
	player_age ++;

	has_been_warned_about_no_money_for_renewals = false;

	return true; // still active
}


void player_t::calc_assets()
{
	sint64 assets[TT_MAX];
	for(int i=0; i < TT_MAX; ++i){
		assets[i] = 0;
	}
	// all convois
	FOR(vector_tpl<convoihandle_t>, const cnv, welt->convoys()) {
		if(  cnv->get_owner() == this  ) {
			sint64 restwert = cnv->calc_sale_value();
			assets[TT_ALL] += restwert;
			assets[finance->translate_waytype_to_tt(cnv->front()->get_waytype())] += restwert;
		}
	}

	// all vehicles stored in depot not part of a convoi
	FOR(slist_tpl<depot_t*>, const depot, depot_t::get_depot_list()) {
		if(  depot->get_player_nr() == player_nr  ) {
			FOR(slist_tpl<vehicle_t*>, const veh, depot->get_vehicle_list()) {
				sint64 restwert = veh->calc_sale_value();
				assets[TT_ALL] += restwert;
				assets[finance->translate_waytype_to_tt(veh->get_waytype())] += restwert;
			}
		}
	}

	finance->set_assets(assets);
}


void player_t::update_assets(sint64 const delta, const waytype_t wt)
{
	finance->update_assets(delta, wt);
}


sint32 player_t::get_scenario_completion() const
{
	return finance->get_scenario_completed();
}


void player_t::set_scenario_completion(sint32 percent)
{
	finance->set_scenario_completed(percent);
}


bool player_t::check_owner( const player_t *owner, const player_t *test )
{
	return owner == test || owner == NULL || (test != NULL  &&  test->is_public_service());
}


void player_t::ai_bankrupt()
{
	DBG_MESSAGE("player_t::ai_bankrupt()","Removing convois");

	for (size_t i = welt->convoys().get_count(); i-- != 0;) {
		convoihandle_t const cnv = welt->convoys()[i];
		if(cnv->get_owner()!=this) {
			continue;
		}

		linehandle_t line = cnv->get_line();

		if(  cnv->get_state() != convoi_t::INITIAL  ) {
			cnv->self_destruct();
			cnv->step();	// to really get rid of it
		}
		else {
			// convois in depots are directly destroyed
			cnv->self_destruct();
		}

		// last vehicle on that connection (no line => railroad)
		if(  !line.is_bound()  ||  line->count_convoys()==0  ) {
			simlinemgmt.delete_line( line );
		}
	}

	// remove headquarters pos
	headquarter_pos = koord::invalid;

	// remove all stops
	// first generate list of our stops
	slist_tpl<halthandle_t> halt_list;
	FOR(vector_tpl<halthandle_t>, const halt, haltestelle_t::get_alle_haltestellen()) {
		if(  halt->get_owner()==this  ) {
			halt_list.append(halt);
		}
	}
	// ... and destroy them
	while (!halt_list.empty()) {
		halthandle_t h = halt_list.remove_first();
		haltestelle_t::destroy( h );
	}

	// transfer all ways in public stops belonging to me to no one
	FOR(vector_tpl<halthandle_t>, const halt, haltestelle_t::get_alle_haltestellen()) {
		if(  halt->get_owner()==welt->get_public_player()  ) {
			// only concerns public stops tiles
			FOR(slist_tpl<haltestelle_t::tile_t>, const& i, halt->get_tiles()) {
				grund_t const* const gr = i.grund;
				for(  uint8 wnr=0;  wnr<2;  wnr++  ) {
					weg_t *w = gr->get_weg_nr(wnr);
					if(  w  &&  w->get_owner()==this  ) {
						// take ownership
						if (wnr>1  ||  (!gr->ist_bruecke()  &&  !gr->ist_tunnel())) {
							player_t::add_maintenance( this, -w->get_desc()->get_maintenance(), w->get_desc()->get_finance_waytype() );
						}
						w->set_owner(NULL); // make unowned
					}
				}
			}
		}
	}

	// deactivate active tool (remove dummy grounds)
	welt->set_tool(tool_t::general_tool[TOOL_QUERY], this);

	// next, mothball all ways, depot etc, that are not road or canals if a mothballed type is available, or else remove them.
	for( int y=0;  y<welt->get_size().y;  y++  ) {
		for( int x=0;  x<welt->get_size().x;  x++  ) {
			planquadrat_t *plan = welt->access(x,y);
			for (size_t b = plan->get_boden_count(); b-- != 0;) {
				grund_t *gr = plan->get_boden_bei(b);
				for (size_t i = gr->get_top(); i-- != 0;) {
					obj_t *obj = gr->obj_bei(i);
					if(obj->get_owner()==this) {
						switch(obj->get_typ()) {
							case obj_t::roadsign:
							case obj_t::signal:
							case obj_t::airdepot:
							case obj_t::bahndepot:
							case obj_t::monoraildepot:
							case obj_t::tramdepot:
							case obj_t::strassendepot:
							case obj_t::schiffdepot:
							case obj_t::senke:
							case obj_t::pumpe:
							case obj_t::wayobj:
							case obj_t::label:
							case obj_t::signalbox:
								obj->cleanup(this);
								delete obj;
								break;
							case obj_t::leitung:
								if(gr->ist_bruecke()) {
									add_maintenance( -((leitung_t*)obj)->get_desc()->get_maintenance(), powerline_wt );
									// do not remove powerline from bridges
									obj->set_owner( welt->get_public_player() );
								}
								else {
									obj->cleanup(this);
									delete obj;
								}
								break;
							case obj_t::gebaeude:
								hausbauer_t::remove( this, (gebaeude_t *)obj, false );
								break;
							case obj_t::way:
							{
								weg_t *w = (weg_t *)obj;
								bool mothball = false;
								if (gr->ist_bruecke() || gr->ist_tunnel())
								{
									w->set_owner(NULL);
									if (!w->is_public_right_of_way())
									{
										mothball = true;
									}
								}
								else if (w->get_waytype() == road_wt || w->get_waytype() == water_wt) {
									add_maintenance(-w->get_desc()->get_maintenance(), w->get_waytype());
									w->set_owner(NULL);
								}
								else
								{
									mothball = true;
								}

								if (mothball)
								{
									weg_t *way = (weg_t *)obj;
									const way_desc_t* mothballed_type = way_builder_t::way_search_mothballed(way->get_waytype(), (systemtype_t)way->get_desc()->get_styp());
									if (mothballed_type && way->get_waytype())
									{
										way->set_desc(mothballed_type);
									}
									else
									{
										gr->weg_entfernen(w->get_waytype(), true);
									}
									way->set_owner(NULL);
								}

								break;
							}
							case obj_t::bruecke:
								add_maintenance( -((bruecke_t*)obj)->get_desc()->get_maintenance(), obj->get_waytype() );
								obj->set_owner( NULL );
								break;
							case obj_t::tunnel:
								add_maintenance( -((tunnel_t*)obj)->get_desc()->get_maintenance(), ((tunnel_t*)obj)->get_desc()->get_finance_waytype() );
								obj->set_owner( NULL );
								break;

							default:
								obj->set_owner( welt->get_public_player() );
						}
					}
				}
				// remove empty tiles (elevated ways)
				if (!gr->ist_karten_boden()  &&  gr->get_top()==0) {
					plan->boden_entfernen(gr);
				}
			}
		}
	}

	active = false;
	// make account negative
	if (finance->get_account_balance() > 0) {
		finance->book_account( -finance->get_account_balance() -1 );
	}

	cbuffer_t buf;
	buf.printf( translator::translate("%s\nwas liquidated."), get_name() );
	welt->get_message()->add_message( buf, koord::invalid, message_t::ai, PLAYER_FLAG|player_nr );
}


/**
 * Stores/save the player state
 * @param file, where the data will be saved
 * @author Hj. Malthaner
 */
void player_t::rdwr(loadsave_t *file)
{
	xml_tag_t sss( file, "player_t" );

	if(file->get_version() < 112005) {
		sint64 konto = finance->get_account_balance();
		file->rdwr_longlong(konto);
		finance->set_account_balance(konto);

		sint32 account_overdrawn = finance->get_account_overdrawn();
		file->rdwr_long(account_overdrawn);
		finance->set_account_overdrawn( account_overdrawn );
	}

	if(file->get_version()<101000) {
		// ignore steps
		sint32 ldummy=0;
		file->rdwr_long(ldummy);
	}

	if(file->get_version()<99009) {
		sint32 farbe;
		file->rdwr_long(farbe);
		player_color_1 = (uint8)farbe*2;
		player_color_2 = player_color_1+24;
	}
	else {
		file->rdwr_byte(player_color_1);
		file->rdwr_byte(player_color_2);
	}

	sint32 halt_count=0;
	if(file->get_version()<99008) {
		file->rdwr_long(halt_count);
	}
	if(file->get_version()<=112002) {
		sint32 haltcount = 0;
		file->rdwr_long(haltcount);
	}

	// save all the financial statistics
	finance->rdwr( file );

	file->rdwr_bool(active);

	// state is not saved anymore
	if(file->get_version()<99014) 
	{
		sint32 ldummy=0;
		file->rdwr_long(ldummy);
		file->rdwr_long(ldummy);
	}

	// the AI stuff is now saved directly by the different AI
	if(  file->get_version()<101000) 
	{
		sint32 ldummy = -1;
		file->rdwr_long(ldummy);
		file->rdwr_long(ldummy);
		file->rdwr_long(ldummy);
		file->rdwr_long(ldummy);
		koord k(-1,-1);
		k.rdwr( file );
		k.rdwr( file );
	}

	if(file->is_loading()) {

		// halt_count will be zero for newer savegames
DBG_DEBUG("player_t::rdwr()","player %i: loading %i halts.",welt->sp2num( this ),halt_count);
		for(int i=0; i<halt_count; i++) {
			haltestelle_t::create( file );
		}
		// empty undo buffer
		init_undo(road_wt,0);
	}

	// headquarters stuff
	if (file->get_version() < 86004)
	{
		headquarter_level = 0;
		headquarter_pos = koord::invalid;
	}
	else {
		file->rdwr_long(headquarter_level);
		headquarter_pos.rdwr( file );
		if(file->is_loading()) {
			if(headquarter_level<0) {
				headquarter_pos = koord::invalid;
				headquarter_level = 0;
			}
		}
	}

	// line management
	if(file->get_version()>=88003) {
		simlinemgmt.rdwr(file,this);
	}

	if(file->get_version()>102002 && file->get_extended_version() != 7) {
		// password hash
		for(  int i=0;  i<20;  i++  ) {
			file->rdwr_byte(pwd_hash[i]);
		}
		if(  file->is_loading()  ) {
			// disallow all actions, if password set (might be unlocked by password gui )
			locked = !pwd_hash.empty();
		}
	}

	// save the name too
	if(file->get_version()>102003 && (file->get_extended_version() >= 9 || file->get_extended_version() == 0)) 
	{
		file->rdwr_str( player_name_buf, lengthof(player_name_buf) );
	}

	if(file->get_version() >= 110007 && file->get_extended_version() >= 10)
	{
		// Save the colour
		file->rdwr_byte(player_color_1);
		file->rdwr_byte(player_color_2);

		// Save access parameters
		uint8 max_players = MAX_PLAYER_COUNT;
		file->rdwr_byte(max_players);
		for(int i = 0; i < max_players; i ++)
		{
			file->rdwr_bool(access[i]);
		}
	}

	// save age
	if(  file->get_version() >= 112002  && (file->get_extended_version() >= 11 || file->get_extended_version() == 0) ) {
		file->rdwr_short( player_age );
	}
}

/**
 * called after game is fully loaded;
 */
void player_t::load_finished()
{
	simlinemgmt.finish_rd();
	display_set_player_color_scheme( player_nr, player_color_1, player_color_2 );
	// recalculate vehicle value
	calc_assets();

	finance->calc_finance_history();
}


void player_t::rotate90( const sint16 y_size )
{
	simlinemgmt.rotate90( y_size );
	headquarter_pos.rotate90( y_size );
}


/**
 * Rückruf, um uns zu informieren, dass ein Vehikel ein Problem hat
 * @author Hansjörg Malthaner
 * @date 26-Nov-2001
 */
void player_t::report_vehicle_problem(convoihandle_t cnv,const koord3d ziel)
{
	switch(cnv->get_state())
	{
	case convoi_t::NO_ROUTE_TOO_COMPLEX:
		DBG_MESSAGE("player_t::report_vehicle_problem", "Vehicle %s can't find a route to (%i,%i) because the route is too long/complex", cnv->get_name(), ziel.x, ziel.y);
		if (this == welt->get_active_player()) {
			cbuffer_t buf;
			buf.printf("%s ", cnv->get_name());
			buf.printf(translator::translate("no_route_too_complex_message")); 
			welt->get_message()->add_message((const char *)buf, cnv->get_pos().get_2d(), message_t::problems, PLAYER_FLAG | player_nr, cnv->front()->get_base_image());
		}
		break;

	case convoi_t::NO_ROUTE:
		DBG_MESSAGE("player_t::report_vehicle_problem","Vehicle %s can't find a route to (%i,%i)!", cnv->get_name(),ziel.x,ziel.y);
			if(this==welt->get_active_player()) {
				cbuffer_t buf;
				buf.printf( translator::translate("Vehicle %s can't find a route!"), cnv->get_name());
				const uint32 max_axle_load = cnv->get_route()->get_max_axle_load();
				const uint32 cnv_weight = cnv->get_highest_axle_load();
				if (cnv_weight > max_axle_load) {
					buf.printf(" ");
					buf.printf(translator::translate("Vehicle weighs %it, but max weight is %it"), cnv_weight, max_axle_load); 
				}
				welt->get_message()->add_message( (const char *)buf, cnv->get_pos().get_2d(), message_t::problems, PLAYER_FLAG | player_nr, cnv->front()->get_base_image());
			}
			break;

		case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
		case convoi_t::CAN_START_ONE_MONTH:
		case convoi_t::CAN_START_TWO_MONTHS:
		DBG_MESSAGE("player_t::report_vehicle_problem","Vehicle %s stuck!", cnv->get_name(),ziel.x,ziel.y);
			{
				cbuffer_t buf;
				buf.printf( translator::translate("Vehicle %s is stucked!"), cnv->get_name());
				welt->get_message()->add_message( (const char *)buf, cnv->get_pos().get_2d(), message_t::warnings, PLAYER_FLAG | player_nr, cnv->front()->get_base_image());
			}
			break;
		
		case convoi_t::OUT_OF_RANGE:
			{
				koord destination = ziel.get_2d();
				schedule_t* const sch = cnv->get_schedule();
				const uint8 entries = sch->get_count();
				uint8 count = 0;
				while(count <= entries && !haltestelle_t::get_halt(destination, this).is_bound() && (welt->lookup_kartenboden(destination) == NULL || !welt->lookup_kartenboden(destination)->get_depot()))
				{
					// Make sure that we are not incorrectly calculating the distance to a waypoint.
					bool rev = cnv->is_reversed();
					uint8 index = sch->get_current_stop();
					sch->increment_index(&index, &rev);
					destination = sch->entries.get_element(index).pos.get_2d(); 
					count++;
				}
				const uint32 distance_from_last_stop_to_here_tiles = shortest_distance(cnv->get_pos().get_2d(), cnv->front()->get_last_stop_pos().get_2d());
				const uint32 distance_tiles = distance_from_last_stop_to_here_tiles + (shortest_distance(cnv->get_pos().get_2d(), destination));
				const double distance = (double)(distance_tiles * (double)welt->get_settings().get_meters_per_tile()) / 1000.0;
				const double excess = distance - (double)cnv->get_min_range();
				DBG_MESSAGE("player_t::report_vehicle_problem","Vehicle %s cannot travel %.2fkm to (%i,%i) because it would exceed its range of %i by %.2fkm", cnv->get_name(), distance, ziel.x, ziel.y, cnv->get_min_range(), excess);
				if(this == welt->get_active_player())
				{
					cbuffer_t buf;
					const halthandle_t destination_halt = haltestelle_t::get_halt(ziel, welt->get_active_player());
					const bool destination_is_depot = welt->lookup(ziel) && welt->lookup(ziel)->get_depot(); 
					const char* name = destination_is_depot ? translator::translate(welt->lookup(ziel)->get_depot()->get_name()) : destination_halt.is_bound() ? destination_halt->get_name() : translator::translate("unknown");
					buf.printf( translator::translate("Vehicle %s cannot travel %.2fkm to %s because that would exceed its range of %ikm by %.2fkm"), cnv->get_name(), distance, name, cnv->get_min_range(), excess);
					welt->get_message()->add_message( (const char *)buf, cnv->get_pos().get_2d(), message_t::warnings, PLAYER_FLAG | player_nr, cnv->front()->get_base_image());
				}
			}
			break;
		default:
		DBG_MESSAGE("player_t::report_vehicle_problem","Vehicle %s, state %i!", cnv->get_name(), cnv->get_state());
	}
	(void)ziel;
}


/* Here functions for UNDO
 * @date 7-Feb-2005
 * @author prissi
 */
void player_t::init_undo( waytype_t wtype, unsigned short max )
{
	// only human player
	// prissi: allow for UNDO for real player
DBG_MESSAGE("player_t::int_undo()","undo tiles %i",max);
	last_built.clear();
	last_built.resize(max+1);
	if(max>0) {
		undo_type = wtype;
	}

}


void player_t::add_undo(koord3d k)
{
	if(last_built.get_size()>0) {
//DBG_DEBUG("player_t::add_undo()","tile at (%i,%i)",k.x,k.y);
		last_built.append(k);
	}
}


sint64 player_t::undo()
{
	if (last_built.empty()) {
		// nothing to UNDO
		return false;
	}
	// check, if we can still do undo
	FOR(vector_tpl<koord3d>, const& i, last_built) {
		grund_t* const gr = welt->lookup(i);
		if(gr==NULL  ||  gr->get_typ()!=grund_t::boden) {
			// well, something was built here ... so no undo
			last_built.clear();
			return false;
		}
		// we allow ways, unimportant stuff but no vehicles, signals, wayobjs etc
		if(gr->obj_count()>0) {
			for( unsigned i=0;  i<gr->get_top();  i++  ) {
				switch(gr->obj_bei(i)->get_typ()) {
					// these are allowed
					case obj_t::zeiger:
					case obj_t::wolke:
					case obj_t::leitung:
					case obj_t::pillar:
					case obj_t::way:
					case obj_t::label:
					case obj_t::crossing:
					case obj_t::pedestrian:
					case obj_t::road_user:
					case obj_t::movingobj:
						break;
					// special case airplane
					// they can be everywhere, so we allow for everything but runway undo
					case obj_t::air_vehicle: {
						if(undo_type!=air_wt) {
							break;
						}
						const air_vehicle_t* aircraft = obj_cast<air_vehicle_t>(gr->obj_bei(i));
						// flying aircrafts are ok
						if(!aircraft->is_on_ground()) {
							break;
						}
						// fall through !
					}
					// all other are forbidden => no undo any more
					default:
						last_built.clear();
						return false;
				}
			}
		}
	}

	// ok, now remove everything last built
	sint64 cost=0;
	FOR(vector_tpl<koord3d>, const& i, last_built) {
		grund_t* const gr = welt->lookup(i);
		if(  undo_type != powerline_wt  ) {
			cost += gr->weg_entfernen(undo_type, true);
			cost -= welt->get_land_value(gr->get_pos());
		}
		else {
			leitung_t* lt = gr->get_leitung();
			if (lt)
			{
				cost += lt->get_desc()->get_value();
				lt->cleanup(NULL);
				delete lt;
			}
		}
	}
	last_built.clear();
	return cost;
}


void player_t::tell_tool_result(tool_t *tool, koord3d, const char *err, bool local)
{
	/* tools can return three kinds of messages
	* NULL = success
	* "" = failure, but just do not try again
	* "bla" error message, which should be shown
	*/
	if (welt->get_active_player() == this && local) {
		if (err == NULL) {
			if (tool->ok_sound != NO_SOUND) {
				sound_play(tool->ok_sound);
			}
		}
		else if (*err != 0) {
			// something went really wrong
			sound_play(SFX_FAILURE);
			// look for coordinate in error message
			// syntax: either @x,y or (x,y)
			koord pos = message_t::get_coord_from_text(err);
			if (pos != koord::invalid) {
				create_win(new news_loc(err, pos), w_time_delete, magic_none);
			}
			else {
				create_win(new news_img(err), w_time_delete, magic_none);
			}
		}
	}
}


void player_t::book_convoi_number(int count)
{
	finance->book_convoi_number(count);
}


double player_t::get_account_balance_as_double() const
{
	return finance->get_account_balance() / 100.0;
}


int player_t::get_account_overdrawn() const
{
	return finance->get_account_overdrawn();
}


bool player_t::has_money_or_assets() const
{
	return finance->has_money_or_assets();
}

void player_t::set_selected_signalbox(signalbox_t* sb)
{
	selected_signalbox = sb ? (signalbox_t*)sb->get_first_tile() : NULL;
	if(!welt->is_destroying())
	{
		tool_t::update_toolbars();
		welt->set_dirty();
	}
}
