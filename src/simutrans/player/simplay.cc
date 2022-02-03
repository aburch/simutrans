/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../simconvoi.h"
#include "../simdebug.h"
#include "../obj/depot.h"
#include "../simhalt.h"
#include "../simintr.h"
#include "../simline.h"
#include "../simmesg.h"
#include "../simsound.h"
#include "../simticker.h"
#include "../tool/simtool.h"
#include "../gui/simwin.h"
#include "../world/simworld.h"
#include "../display/viewport.h"

#include "../builder/brueckenbauer.h"
#include "../builder/hausbauer.h"
#include "../builder/tunnelbauer.h"

#include "../descriptor/tunnel_desc.h"
#include "../descriptor/way_desc.h"

#include "../ground/grund.h"

#include "../dataobj/settings.h"
#include "../dataobj/scenario.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../network/network_socket_list.h"

#include "../obj/bruecke.h"
#include "../obj/gebaeude.h"
#include "../obj/leitung2.h"
#include "../obj/tunnel.h"

#include "../gui/messagebox.h"
#include "../gui/player_frame.h"

#include "../utils/cbuffer.h"
#include "../utils/simstring.h"

#include "../vehicle/air_vehicle.h"

#include "simplay.h"
#include "finance.h"

karte_ptr_t player_t::welt;

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t load_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

player_t::player_t(uint8 nr) :
	finance(new finance_t(this, welt)),
	player_age(0),
	player_color_1(0),
	player_color_2(0),
	player_nr(nr),
	active(false),
	locked(false),
	unlock_pending(false),
	simlinemgmt(),
	undo_type(invalid_wt),
	headquarter_level(0),
	headquarter_pos(koord::invalid)
{
	welt->get_settings().set_player_color_to_default(this);

	// we have different AI, try to find out our type:
	sprintf(player_name_buf,"player %i",player_nr-1);
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


sint64 player_t::add_maintenance(sint64 change, waytype_t const wt)
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &load_mutex );
#endif
	const sint64 tmp = finance->book_maintenance(change, wt);
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &load_mutex );
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
			if(  amount>=10000  &&  !welt->is_fast_forward()  ) {
				welt->play_sound_area_clipped(pos, SFX_CASH, CASH_SOUND );
			}
		}
	}
}


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


void player_t::book_running_costs(const sint64 amount, const waytype_t wt)
{
	finance->book_running_costs(amount, wt);
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


void player_t::display_messages()
{
	const viewport_t *vp = welt->get_viewport();

	FOR(slist_tpl<income_message_t*>, const m, messages) {

		const scr_coord scr_pos = vp->get_screen_coord(koord3d(m->pos,welt->lookup_hgt(m->pos)),koord(0,m->alter >> 4)) + scr_coord((get_tile_raster_width()-display_calc_proportional_string_len_width(m->str, 0x7FFF))/2,0);

		display_shadow_proportional_rgb( scr_pos.x, scr_pos.y, PLAYER_FLAG|color_idx_to_rgb(player_color_1+3), color_idx_to_rgb(COL_BLACK), m->str, true);
		if(  m->pos.x < 3  ||  m->pos.y < 3  ) {
			// very close to border => renew background
			welt->set_background_dirty();
		}
	}
}


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
	player_color_1 = col1;
	player_color_2 = col2;
	display_set_player_color_scheme( player_nr, col1, col2 );
}


void player_t::step()
{
}


