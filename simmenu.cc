/*
 * Copyright (c) 2006 prissi
 *
 * welt file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/* menu functions in order to separate a little between menu and simworld */

#include "simworld.h"
#include "simwin.h"
#include "simplay.h"
#include "simmenu.h"
#include "simskin.h"
#include "simsound.h"
#include "simwerkz.h"

#include "bauer/hausbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/tunnelbauer.h"

#include "besch/haus_besch.h"

#include "boden/grund.h"
#include "boden/wege/strasse.h"

#include "dataobj/translator.h"
#include "dataobj/umgebung.h"

#include "dings/roadsign.h"
#include "dings/wayobj.h"

#include "gui/werkzeug_parameter_waehler.h"
#include "gui/messagebox.h"

#include "utils/simstring.h"

/**
 * Creates a tooltip from tip text and money value
 * @author Hj. Malthaner
 */
char * tool_tip_with_price(const char * tip, sint64 price)
{
	static char buf[256];

	const int n = sprintf(buf, "%s, ", tip);
	money_to_string(buf+n, (double)price/-100.0);
	return buf;
}




// open a menu tool window
// player 1 gets a different one!
werkzeug_parameter_waehler_t *menu_fill(karte_t *welt, long magic, spieler_t *sp )
{
	struct sound_info click_sound = { SFX_SELECT, 255, 0 };
	werkzeug_parameter_waehler_t *wzw = (werkzeug_parameter_waehler_t *)win_get_magic(magic);
	bool do_sound = (wzw==NULL);

	switch(magic) {

		case magic_slopetools:
			if(!grund_t::underground_mode) {

				if(wzw==NULL) {
					wzw = new werkzeug_parameter_waehler_t(welt, "SLOPETOOLS");
					wzw->setze_hilfe_datei("slopetools.txt");
				}
				else {
					wzw->reset_tools();
				}

				wzw->add_tool(wkz_raise,
					karte_t::Z_GRID,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::hang_werkzeug->gib_bild_nr(8),
					skinverwaltung_t::upzeiger->gib_bild_nr(0),
					tool_tip_with_price(translator::translate("Anheben"), umgebung_t::cst_alter_land));

				wzw->add_tool(wkz_lower,
					karte_t::Z_GRID,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::hang_werkzeug->gib_bild_nr(9),
					skinverwaltung_t::downzeiger->gib_bild_nr(0),
					tool_tip_with_price(translator::translate("Absenken"), umgebung_t::cst_alter_land));

				wzw->add_param_tool(wkz_set_slope,(long)SOUTH_SLOPE,
					karte_t::Z_PLAN,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::hang_werkzeug->gib_bild_nr(0),
					skinverwaltung_t::slopezeiger->gib_bild_nr(0),
					tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

				wzw->add_param_tool(wkz_set_slope,(long)NORTH_SLOPE,
					karte_t::Z_PLAN,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::hang_werkzeug->gib_bild_nr(1),
					skinverwaltung_t::slopezeiger->gib_bild_nr(0),
					tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

				wzw->add_param_tool(wkz_set_slope,(long)WEST_SLOPE,
					karte_t::Z_PLAN,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::hang_werkzeug->gib_bild_nr(2),
					skinverwaltung_t::slopezeiger->gib_bild_nr(0),
					tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

				wzw->add_param_tool(wkz_set_slope,(long)EAST_SLOPE,
					karte_t::Z_PLAN,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::hang_werkzeug->gib_bild_nr(3),
					skinverwaltung_t::slopezeiger->gib_bild_nr(0),
					tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

				wzw->add_param_tool(wkz_set_slope,(long)ALL_DOWN_SLOPE,
					karte_t::Z_PLAN,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::hang_werkzeug->gib_bild_nr(5),
					skinverwaltung_t::slopezeiger->gib_bild_nr(0),
					tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

				wzw->add_param_tool(wkz_set_slope,(long)ALL_UP_SLOPE,
					karte_t::Z_PLAN,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::hang_werkzeug->gib_bild_nr(6),
					skinverwaltung_t::slopezeiger->gib_bild_nr(0),
					tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

		#ifdef COVER_TILES
				// cover tile
				wzw->add_param_tool(wkz_set_slope,(long)0,
					karte_t::Z_PLAN,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::hang_werkzeug->gib_bild_nr(4),
					skinverwaltung_t::slopezeiger->gib_bild_nr(0),
					tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));
		#endif

				wzw->add_param_tool(wkz_set_slope,(long)RESTORE_SLOPE,
					karte_t::Z_PLAN,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::hang_werkzeug->gib_bild_nr(7),
					skinverwaltung_t::slopezeiger->gib_bild_nr(0),
					tool_tip_with_price(translator::translate("Restore natural slope"), umgebung_t::cst_alter_land));

			}
			else if(wzw!=NULL) {
				destroy_win( wzw );
				wzw = NULL;
			}
		break;

		case magic_railtools:
			if(wegbauer_t::weg_search(track_wt,1,welt->get_timeline_year_month(),weg_t::type_flat)!=NULL) {
				if(wzw==NULL) {
					wzw = new werkzeug_parameter_waehler_t(welt, "RAILTOOLS");
					wzw->setze_hilfe_datei("railtools.txt");
				}
				else {
					wzw->reset_tools();
				}

				if(sp!=welt->gib_spieler(1)  &&  !grund_t::underground_mode) {
					// public player is not allowed to run vehicles ...
					wegbauer_t::fill_menu(wzw,
						track_wt,
						wkz_wegebau,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt,
						weg_t::type_flat
						);

					wegbauer_t::fill_menu(wzw,
						track_wt,
						wkz_wegebau,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt,
						weg_t::type_elevated
						);
				}

				if(sp!=welt->gib_spieler(1)) {
					wayobj_t::fill_menu(wzw,
						track_wt,
						wkz_wayobj,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt
						);

					wzw->add_param_tool(&wkz_wayremover,
						(const int)track_wt,
						karte_t::Z_PLAN,
						SFX_REMOVER,
						SFX_FAILURE,
						skinverwaltung_t::edit_werkzeug->gib_bild_nr(7),
						skinverwaltung_t::killzeiger->gib_bild_nr(0),
						translator::translate("remove tracks"));

					if(!grund_t::underground_mode) {
						// no bridges in tunnels ...
						brueckenbauer_t::fill_menu(wzw,
							track_wt,
							SFX_JACKHAMMER,
							SFX_FAILURE,
							welt
							);
					}

					tunnelbauer_t::fill_menu(wzw,
						track_wt,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt
						);

					roadsign_t::fill_menu(wzw,
						track_wt,
						wkz_roadsign,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt
						);

					wzw->add_param_tool(wkz_depot,
						hausbauer_t::bahn_depot_besch,
						karte_t::Z_PLAN,
						SFX_GAVEL,
						SFX_FAILURE,
						hausbauer_t::bahn_depot_besch->gib_cursor()->gib_bild_nr(1),
						hausbauer_t::bahn_depot_besch->gib_cursor()->gib_bild_nr(0),
						tool_tip_with_price(translator::translate("Build train depot"), umgebung_t::cst_depot_rail));
				}

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::bahnhof,
					wkz_halt,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_station,
					welt );

				if(!grund_t::underground_mode) {
					hausbauer_t::fill_menu(wzw,
						haus_besch_t::bahnhof_geb,
						wkz_station_building,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						umgebung_t::cst_multiply_post,
						welt );
				}
			}
			else {
				create_win( new news_img("Trains are not available yet!"), w_time_delete, magic_none );
			}
		break;

		case magic_monorailtools:
			if(wegbauer_t::weg_search(monorail_wt,1,welt->get_timeline_year_month(),weg_t::type_all)!=NULL  &&  hausbauer_t::monorail_depot_besch!=NULL) {
				if(wzw==NULL) {
					wzw = new werkzeug_parameter_waehler_t(welt, "MONORAILTOOLS");
					wzw->setze_hilfe_datei("monorailtools.txt");
				}
				else {
					wzw->reset_tools();
				}

				if(sp!=welt->gib_spieler(1)  &&  !grund_t::underground_mode) {
					// public player is not allowed to run vehicles ...
					wegbauer_t::fill_menu(wzw,
						monorail_wt,
						wkz_wegebau,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt,
						weg_t::type_flat
						);

					wegbauer_t::fill_menu(wzw,
						monorail_wt,
						wkz_wegebau,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt,
						weg_t::type_elevated
						);

					wayobj_t::fill_menu(wzw,
						monorail_wt,
						wkz_wayobj,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt
						);
				}

				if(sp!=welt->gib_spieler(1)) {
					wzw->add_param_tool(&wkz_wayremover,
						(const int)monorail_wt,
						karte_t::Z_PLAN,
						SFX_REMOVER,
						SFX_FAILURE,
						skinverwaltung_t::edit_werkzeug->gib_bild_nr(8),
						skinverwaltung_t::killzeiger->gib_bild_nr(0),
						translator::translate("remove monorails"));

					if(!grund_t::underground_mode) {
						brueckenbauer_t::fill_menu(wzw,
							monorail_wt,
							SFX_JACKHAMMER,
							SFX_FAILURE,
							welt
							);
					}

					tunnelbauer_t::fill_menu(wzw,
						monorail_wt,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt
						);

					roadsign_t::fill_menu(wzw,
						monorail_wt,
						wkz_roadsign,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt
						);

					wzw->add_param_tool(wkz_depot,
						hausbauer_t::monorail_depot_besch,
						karte_t::Z_PLAN,
						SFX_GAVEL,
						SFX_FAILURE,
						hausbauer_t::monorail_depot_besch->gib_cursor()->gib_bild_nr(1),
						hausbauer_t::monorail_depot_besch->gib_cursor()->gib_bild_nr(0),
						tool_tip_with_price(translator::translate("Build monotrail depot"), umgebung_t::cst_depot_rail));
				}

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::monorailstop,
					wkz_halt,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_station,
					welt );

				if(!grund_t::underground_mode) {
					hausbauer_t::fill_menu(wzw,
						haus_besch_t::monorail_geb,
						wkz_station_building,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						umgebung_t::cst_multiply_station,
						welt );
				}
			}
			else {
				create_win( new news_img("Monorails are not available yet!"), w_time_delete, magic_none );
			}
		break;

		case magic_tramtools:
			if(wegbauer_t::weg_search(tram_wt,1,welt->get_timeline_year_month(),weg_t::type_all)!=NULL  &&  hausbauer_t::tram_depot_besch!=NULL) {
				if(wzw==NULL) {
					wzw = new werkzeug_parameter_waehler_t(welt, "TRAMTOOLS");
					wzw->setze_hilfe_datei("tramtools.txt");
				}
				else {
					wzw->reset_tools();
				}

				if(sp!=welt->gib_spieler(1)  &&  !grund_t::underground_mode) {

					wegbauer_t::fill_menu(wzw,
						track_wt,
						wkz_wegebau,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt,
						weg_t::type_tram
					);

					wayobj_t::fill_menu(wzw,
						track_wt,
						wkz_wayobj,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt
						);

					wzw->add_param_tool(&wkz_wayremover,
						(const int)track_wt,
						karte_t::Z_PLAN,
						SFX_REMOVER,
						SFX_FAILURE,
						skinverwaltung_t::edit_werkzeug->gib_bild_nr(7),
						skinverwaltung_t::killzeiger->gib_bild_nr(0),
						translator::translate("remove tracks"));

					roadsign_t::fill_menu(wzw,
						track_wt,
						wkz_roadsign,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt
						);
				}

				if(sp!=welt->gib_spieler(1)) {
					wzw->add_param_tool(wkz_depot,
						hausbauer_t::tram_depot_besch,
						karte_t::Z_PLAN,
						SFX_GAVEL,
						SFX_FAILURE,
						hausbauer_t::tram_depot_besch->gib_cursor()->gib_bild_nr(1),
						hausbauer_t::tram_depot_besch->gib_cursor()->gib_bild_nr(0),
						tool_tip_with_price(translator::translate("Build tram depot"), umgebung_t::cst_depot_rail));
				}

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::bahnhof,
					wkz_halt,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_station,
					welt );

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::bushalt,
					wkz_halt,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_roadstop,
					welt );
			}
			else {
				create_win( new news_img("Trams are not available yet!"), w_time_delete, magic_none );
			}
		break;

		case magic_roadtools:
			if(wegbauer_t::weg_search(road_wt,1,welt->get_timeline_year_month(),weg_t::type_all)!=NULL  &&  hausbauer_t::str_depot_besch!=NULL) {
				if(wzw==NULL) {
					wzw = new werkzeug_parameter_waehler_t(welt, "ROADTOOLS");
					wzw->setze_hilfe_datei("roadtools.txt");
				}
				else {
					wzw->reset_tools();
				}

				if(!grund_t::underground_mode) {
					wegbauer_t::fill_menu(wzw,
						road_wt,
						wkz_wegebau,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt,
						weg_t::type_flat
						);

					wegbauer_t::fill_menu(wzw,
						road_wt,
						wkz_wegebau,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt,
						weg_t::type_elevated
						);
				}

				wayobj_t::fill_menu(wzw,
					road_wt,
					wkz_wayobj,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					welt
					);

				wzw->add_param_tool(&wkz_wayremover,
					(const int)road_wt,
					karte_t::Z_PLAN,
					SFX_REMOVER,
					SFX_FAILURE,
					skinverwaltung_t::edit_werkzeug->gib_bild_nr(9),
					skinverwaltung_t::killzeiger->gib_bild_nr(0),
					translator::translate("remove roads"));

				if(!grund_t::underground_mode) {
					brueckenbauer_t::fill_menu(wzw,
						road_wt,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt
						);
				}

				tunnelbauer_t::fill_menu(wzw,
					road_wt,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					welt
					);

				roadsign_t::fill_menu(wzw,
					road_wt,
					wkz_roadsign,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					welt
					);

				if(sp!=welt->gib_spieler(1)) {
					wzw->add_param_tool(wkz_depot,
						hausbauer_t::str_depot_besch,
						karte_t::Z_PLAN,
						SFX_GAVEL,
						SFX_FAILURE,
						hausbauer_t::str_depot_besch->gib_cursor()->gib_bild_nr(1),
						hausbauer_t::str_depot_besch->gib_cursor()->gib_bild_nr(0),
						tool_tip_with_price(translator::translate("Build road depot"), umgebung_t::cst_depot_road));
				}

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::bushalt,
					wkz_halt,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_roadstop,
					welt );

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::ladebucht,
					wkz_halt,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_roadstop,
					welt );

				if(!grund_t::underground_mode) {
					hausbauer_t::fill_menu(wzw,
						haus_besch_t::bushalt_geb,
						wkz_station_building,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						umgebung_t::cst_multiply_post,
						welt );

					hausbauer_t::fill_menu(wzw,
						haus_besch_t::ladebucht_geb,
						wkz_station_building,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						umgebung_t::cst_multiply_post,
						welt );
				}
			}
			else {
				create_win( new news_img("Cars are not available yet!"), w_time_delete, magic_none );
			}
		break;

		case magic_shiptools:
		{
			if(wzw==NULL) {
				wzw = new werkzeug_parameter_waehler_t(welt, "SHIPTOOLS");
				wzw->setze_hilfe_datei("shiptools.txt");
			}
			else {
				wzw->reset_tools();
			}

			if(!grund_t::underground_mode) {
				wegbauer_t::fill_menu(wzw,
					water_wt,
					wkz_wegebau,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					welt,
					weg_t::type_flat
					);
			}

			wayobj_t::fill_menu(wzw,
				water_wt,
				wkz_wayobj,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				welt
				);

			wzw->add_param_tool(&wkz_wayremover,
				(const int)water_wt,
				karte_t::Z_PLAN,
				SFX_REMOVER,
				SFX_FAILURE,
				skinverwaltung_t::edit_werkzeug->gib_bild_nr(10),
				skinverwaltung_t::killzeiger->gib_bild_nr(0),
				translator::translate("remove channels"));

			if(!grund_t::underground_mode) {
				brueckenbauer_t::fill_menu(wzw,
					water_wt,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					welt
					);
			}

			tunnelbauer_t::fill_menu(wzw,
				water_wt,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				welt
				);

			roadsign_t::fill_menu(wzw,
				water_wt,
				wkz_roadsign,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				welt
				);

			hausbauer_t::fill_menu(wzw,
				haus_besch_t::binnenhafen,
				wkz_halt,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				umgebung_t::cst_multiply_dock,
				welt );


			if(!grund_t::underground_mode) {
				hausbauer_t::fill_menu(wzw,
					haus_besch_t::hafen,
					wkz_dockbau,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_dock,
					welt );
			}

			if(sp!=welt->gib_spieler(1)) {
				if(skinverwaltung_t::schiffs_werkzeug->gib_bild_nr(0)!=NULL) {
					wzw->add_param_tool(wkz_depot,
						hausbauer_t::sch_depot_besch,
						karte_t::Z_PLAN,
						SFX_DOCK,
						SFX_FAILURE,
						skinverwaltung_t::schiffs_werkzeug->gib_bild_nr(0),
						skinverwaltung_t::werftNSzeiger->gib_bild_nr(0),
						tool_tip_with_price(translator::translate("Build ship depot"), umgebung_t::cst_depot_ship));
				}
				else {
					wzw->add_param_tool(wkz_depot,
						hausbauer_t::sch_depot_besch,
						karte_t::Z_PLAN,
						SFX_DOCK,
						SFX_FAILURE,
						hausbauer_t::sch_depot_besch->gib_cursor()->gib_bild_nr(1),
						hausbauer_t::sch_depot_besch->gib_cursor()->gib_bild_nr(0),
						tool_tip_with_price(translator::translate("Build ship depot"), umgebung_t::cst_depot_ship));
				}
			}

			if(!grund_t::underground_mode) {
				hausbauer_t::fill_menu(wzw,
					haus_besch_t::binnenhafen_geb,
					wkz_station_building,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_post,
					welt );

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::hafen_geb,
					wkz_station_building,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_post,
					welt );
			}
		}
		break;

		case magic_airtools:
			if (!hausbauer_t::air_depot.empty() && wegbauer_t::weg_search(air_wt, 1, welt->get_timeline_year_month(),weg_t::type_all) != NULL) {

				if(grund_t::underground_mode) {
					if(wzw) {
						destroy_win( wzw );
						return NULL;
					}
				}

				if(wzw==NULL) {
					wzw = new werkzeug_parameter_waehler_t(welt, "AIRTOOLS");
					wzw->setze_hilfe_datei("airtools.txt");
				}
				else {
					wzw->reset_tools();
				}

				wegbauer_t::fill_menu(wzw,
					air_wt,
					wkz_wegebau,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					welt,
					weg_t::type_flat
					);

				wegbauer_t::fill_menu(wzw,
					air_wt,
					wkz_wegebau,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					welt,
					(weg_t::system_type)1
					);

				wayobj_t::fill_menu(wzw,
					air_wt,
					wkz_wayobj,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					welt
					);

				roadsign_t::fill_menu(wzw,
					air_wt,
					wkz_roadsign,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					welt
					);

				wzw->add_param_tool(&wkz_wayremover,
					(const int)air_wt,
					karte_t::Z_PLAN,
					SFX_REMOVER,
					SFX_FAILURE,
					skinverwaltung_t::edit_werkzeug->gib_bild_nr(11),
					skinverwaltung_t::killzeiger->gib_bild_nr(0),
					translator::translate("remove airstrips"));

				if(sp!=welt->gib_spieler(1)) {
					hausbauer_t::fill_menu(wzw,
						hausbauer_t::air_depot,
						wkz_depot,
						SFX_GAVEL,
						SFX_FAILURE,
						umgebung_t::cst_multiply_airterminal,
						welt );
				}

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::airport,
					wkz_halt,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_airterminal,
					welt );

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::airport_geb,
					wkz_station_building,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_post,
					welt );
			}
			else {
				create_win( new news_img("Planes are not available yet!"), w_time_delete, magic_none);
			}
		break;

		case magic_specialtools:
			if(!grund_t::underground_mode) {
				if(wzw==NULL) {
					wzw = new werkzeug_parameter_waehler_t(welt, "SPECIALTOOLS");
					wzw->setze_hilfe_datei("special.txt");
				}
				else {
					wzw->reset_tools();
				}

				wegbauer_t::fill_menu(wzw,
					road_wt,
					wkz_wegebau,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					welt,
					weg_t::type_all
					);

				wegbauer_t::fill_menu(wzw,
					track_wt,
					wkz_wegebau,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					welt,
					weg_t::type_all
					);

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::post,
					wkz_station_building,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_post,
					welt );

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::wartehalle,
					wkz_station_building,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_post,
					welt );

				hausbauer_t::fill_menu(wzw,
					haus_besch_t::lagerhalle,
					wkz_station_building,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					umgebung_t::cst_multiply_post,
					welt );

				wzw->add_tool(wkz_switch_player,
					karte_t::Z_PLAN,
					-1,
					-1,
					skinverwaltung_t::edit_werkzeug->gib_bild_nr(4),
					IMG_LEER,
					translator::translate("Change player") );

				wzw->add_tool(wkz_add_city,
					karte_t::Z_PLAN,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::special_werkzeug->gib_bild_nr(0),
					skinverwaltung_t::stadtzeiger->gib_bild_nr(0),
					tool_tip_with_price(translator::translate("Found new city"), umgebung_t::cst_found_city));

				wzw->add_tool(wkz_pflanze_baum,
					karte_t::Z_PLAN,
					SFX_SELECT,
					SFX_FAILURE,
					skinverwaltung_t::special_werkzeug->gib_bild_nr(6),
					skinverwaltung_t::baumzeiger->gib_bild_nr(0),
					translator::translate("Plant tree"));

				if(wegbauer_t::leitung_besch) {
					char buf[128];
					const sint32 shift_maintanance = (welt->ticks_bits_per_tag-18);

					sprintf(buf, "%s, %ld$ (%ld$)",
						translator::translate("Build powerline"),
						wegbauer_t::leitung_besch->gib_preis()/100l,
						(wegbauer_t::leitung_besch->gib_wartung()<<shift_maintanance)/100l
						);

					brueckenbauer_t::fill_menu(wzw,
						powerline_wt,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						welt
						);

					wzw->add_param_tool(wkz_wegebau,
						(const void *)wegbauer_t::leitung_besch,
						karte_t::Z_PLAN,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						wegbauer_t::leitung_besch->gib_cursor()->gib_bild_nr(1),
						wegbauer_t::leitung_besch->gib_cursor()->gib_bild_nr(0),
						buf
						);

					sprintf(buf, "%s, %ld$ (%ld$)",
						translator::translate("Build drain"),
						(long)(umgebung_t::cst_transformer/-100l),
						(long)(umgebung_t::cst_maintain_transformer<<shift_maintanance)/-100l
						);

					wzw->add_tool(wkz_senke,
						karte_t::Z_PLAN,
						SFX_JACKHAMMER,
						SFX_FAILURE,
						skinverwaltung_t::special_werkzeug->gib_bild_nr(2),
						skinverwaltung_t::undoc_zeiger->gib_bild_nr(0),
						buf);
				}

				wzw->add_tool(wkz_marker,
					karte_t::Z_PLAN,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::special_werkzeug->gib_bild_nr(5),
					skinverwaltung_t::belegtzeiger->gib_bild_nr(0),
					translator::translate("Marker"));
			}
			else if( wzw!=NULL ) {
				destroy_win( wzw );
				wzw = NULL;
			}
		break;

		case magic_listtools:
		{
			if(wzw==NULL) {
				wzw = new werkzeug_parameter_waehler_t(welt, "LISTTOOLS");
				wzw->setze_hilfe_datei("list.txt");
			}
			else {
				wzw->reset_tools();
			}

			wzw->add_tool(wkz_list_halt_tool,
				karte_t::Z_PLAN,
				-1,
				-1,
				skinverwaltung_t::listen_werkzeug->gib_bild_nr(0),
				IMG_LEER,
				translator::translate("hl_title"));

			wzw->add_tool(wkz_list_vehicle_tool,
				karte_t::Z_PLAN,
				-1,
				-1,
				skinverwaltung_t::listen_werkzeug->gib_bild_nr(1),
				IMG_LEER,
				translator::translate("cl_title"));

			wzw->add_tool(wkz_list_town_tool,
				karte_t::Z_PLAN,
				-1,
				-1,
				skinverwaltung_t::listen_werkzeug->gib_bild_nr(2),
				IMG_LEER,
				translator::translate("tl_title"));

			wzw->add_tool(wkz_list_good_tool,
				karte_t::Z_PLAN,
				-1,
				-1,
				skinverwaltung_t::listen_werkzeug->gib_bild_nr(3),
				IMG_LEER,
				translator::translate("gl_title"));

			wzw->add_tool(wkz_list_factory_tool,
				karte_t::Z_PLAN,
				-1,
				-1,
				skinverwaltung_t::listen_werkzeug->gib_bild_nr(4),
				IMG_LEER,
				translator::translate("fl_title"));

			wzw->add_tool(wkz_list_curiosity_tool,
				karte_t::Z_PLAN,
				-1,
				-1,
				skinverwaltung_t::listen_werkzeug->gib_bild_nr(5),
				IMG_LEER,
				translator::translate("curlist_title"));
		}
		break;

		case magic_edittools:
		{
			if(wzw==NULL) {
				wzw = new werkzeug_parameter_waehler_t(welt, "EDITTOOLS");
				wzw->setze_hilfe_datei("edittools.txt");
			}
			else {
				wzw->reset_tools();
			}

			wzw->add_param_tool(&wkz_grow_city,
				(long)100,
				karte_t::Z_PLAN,
				-1,
				SFX_FAILURE,
				skinverwaltung_t::edit_werkzeug->gib_bild_nr(0),
				skinverwaltung_t::upzeiger->gib_bild_nr(0),
				translator::translate("Grow city") );

			wzw->add_param_tool(&wkz_grow_city,
				(long)-100,
				karte_t::Z_PLAN,
				-1,
				SFX_FAILURE,
				skinverwaltung_t::edit_werkzeug->gib_bild_nr(1),
				skinverwaltung_t::downzeiger->gib_bild_nr(0),
				translator::translate("Shrink city") );

			// cityroads
			const weg_besch_t *besch = welt->get_city_road();
			if(besch!=NULL) {
				char buf[512];
				sprintf(buf, "%s, %ld$ (%ld$), %dkm/h",
					translator::translate(besch->gib_name()),
					besch->gib_preis()/100,
					besch->gib_wartung()/100,
					besch->gib_topspeed());

				wzw->add_param_tool(&wkz_wegebau,
					(const void *)besch,
					karte_t::Z_PLAN,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					skinverwaltung_t::edit_werkzeug->gib_bild_nr(2),
					strasse_t::default_strasse->gib_cursor()->gib_bild_nr(0),
					buf);
			}

			wzw->add_tool(&wkz_add_attraction,
				karte_t::Z_PLAN,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				skinverwaltung_t::edit_werkzeug->gib_bild_nr(3),
				skinverwaltung_t::undoc_zeiger->gib_bild_nr(0),
				translator::translate("Built random attraction") );

			wzw->add_tool(wkz_build_industries_land,
				karte_t::Z_PLAN,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				skinverwaltung_t::special_werkzeug->gib_bild_nr(3),
				skinverwaltung_t::undoc_zeiger->gib_bild_nr(0),
				translator::translate("Build land consumer"));

			wzw->add_tool(wkz_build_industries_city,
				karte_t::Z_PLAN,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				skinverwaltung_t::special_werkzeug->gib_bild_nr(4),
				skinverwaltung_t::undoc_zeiger->gib_bild_nr(0),
				translator::translate("Build city market"));

			wzw->add_tool(wkz_lock,
				karte_t::Z_PLAN,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				skinverwaltung_t::edit_werkzeug->gib_bild_nr(5),
				skinverwaltung_t::edit_werkzeug->gib_bild_nr(5),
				translator::translate("Lock game"));

			wzw->add_tool(wkz_step_year,
				karte_t::Z_PLAN,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				skinverwaltung_t::edit_werkzeug->gib_bild_nr(6),
				skinverwaltung_t::undoc_zeiger->gib_bild_nr(0),
				translator::translate("Step timeline one year"));
		}
		break;
	}
	// windows newly opened?
	if(wzw!=NULL  &&  do_sound) {
		sound_play(click_sound);
	}
	return wzw;
}