bool player_t::new_month()
{
	// since the messages must remain on the screen longer ...
	static cbuffer_t buf;

	finance->new_month();

	// new month has started => recalculate vehicle value
	calc_assets();

	finance->calc_finance_history();

	// Bankrupt ?
	if(  finance->get_account_balance() < 0  ) {
		finance->increase_account_overdrawn();
		if(  !welt->get_settings().is_freeplay()  &&  player_nr != 1  ) {
			if(  welt->get_active_player_nr()==player_nr  &&  !env_t::networkmode  ) {
				if(  finance->get_netwealth() < 0 ) {
					destroy_all_win(true);
					create_win( display_get_width()/2-128, 40, new news_img("Bankrott:\n\nDu bist bankrott.\n"), w_info, magic_none);
					ticker::add_msg( translator::translate("Bankrott:\n\nDu bist bankrott.\n"), koord::invalid, PLAYER_FLAG + player_color_1 + 1 );
					welt->stop(false);
				}
				else if(  finance->get_netwealth()*10 < welt->get_settings().get_starting_money(welt->get_current_month()/12)  ){
					// tell the player (problem!)
					welt->get_message()->add_message( translator::translate("Net wealth less than 10% of starting capital!"), koord::invalid, message_t::problems, player_nr, IMG_EMPTY );
				}
				else {
					// tell the player (just warning)
					buf.clear();
					buf.printf( translator::translate("On loan since %i month(s)"), finance->get_account_overdrawn() );
					welt->get_message()->add_message( buf, koord::invalid, message_t::ai, player_nr, IMG_EMPTY );
				}
			}
			// no assets => nothing to go bankrupt about again
			else if(  finance->get_maintenance(TT_ALL) != 0  ||  finance->has_convoi()  ) {

				// for AI, we only declare bankrupt, if total assets are below zero
				if(  finance->get_netwealth() < 0  ) {
					return false;
				}
				// tell the current player (even during networkgames)
				if(  welt->get_active_player_nr()==player_nr  ) {
					if(  finance->get_netwealth()*10 < welt->get_settings().get_starting_money(welt->get_current_month()/12)  ){
						// net wealth nearly spent (problem!)
						welt->get_message()->add_message( translator::translate("Net wealth near zero"), koord::invalid, message_t::problems, player_nr, IMG_EMPTY );
					}
					else {
						// just minus in account (just tell)
						buf.clear();
						buf.printf( translator::translate("On loan since %i month(s)"), finance->get_account_overdrawn() );
						welt->get_message()->add_message( buf, koord::invalid, message_t::ai, player_nr, IMG_EMPTY );
					}
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
				if (env_t::server) {
					// server: unlock this player for all clients
					socket_list_t::unlock_player_all(player_nr, true);
				}
				// clients: local unlock
				pwd_hash.clear();
				locked = false;
				unlock_pending = false;
			}
		}
	}

	// update line info
	simlinemgmt.new_month();

	// subtract maintenance after bankruptcy check
	finance->book_account( -finance->get_maintenance_with_bits(TT_ALL) );

	// company gets older ...
	player_age ++;

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
			sint64 restwert = cnv->calc_restwert();
			assets[TT_ALL] += restwert;
			assets[finance->translate_waytype_to_tt(cnv->front()->get_desc()->get_waytype())] += restwert;
		}
	}

	// all vehicles stored in depot not part of a convoi
	FOR(slist_tpl<depot_t*>, const depot, depot_t::get_depot_list()) {
		if(  depot->get_owner_nr() == player_nr  ) {
			FOR(slist_tpl<vehicle_t*>, const veh, depot->get_vehicle_list()) {
				sint64 restwert = veh->calc_sale_value();
				assets[TT_ALL] += restwert;
				assets[finance->translate_waytype_to_tt(veh->get_desc()->get_waytype())] += restwert;
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
	return owner == test  ||  owner == NULL  ||  test == welt->get_public_player();
}


void player_t::ai_bankrupt()
{
	player_t *const psplayer = welt->get_public_player();

	DBG_MESSAGE("player_t::ai_bankrupt()","Removing convois");

	for (size_t i = welt->convoys().get_count(); i-- != 0;) {
		convoihandle_t const cnv = welt->convoys()[i];
		if(cnv->get_owner()!=this) {
			continue;
		}

		linehandle_t line = cnv->get_line();

		cnv->self_destruct();
		// convois not in depots must step to really get rid of them
		if(  cnv.is_bound()  &&  cnv->get_state() != convoi_t::INITIAL  ) {
			cnv->step();
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
					// make public
					if(  w  &&  w->get_owner()==this  ) {
						// tunnels and bridges are handled later? (logic needs to be checked for correct maintenance costs)
						if (!gr->ist_bruecke()  &&  !gr->ist_tunnel()) {
							sint32 const costs = w->get_desc()->get_maintenance();
							waytype_t const wt = w->get_desc()->get_finance_waytype();
							player_t::add_maintenance(this, -costs, wt);
							player_t::add_maintenance(psplayer, costs, wt);
						}
						w->set_owner(psplayer);
					}
				}
			}
		}
	}

	// deactivate active tool (remove dummy grounds)
	welt->set_tool(tool_t::general_tool[TOOL_QUERY], this);

	// next remove all ways, depot etc, that are not road or channels
	for( int y=0;  y<welt->get_size().y;  y++  ) {
		for( int x=0;  x<welt->get_size().x;  x++  ) {
			planquadrat_t *plan = welt->access(x,y);
			for (size_t b = plan->get_boden_count(); b-- != 0;) {
				grund_t *gr = plan->get_boden_bei(b);
				// remove tunnel and bridges first
				if(  gr->get_top()>0  &&  gr->obj_bei(0)->get_owner()==this   &&  (gr->ist_bruecke()  ||  gr->ist_tunnel())  ) {
					koord3d pos = gr->get_pos();

					waytype_t wt = gr->hat_wege() ? gr->get_weg_nr(0)->get_waytype() : powerline_wt;
					if (gr->ist_bruecke()) {
						bridge_builder_t::remove( this, pos, wt );
						// fails if powerline bridge somehow connected to powerline bridge of another player
					}
					else {
						tunnel_builder_t::remove( this, pos, wt, true );
					}
					// maybe there are some objects left (station on bridge head etc)
					gr = plan->get_boden_in_hoehe(pos.z);
					if (gr == NULL) {
						continue;
					}
				}
				for (uint8 i = gr->get_top(); i-- != 0;) {
					obj_t *obj = gr->obj_bei(i);
					if(obj->get_owner()==this) {
						sint32 costs = 0;
						waytype_t wt = ignore_wt;
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
								obj->cleanup(this);
								delete obj;
								break;
							case obj_t::leitung:
								// do not remove powerline from bridges
								if(gr->ist_bruecke()) {
									costs = ((leitung_t*)obj)->get_desc()->get_maintenance();
									add_maintenance(-costs, powerline_wt);
									psplayer->add_maintenance(costs, powerline_wt);
									obj->set_owner(psplayer);
								}
								else {
									obj->cleanup(this);
									delete obj;
								}
								break;
							case obj_t::gebaeude:
								hausbauer_t::remove( this, (gebaeude_t *)obj );
								gr = plan->get_boden_bei(b); // fundament has now been replaced by normal ground
								break;
							case obj_t::way:
							{
								weg_t *w=(weg_t *)obj;
								// tunnels and bridges made public
								if (gr->ist_bruecke()  ||  gr->ist_tunnel()) {
									w->set_owner(psplayer);
								}
								// roads and water ways also made public
								else if(w->get_waytype()==road_wt  ||  w->get_waytype()==water_wt) {
									costs = w->get_desc()->get_maintenance();
									wt = w->get_waytype();
									add_maintenance(-costs, wt);
									psplayer->add_maintenance(costs, wt);
									w->set_owner(psplayer);
								}
								else {
									gr->weg_entfernen( w->get_waytype(), true );
								}
								break;
							}
							case obj_t::bruecke:
								costs = ((bruecke_t*)obj)->get_desc()->get_maintenance();
								wt = obj->get_waytype();
								add_maintenance(-costs, wt);
								psplayer->add_maintenance(costs, wt);
								obj->set_owner(psplayer);
								break;
							case obj_t::tunnel:
								costs = ((tunnel_t*)obj)->get_desc()->get_maintenance();
								wt = ((tunnel_t*)obj)->get_desc()->get_finance_waytype();
								add_maintenance(-costs, wt);
								psplayer->add_maintenance(costs, wt);
								obj->set_owner(psplayer);
								break;

							default:
								obj->set_owner(psplayer);
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


void player_t::rdwr(loadsave_t *file)
{
	xml_tag_t sss( file, "spieler_t" );

	if(file->is_version_less(112, 5)) {
		sint64 konto = finance->get_account_balance();
		file->rdwr_longlong(konto);
		finance->set_account_balance(konto);

		sint32 account_overdrawn = finance->get_account_overdrawn();
		file->rdwr_long(account_overdrawn);
		finance->set_account_overdrawn( account_overdrawn );
	}

	if(file->is_version_less(101, 0)) {
		// ignore steps
		sint32 ldummy=0;
		file->rdwr_long(ldummy);
	}

	if(file->is_version_less(99, 9)) {
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
	if(file->is_version_less(99, 8)) {
		file->rdwr_long(halt_count);
	}
	if(file->is_version_less(112, 3)) {
		sint32 haltcount = 0;
		file->rdwr_long(haltcount);
	}

	// save all the financial statistics
	finance->rdwr( file );

	file->rdwr_bool(active);

	// state is not saved anymore
	if(file->is_version_less(99, 14)) {
		sint32 ldummy=0;
		file->rdwr_long(ldummy);
		file->rdwr_long(ldummy);
	}

	// the AI stuff is now saved directly by the different AI
	if(  file->is_version_less(101, 0)) {
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
	if (file->is_version_less(86, 4))
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
	if(file->is_version_atleast(88, 3)) {
		simlinemgmt.rdwr(file,this);
	}

	if(file->is_version_atleast(102, 3)) {
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
	if(  file->is_version_atleast(102, 4)  ) {
		file->rdwr_str( player_name_buf, lengthof(player_name_buf) );
	}

	// save age
	if(  file->is_version_atleast(112, 2)  ) {
		file->rdwr_short( player_age );
	}
}


void player_t::finish_rd()
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


void player_t::report_vehicle_problem(convoihandle_t cnv,const koord3d position)
{
	switch(cnv->get_state()) {

		case convoi_t::NO_ROUTE:
DBG_MESSAGE("player_t::report_vehicle_problem","Vehicle %s can't find a route to (%i,%i)!", cnv->get_name(),position.x,position.y);
			{
				cbuffer_t buf;
				buf.printf( translator::translate("Vehicle %s can't find a route!"), cnv->get_name());
				welt->get_message()->add_message( (const char *)buf, cnv->get_pos().get_2d(), message_t::problems, PLAYER_FLAG | player_nr, cnv->front()->get_base_image());
			}
			break;

		case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
		case convoi_t::CAN_START_ONE_MONTH:
		case convoi_t::CAN_START_TWO_MONTHS:
DBG_MESSAGE("player_t::report_vehicle_problem","Vehicle %s stuck!", cnv->get_name(),position.x,position.y);
			{
				cbuffer_t buf;
				buf.printf( translator::translate("Vehicle %s is stucked!"), cnv->get_name());
				welt->get_message()->add_message( (const char *)buf, cnv->get_pos().get_2d(), message_t::warnings, PLAYER_FLAG | player_nr, cnv->front()->get_base_image());
			}
			break;

		default:
DBG_MESSAGE("player_t::report_vehicle_problem","Vehicle %s, state %i!", cnv->get_name(), cnv->get_state());
	}
	(void)position;
}


void player_t::init_undo( waytype_t wtype, unsigned short max )
{
	// only human player
	// allow for UNDO for real player
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
					}
					/* FALLTHROUGH */
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
			cost += gr->weg_entfernen(undo_type,true);
		}
		else {
			if (leitung_t* lt = gr->get_leitung()) {
				cost += lt->get_desc()->get_price();
				lt->cleanup(NULL);
				delete lt;
			}
		}
	}
	last_built.clear();
	return cost;
}


void player_t::tell_tool_result(tool_t *tool, koord3d, const char *err)
{
	/* tools can return three kinds of messages
	 * NULL = success
	 * "" = failure, but just do not try again
	 * "bla" error message, which should be shown
	 */
	if (welt->get_active_player()==this) {
		if(err==NULL) {
			if(tool->ok_sound!=NO_SOUND) {
				sound_play(tool->ok_sound,255,TOOL_SOUND);
			}
		}
		else if(*err!=0) {
			// something went really wrong
			sound_play( SFX_FAILURE, 255, TOOL_SOUND );
			// look for coordinate in error message
			// syntax: either @x,y or (x,y)
			open_error_msg_win(err);
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

bool player_t::can_afford(sint64 cost) const
{
	return welt->get_settings().is_freeplay() ||
		is_public_service() ||
		get_finance()->get_netwealth() >= -cost;
}

bool player_t::is_public_service() const
{
	return get_player_nr() == PUBLIC_PLAYER_NR;
}
