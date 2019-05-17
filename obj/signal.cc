/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../gui/simwin.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "../simobj.h"
#include "../boden/wege/schiene.h"
#include "../boden/grund.h"
#include "../display/simimg.h"
#include "../dataobj/ribi.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../utils/cbuffer_t.h"
#include "../obj/gebaeude.h"
#include "../simsignalbox.h"
#include "../descriptor/building_desc.h"
#include "../simhalt.h"
#include "../utils/simstring.h"

#include "../gui/signal_info.h"

#include "signal.h"


signal_t::signal_t( loadsave_t *file) :
#ifdef INLINE_OBJ_TYPE
	roadsign_t(obj_t::signal, file)
#else
	roadsign_t(file)
#endif
{
	rdwr_signal(file);
	if(desc==NULL) {
		desc = roadsign_t::default_signal;
	}
}

signal_t::signal_t(player_t *player, koord3d pos, ribi_t::ribi dir,const roadsign_desc_t *desc, koord3d sb, bool preview) : roadsign_t(obj_t::signal, player, pos, dir, desc, preview)
{
	if((desc->get_working_method() == time_interval || desc->get_working_method() == time_interval_with_telegraph) && !desc->is_choose_sign() && !desc->is_longblock_signal())
	{
		state = clear;
	}
	else if(desc->is_pre_signal())
	{
		// Distant signals do not display a danger aspect.
		state = caution;
	}
	else
	{
		state = danger;
	}

	train_last_passed = 0;
	no_junctions_to_next_signal = false;

	if(desc->get_signal_group())
	{
		const grund_t* gr = welt->lookup(sb);
		if(gr)
		{
			gebaeude_t* gb = gr->get_building();
			if(gb && gb->get_tile()->get_desc()->is_signalbox())
			{
				signalbox_t* sigb = (signalbox_t*)gb;
				signalbox = sb;
				sigb->add_signal(this); 
			}
		}
	}

	if(desc->is_longblock_signal() && (desc->get_working_method() == time_interval || desc->get_working_method() == time_interval_with_telegraph || desc->get_working_method() == absolute_block))
	{
		// Register station signals at the halt.
		halthandle_t halt = haltestelle_t::get_halt(pos, player);
		if(halt.is_bound())
		{
			halt->add_station_signal(pos);
		}
	}
}

signal_t::~signal_t()
{
	const grund_t* gr = welt->lookup(signalbox);
	if(gr)
	{
		gebaeude_t* gb = gr->get_building();
		if(gb && gb->get_tile()->get_desc()->is_signalbox())
		{
			signalbox_t* sigb = (signalbox_t*)gb;
			sigb->remove_signal(this); 
		}
	}
	welt->remove_time_interval_signal_to_check(this); 
	if(desc->is_longblock_signal() && (desc->get_working_method() == time_interval || desc->get_working_method() == time_interval_with_telegraph))
	{
		// De-register station signals at the halt.
		halthandle_t halt = haltestelle_t::get_halt(get_pos(), get_owner());
		if(halt.is_bound())
		{
			halt->remove_station_signal(get_pos());
		}
	}
}

void signal_t::show_info()
{
	create_win(new signal_info_t(this), w_info, (ptrdiff_t)this);
}


/**
* @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
* Beobachtungsfenster angezeigt wird.
* "return a description string for the object , such as the in a
* Observation window is displayed." (Google)
* @author Hj. Malthaner
*/

void signal_t::info(cbuffer_t & buf, bool dummy) const
{
	// well, needs to be done
	obj_t::info(buf);
	signal_t* sig = (signal_t*)this;

	bool is_station_signal = desc->is_longblock_signal() && (desc->get_working_method() == time_interval || desc->get_working_method() == time_interval_with_telegraph || desc->get_working_method() == absolute_block);
	
	buf.append(translator::translate(desc->get_name()));
	buf.append("\n\n");

	buf.append(translator::translate(get_working_method_name(desc->get_working_method())));
	buf.append("\n");

	if (desc->is_choose_sign())
	{
		buf.append(translator::translate("choose_signal"));
		buf.append("\n");
	}
	if (desc->get_double_block() == true)
	{
		buf.append(translator::translate("double_block_signal"));
		buf.append("\n");
	}
	if (desc->is_longblock_signal() && (desc->get_working_method() == time_interval || desc->get_working_method() == time_interval_with_telegraph || desc->get_working_method() == absolute_block))
	{
		buf.append(translator::translate("station_signal"));
		buf.append("\n");
	}
	else if (desc->is_longblock_signal())
	{
		buf.append(translator::translate("longblock_signal"));
		buf.append("\n");
	}
	if (desc->is_combined_signal())
	{
		buf.append(translator::translate("combined_signal"));
		buf.append("\n");
	}
	if (desc->is_pre_signal())
	{
		buf.append(translator::translate("distant_signal"));
		buf.append("\n");
	}
	if (desc->get_intermediate_block() == true)
	{
		buf.append(translator::translate("intermediate_signal"));
		buf.append("\n");
	}
	if (desc->get_permissive() == true)
	{
		buf.append(translator::translate("permissive_signal"));
		buf.append("\n");
	}

	koord3d sig_pos = sig->get_pos();
	const grund_t *sig_gr = welt->lookup_kartenboden(sig_pos.x, sig_pos.y);

	if (sig_gr->get_hoehe() > sig_pos.z == true)
	{
		buf.append(translator::translate("underground_signal"));
		buf.append("\n");
	}
	if (desc->get_working_method() == drive_by_sight)
	{
		const sint32 max_speed_drive_by_sight = welt->get_settings().get_max_speed_drive_by_sight();
		if (max_speed_drive_by_sight && get_desc()->get_waytype() != tram_wt)
		{
			buf.printf("%s%s%d%s%s", translator::translate("Max. speed:"), " ", speed_to_kmh(max_speed_drive_by_sight), " ", "km/h");
			buf.append("\n");
		}
	}
	else
	{

		buf.printf("%s%s%d%s%s", translator::translate("Max. speed:"), " ", speed_to_kmh(desc->get_max_speed()), " ", "km/h");
		buf.append("\n");

		const grund_t* sig_gr3d = welt->lookup(sig_pos);
		const weg_t* way = sig_gr3d->get_weg(desc->get_wtyp() != tram_wt ? desc->get_wtyp() : track_wt);
		//	if (way->get_max_speed() * 2 >= speed_to_kmh(desc->get_max_speed()))  // Wether this information only shall be shown when the track speed is close to or above the max speed of the signal
		//	{
		buf.printf("%s%s%s%d%s%s%s", "(", translator::translate("track_speed"), ": ", way->get_max_speed(), " ", "km/h", ")");
		buf.append("\n");
		//	}
	}

	if (desc->get_maintenance() > 0)
	{
		char maintenance_number[64];
		money_to_string(maintenance_number, (double)welt->calc_adjusted_monthly_figure(desc->get_maintenance()) / 100.0);
		buf.printf("%s%s", translator::translate("maintenance"), ": ");
		buf.append(maintenance_number);
	}
	else
	{
		buf.append(translator::translate("no_maintenance_costs"));
	}
	buf.append("\n");

	buf.append(translator::translate("Direction"));
	buf.append(": ");
	if (desc->is_longblock_signal() && (desc->get_working_method() == time_interval || desc->get_working_method() == time_interval_with_telegraph || desc->get_working_method() == absolute_block))
	{
		if (get_dir() == 1 || get_dir() == 4)
		{
			buf.append(translator::translate("north_and_south"));
		}
		else
		{
			buf.append(translator::translate("east_and_west"));
		}
	}
	else
	{
		buf.append(translator::translate(get_directions_name(get_dir())));
	}
	buf.append("\n");

	// Show the signal state in the info window // Ves
	// Does this signal have any aspects to show?
	if (desc->get_working_method() == drive_by_sight || desc->get_aspects() <= 1)
	{
	}
	else
	{
		// yay!
		buf.append(translator::translate("aspects"));
		buf.append(": ");
		if (desc->is_longblock_signal() && desc->get_working_method() == absolute_block)
		{
			buf.append("2");
			// A station signal using the absolute block working method returns the value from get_aspect() as 3, but it has practically only 2 aspects.
		}
		else
		{
			buf.append(desc->get_aspects());
		}
		if (desc->is_choose_sign() || desc->get_permissive() == true)
		{
			buf.append(" + ");
			if (desc->is_choose_sign() && desc->get_permissive() == true)
			{
				buf.append(translator::translate("alt_route_and_callon"));
			}
			else if (desc->is_choose_sign())
			{
				buf.append(translator::translate("alt_route"));
			}
			else
			{
				buf.append(translator::translate("callon"));
			}
			
		}

		buf.append("\n");

		buf.append(translator::translate("current_state"));
		buf.append(": ");
		if (get_state() != danger)
		{
			// Is this signal a presignal?
			if (desc->is_pre_signal())
			{
				buf.append(translator::translate(get_pre_signal_aspects_name(get_state())));
			}
			else
			{
				// Is this a "station signal"?
				if (desc->is_longblock_signal() && (desc->get_working_method() == time_interval || desc->get_working_method() == time_interval_with_telegraph || desc->get_working_method() == absolute_block))
				{
					// Is the station signal using a 'normal' working method?
					if (desc->get_working_method() != time_interval && desc->get_working_method() != time_interval_with_telegraph)
					{
						switch (get_dir())
						{
						case 1:
							if (get_state() == clear_no_choose || get_state() == caution_no_choose)
								buf.printf("%s (%s)", translator::translate(get_3_signal_aspects_name(get_state())), translator::translate("south"));
							else
								buf.printf("%s (%s)", translator::translate(get_3_signal_aspects_name(get_state())), translator::translate("north"));
							break;
						case 2:
							if (get_state() == clear_no_choose || get_state() == caution_no_choose)
								buf.printf("%s (%s)", translator::translate(get_3_signal_aspects_name(get_state())), translator::translate("west"));
							else
								buf.printf("%s (%s)", translator::translate(get_3_signal_aspects_name(get_state())), translator::translate("east"));
							break;
						case 4:
							if (get_state() == clear_no_choose || get_state() == caution_no_choose)
								buf.printf("%s (%s)", translator::translate(get_3_signal_aspects_name(get_state())), translator::translate("north"));
							else
								buf.printf("%s (%s)", translator::translate(get_3_signal_aspects_name(get_state())), translator::translate("south"));
							break;
						case 8:
							if (get_state() == clear_no_choose || get_state() == caution_no_choose)
								buf.printf("%s (%s)", translator::translate(get_3_signal_aspects_name(get_state())), translator::translate("east"));
							else
								buf.printf("%s (%s)", translator::translate(get_3_signal_aspects_name(get_state())), translator::translate("west"));
							break;
						}
					}
					// Nope, it is indeed using one of the time interval working methods!
					else
					{
						switch (get_dir())
						{
						case 1:
							if (get_state() == clear_no_choose || get_state() == caution_no_choose)
								buf.printf("%s (%s)", translator::translate(get_time_signal_aspects_name(get_state())), translator::translate("south"));
							else
								buf.printf("%s (%s)", translator::translate(get_time_signal_aspects_name(get_state())), translator::translate("north"));
							break;
						case 2:
							if (get_state() == clear_no_choose || get_state() == caution_no_choose)
								buf.printf("%s (%s)", translator::translate(get_time_signal_aspects_name(get_state())), translator::translate("west"));
							else
								buf.printf("%s (%s)", translator::translate(get_time_signal_aspects_name(get_state())), translator::translate("east"));
							break;
						case 4:
							if (get_state() == clear_no_choose || get_state() == caution_no_choose)
								buf.printf("%s (%s)", translator::translate(get_time_signal_aspects_name(get_state())), translator::translate("north"));
							else
								buf.printf("%s (%s)", translator::translate(get_time_signal_aspects_name(get_state())), translator::translate("south"));
							break;
						case 8:
							if (get_state() == clear_no_choose || get_state() == caution_no_choose)
								buf.printf("%s (%s)", translator::translate(get_time_signal_aspects_name(get_state())), translator::translate("east"));
							else
								buf.printf("%s (%s)", translator::translate(get_time_signal_aspects_name(get_state())), translator::translate("west"));
							break;
						}
					}
				}
				// is it a choose signal?
				else if (desc->is_choose_sign())
				{
					if (desc->get_working_method() == time_interval || (desc->get_working_method() == time_interval_with_telegraph))
					{
						buf.append(translator::translate(get_time_choose_signal_aspects_name(get_state())));
					}
					else
						switch (desc->get_aspects())
						{
						case 2:
							buf.append(translator::translate(get_2_choose_signal_aspects_name(get_state())));
							break;
						case 3:
							buf.append(translator::translate(get_3_choose_signal_aspects_name(get_state())));
							break;
						case 4:
							buf.append(translator::translate(get_4_choose_signal_aspects_name(get_state())));
							break;
						case 5:
							buf.append(translator::translate(get_5_choose_signal_aspects_name(get_state())));
							break;
						};
				}
				else
					// Time interval or time interval with telegraph?
					if (desc->get_working_method() == time_interval || (desc->get_working_method() == time_interval_with_telegraph))
					{
						buf.append(translator::translate(get_time_signal_aspects_name(get_state())));
					}
					else // just a normal lousy signal then..
						switch (desc->get_aspects())
						{
						case 2:
							buf.append(translator::translate(get_2_signal_aspects_name(get_state())));
							break;
						case 3:
							buf.append(translator::translate(get_3_signal_aspects_name(get_state())));
							break;
						case 4:
							buf.append(translator::translate(get_4_signal_aspects_name(get_state())));
							break;
						case 5:
							buf.append(translator::translate(get_5_signal_aspects_name(get_state())));
							break;
						};
			}
		}
		else
			buf.append(translator::translate("danger"));
			buf.append(translator::translate("\n"));
	}
	if (get_state() != danger)
	{
		// Possible information about how many blocks are reserved, or speed restrictions for time interval signals
	}
	buf.append(translator::translate("\n"));

	if (((desc->is_longblock_signal() || get_dir() == 3 || get_dir() == 6 || get_dir() == 9 || get_dir() == 12 || get_dir() == 5 || get_dir() == 10) && (desc->get_working_method() == time_interval_with_telegraph || desc->get_working_method() == track_circuit_block || desc->get_working_method() == cab_signalling || desc->get_working_method() == moving_block)) && (desc->is_pre_signal()) == false)
	{
		buf.append(translator::translate("This signal creates directional reservations"));
		buf.append(translator::translate("\n\n"));
	}

	if (desc->get_double_block() == true)
	{
		buf.append(translator::translate("this_signal_will_not_clear_until_next_signal_is_clear"));
		buf.append(translator::translate("\n\n"));
	}

	// Check wether this signal protects a junction otherwise count how far the next stop signal is
	// However, display nothing for station signals and one train staffs, since the information would be very random dependent on where you put the signal and would not be very informative anyway.
	if (!is_station_signal && desc->get_working_method() != one_train_staff)
	{
		const waytype_t waytype = sig->get_waytype();
		uint8 initial_direction = get_dir();
		uint8 initial_direction_2 = get_dir();
		uint8 directions = 1;
		uint8 coming_from_direction = get_dir();
		int tiles = 0;
		bool dead_end = false;
		bool crossing = false;
		bool signal = false;
		int max_tiles_to_look = 1000;
		char direction[20];

		{
			// If signal is double headed, save the directions for each head.
			switch (get_dir())
			{
			case 3: // north_and_east
				initial_direction = 1;
				initial_direction_2 = 2;
				directions = 2;
				break;
			case 5: // north_and_south
				initial_direction = 1;
				initial_direction_2 = 4;
				directions = 2;
				break;
			case 6: // south_and_east
				initial_direction = 4;
				initial_direction_2 = 2;
				directions = 2;
				break;
			case 9: // north_and_west
				initial_direction = 1;
				initial_direction_2 = 8;
				directions = 2;
				break;
			case 10: // east_and_west
				initial_direction = 2;
				initial_direction_2 = 8;
				directions = 2;
				break;
			case 12: // south_and_west
				initial_direction = 4;
				initial_direction_2 = 8;
				directions = 2;
				break;
			default:
				break;
			}
			// Otherwise just reorder the directions so that the RIBI understands them.
			initial_direction = get_dir() == 1 ? 4 : get_dir() == 2 ? 8 : get_dir() == 4 ? 1 : get_dir() == 8 ? 2 : initial_direction;
		}

		// Signal get_dir() values
		//1 = South
		//2 = West
		//4 = North
		//8 = East

		// Ribi enum direction values
		// 1 = North
		// 4 = South
		// 2 = East
		// 8 = West

		//bool track_pos_z = false;
		grund_t *gr;
		koord3d weg_pos;
		for (int j = 0; j < directions; j++)
		{
			// If the signal is doubleheaded we need to display a heading for the entry
			initial_direction = j == 1 ? initial_direction_2 : initial_direction;
			directions == 2 ? sprintf(direction, " (%s)", translator::translate(get_directions_name(initial_direction == 1 ? 4 : initial_direction == 2 ? 8 : initial_direction == 4 ? 1 : initial_direction == 8 ? 2 : initial_direction))) : sprintf(direction, "");

			gr = welt->lookup(get_pos());
			tiles = 0;
			coming_from_direction = get_dir();
			crossing = false;
			dead_end = false;
			signal = false;
			for (uint32 i = 0; i < max_tiles_to_look; i++)
			{
				grund_t* to;
				dead_end = true;
				for (int r = 0; r < 4; r++)
				{
					if (((ribi_t::nsew[r] & initial_direction) != 0 || (ribi_t::nsew[r] & coming_from_direction) == 0) && gr->get_neighbour(to, waytype, ribi_t::nsew[r]))
					{
						weg_t* weg = to->get_weg(waytype);
						weg_pos = weg->get_pos();
						gr = welt->lookup(weg_pos);
						initial_direction = 0; // reset initial direction and start record what direction we came from by reversing the direction in which we are traveling
						uint8 new_from_direction = r == 0 ? 1 : r == 1 ? 0 : r == 2 ? 3 : r == 3 ? 2 : 0;
						coming_from_direction = ribi_t::nsew[new_from_direction];
						dead_end = false;

						// Is this new tile a junction of some sorth?
						ribi_t::ribi ribi = weg->get_ribi_unmasked();
						switch (ribi)
						{
						case ribi_t::_ribi::northsoutheast:
						case ribi_t::_ribi::northeastwest:
						case ribi_t::_ribi::northsouthwest:
						case ribi_t::_ribi::southeastwest:
						case ribi_t::_ribi::all:
							crossing = true;
							break;
						//default:
							//break;
						}

						// Does this tile have a signal?
						if (weg->has_signal())
						{
							signal_t* next_signal = gr->find<signal_t>();
							if (next_signal)
							{
								const roadsign_desc_t * next_desc = next_signal->get_desc();
								// Next signal is only a presignal?
								if (!next_desc->is_pre_signal())
								{
									// Determine if the signal are facing the wrong way.
									uint8 sig_dir = next_signal->get_dir();
									uint8 sig_ribi_dir;

									if (next_desc->is_longblock_signal() && (next_desc->get_working_method() == time_interval || next_desc->get_working_method() == time_interval_with_telegraph || next_desc->get_working_method() == absolute_block))
									{
										// Station signal, always true.
										signal = true;
										break;
									}
									else
									{
										// If signal is double headed, also assume always true
										switch (sig_dir)
										{
										case 3: // north_and_east
										case 5: // north_and_south
										case 6: // south_and_east
										case 9: // north_and_west
										case 10: // east_and_west
										case 12: // south_and_west
											signal = true;
											break;
											break; // second break, to get out of the for loop
										default:
											break;
										}
										// Single headed. Reorder the directions so the RIBI understands them.
										sig_ribi_dir = sig_dir == 1 ? 4 : sig_dir == 2 ? 8 : sig_dir == 4 ? 1 : sig_dir == 8 ? 2 : sig_ribi_dir;
										if (ribi_t::nsew[r] == sig_ribi_dir)
										{
											signal = true;
											break;
										}
									}									
								}
							}
						}
						break;
					}
				}
				tiles++;
				if (crossing || dead_end || signal)
				{
					break;
				}
			}

			// Convert the tiles counted to actual distance
			const double km_per_tile = welt->get_settings().get_meters_per_tile() / 1000.0;
			const double distance_km = (double)tiles * km_per_tile;
			char distance[10];

			if (distance_km < 1)
			{
				float distance_m = distance_km * 1000;
				sprintf(distance, "%i%s", (int)distance_m, "m");
			}
			else
			{
				uint decimal;
				if (distance_km < 20)
				{
					decimal = 1;
				}
				else
				{
					decimal = 0;
				}
				char number_actual[10];
				number_to_string(number_actual, distance_km, decimal);
				sprintf(distance, "%s%s", number_actual, "km");
			}

			if (crossing)
			{
				buf.printf("%s%s: %s\n", translator::translate("this_signal_protects_a_junction"), direction, distance);
			}
			else if (signal)
			{
				buf.printf("%s%s: %s\n", translator::translate("distance_to_next_stop_signal"), direction, distance);
			}
			else if (dead_end)
			{
				buf.printf("%s%s: %s\n", translator::translate("distance_to_dead_end"), direction, distance);
			}
		}
		buf.append("\n");
	}

	// Deal with station signals where the time since the train last passed is standardised for the whole station.
	halthandle_t this_tile_halt = haltestelle_t::get_halt(sig->get_pos(), get_owner()); 
	uint32 station_signals_count = this_tile_halt.is_bound() ? this_tile_halt->get_station_signals_count() : 0;
	if(station_signals_count)
	{
		for(uint32 i = 0; i < 4; i ++)
		{
			switch(i)
			{
			case 0:
			default:
				buf.append(translator::translate("Time since a train last passed")); 
				buf.append(":\n");
				buf.append(translator::translate("north"));
				break;
			case 1:
				buf.append(translator::translate("south"));
				break;
			case 2:
				buf.append(translator::translate("east"));
				break;
			case 3:
				buf.append(translator::translate("west"));
			};
			buf.append(": "); 
			char time_since_train_last_passed[32];
			welt->sprintf_ticks(time_since_train_last_passed, sizeof(time_since_train_last_passed), welt->get_ticks() - this_tile_halt->get_train_last_departed(i));
			buf.append(time_since_train_last_passed);		
			buf.append("\n");
		}
	}
	else
	{
		buf.append(translator::translate("Time since a train last passed")); 
		buf.append(": "); 
		char time_since_train_last_passed[32];
		welt->sprintf_ticks(time_since_train_last_passed, sizeof(time_since_train_last_passed), welt->get_ticks() - sig->get_train_last_passed());
		buf.append(time_since_train_last_passed);
		buf.append("\n");
	}
	buf.append("\n");

	if (desc->get_working_method() == moving_block)
	{
		uint32 mdt_mbs = desc->get_max_distance_to_signalbox(); // Max distance to Moving Block Signal
		buf.append(translator::translate("max_dist_between_signals"));
		buf.append(": ");
		if (mdt_mbs == 0)
		{
			buf.append(translator::translate("infinite_range"));
		}
		else
		{
			if (mdt_mbs < 1000)
			{
				buf.append(mdt_mbs);
				buf.append("m");
			}

			else
			{
				uint n_max;
				const double max_dist_mov = (double)mdt_mbs / 1000;
				if (max_dist_mov < 20)
				{
					n_max = 1;
				}
				else
				{
					n_max = 0;
				}
				char number_max_mov[10];
				number_to_string(number_max_mov, max_dist_mov, n_max);
				buf.append(number_max_mov);
				buf.append("km");

			}
		}
		buf.append("\n\n");
	}

	
	buf.append(translator::translate("Controlled from"));
	buf.append(":\n");
	koord3d sb = sig->get_signalbox();
	if(sb == koord3d::invalid)
	{
		buf.append(translator::translate("keine"));
	}
	else
	{
		const grund_t* gr = welt->lookup(sb);
		if(gr)
		{
			const gebaeude_t* gb = gr->get_building();
			if(gb)
			{
				uint8 textlines = 2; // to locate the clickable signalbox button
				const grund_t *ground = welt->lookup_kartenboden(sb.x, sb.y);
				bool sb_underground = ground->get_hoehe() > sb.z;

				buf.append("   ");
				buf.append(translator::translate(gb->get_name()));
				if (sb_underground)
				{
					buf.append("\n  ");
					textlines += 1;
				}
				buf.append(" <");
				buf.append(sb.x);
				buf.append(",");
				buf.append(sb.y);
				buf.append(">"); 
				if (sb_underground)
				{
					buf.printf(" (%s)", translator::translate("underground"));
				}
				buf.append("\n   ");
				
				// Show the distance between the signal and its signalbox, along with the signals maximum range
				koord3d sigpos = sig->get_pos();
				const uint32 tiles_to_signalbox = shortest_distance(sigpos.get_2d(), sb.get_2d());
				const double km_per_tile = welt->get_settings().get_meters_per_tile() / 1000.0;
				const double km_to_signalbox = (double)tiles_to_signalbox * km_per_tile;

				if (km_to_signalbox < 1)
				{
					float m_to_signalbox = km_to_signalbox * 1000;
					buf.append(m_to_signalbox);
					buf.append("m");
				}
				else
				{
					uint n_actual;
					if (km_to_signalbox < 20)
					{
						n_actual = 1;
					}
					else
					{
						n_actual = 0;
					}
					char number_actual[10];
					number_to_string(number_actual, km_to_signalbox, n_actual);
					buf.append(number_actual);
					buf.append("km");
				}

				if (desc->get_working_method() != moving_block)
				{
					buf.append(" (");

					uint32 mdt_sb = desc->get_max_distance_to_signalbox();

					if (mdt_sb == 0)
					{
						buf.append(translator::translate("infinite_range"));
					}
					else
					{
						if (mdt_sb < 1000)
						{
							buf.printf("%s: ", translator::translate("max"));
							buf.append(mdt_sb);
							buf.append("m");
						}

						else
						{
							uint n_max;
							const double max_dist = (double)mdt_sb / 1000;
							if (max_dist < 20)
							{
								n_max = 1;
							}
							else
							{
								n_max = 0;
							}
							char number_max[10];
							number_to_string(number_max, max_dist, n_max);
							buf.printf("%s: ", translator::translate("max"));
							buf.append(number_max);
							buf.append("km");
						}
					}
					buf.append(")");
				}

				sig->textlines_in_signal_window = textlines;
			}
			else
			{
				buf.append(translator::translate("keine"));
				dbg->warning("signal_t::info()", "Signalbox could not be found from a signal on valid ground");
			}
		}
		else
		{
			buf.append(translator::translate("keine"));
			dbg->warning("signal_t::info()", "Signalbox could not be found from a signal on valid ground");
		}
	}
}


void signal_t::calc_image()
{
	foreground_image = IMG_EMPTY;
	image_id image = IMG_EMPTY;
	after_xoffset = 0;
	after_yoffset = 0;
	sint8 xoff = 0, yoff = 0;
	const bool left_swap = welt->get_settings().is_signals_left()  &&  desc->get_offset_left();

	grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		set_flag(obj_t::dirty);
		const slope_t::type full_hang = gr->get_weg_hang();
		const sint8 hang_diff = slope_t::max_diff(full_hang);
		const ribi_t::ribi hang_dir = ribi_t::backward( ribi_type(full_hang) );
		
		const sint8 height_step = TILE_HEIGHT_STEP << slope_t::is_doubles(gr->get_weg_hang());

		weg_t *sch = gr->get_weg(desc->get_wtyp()!=tram_wt ? desc->get_wtyp() : track_wt);
		if(sch) 
		{
			uint16 number_of_signal_image_types = desc->get_aspects(); 
			if(desc->get_has_call_on())
			{
				number_of_signal_image_types += 1;
			}
			if(desc->get_has_selective_choose())
			{
				number_of_signal_image_types += desc->get_aspects() - 1;
			}
			uint16 offset = 0;
			ribi_t::ribi dir = sch->get_ribi_unmasked() & (~calc_mask());
			if(sch->is_electrified()  &&  desc->get_count() >= number_of_signal_image_types * 8) // 8: Four directions per aspect * 2 types (electrified and non-electrified) per aspect
			{
				offset = number_of_signal_image_types * 4;
			}

			// vertical offset of the signal positions
			if(full_hang==slope_t::flat) {
				yoff = -gr->get_weg_yoff();
				after_yoffset = yoff;
			}
			else {
				const ribi_t::ribi test_hang = left_swap ? ribi_t::backward(hang_dir) : hang_dir;
				if(test_hang==ribi_t::east ||  test_hang==ribi_t::north) {
					yoff = -TILE_HEIGHT_STEP*hang_diff;
					after_yoffset = 0;
				}
				else {
					yoff = 0;
					after_yoffset = -TILE_HEIGHT_STEP*hang_diff;
				}
			}

			// and now calculate the images:
			// we need to hide the "second" image on tunnel entries
			ribi_t::ribi temp_dir = dir;
			if(  gr->get_typ()==grund_t::tunnelboden  &&  gr->ist_karten_boden()  &&
				(grund_t::underground_mode==grund_t::ugm_none  ||  (grund_t::underground_mode==grund_t::ugm_level  &&  gr->get_hoehe()<grund_t::underground_level))   ) {
				// entering tunnel here: hide the image further in if not undergroud/sliced
				const ribi_t::ribi tunnel_hang_dir = ribi_t::backward( ribi_type(gr->get_grund_hang()) );
				if(  tunnel_hang_dir==ribi_t::east ||  tunnel_hang_dir==ribi_t::north  ) {
					temp_dir &= ~ribi_t::southwest;
				}
				else {
					temp_dir &= ~ribi_t::northeast;
				}
			}

			uint8 modified_state = state;
			const sint8 diff = 5 - desc->get_aspects(); 
			if(desc->get_has_call_on())
			{
				if(desc->get_has_selective_choose())
				{
					if(desc->get_aspects() < 5)
					{
						if(state > advance_caution)
						{
							modified_state = state - diff;
						}
					}
				}
				else
				{
					if(state > advance_caution)
					{
						modified_state -= (desc->get_aspects() - 1) + diff;
					}
				}
			}

			if(state == call_on && !desc->get_has_call_on())
			{
				modified_state = danger;
			}

			if (state == call_on && desc->get_has_call_on())
			{
				if (desc->get_has_selective_choose())
				{
					modified_state = desc->get_aspects() + (desc->get_aspects() - 1);
				}
				else
				{
					modified_state = desc->get_aspects();
				}
			}

			if(state == clear_no_choose && !desc->get_has_selective_choose())
			{
				modified_state = clear;
			}

			if(state == caution_no_choose && !desc->get_has_selective_choose())
			{
				modified_state = caution;
			}

			if(state == preliminary_caution_no_choose && !desc->get_has_selective_choose())
			{
				modified_state = preliminary_caution;
			}

			if(state == advance_caution_no_choose && !desc->get_has_selective_choose())
			{
				modified_state = advance_caution;
			}

			if(desc->is_pre_signal() && desc->get_aspects() == 2 && state == caution)
			{
				modified_state = danger;
			}

			if(desc->get_aspects() == 1)
			{
				modified_state = danger;
			}

			if(desc->get_aspects() == 2 && !desc->is_pre_signal() && !desc->is_choose_sign() && state > clear && !desc->get_has_call_on())
			{
				modified_state = clear;
			}

			if(desc->get_has_selective_choose() && desc->get_aspects() < 5 && state >= clear_no_choose)
			{
				modified_state -= diff; 
			}

			const schiene_t* sch1 = (schiene_t*)sch; 
			ribi_t::ribi reserved_direction = sch1->get_reserved_direction();
			if(desc->is_longblock_signal() && (desc->get_working_method() == time_interval || desc->get_working_method() == time_interval_with_telegraph))
			{
				// Allow both directions for a station signal
				//reserved_direction |= ribi_t::backward(reserved_direction);
				
				// The above does not work because the reservation is taken from the track below the signal, and the train might be departing from another track. 
				// Which state, if any, to set should be handled by the block reserver in the case of station signals. 
				reserved_direction = ribi_t::all;
			}
			// signs for left side need other offsets and other front/back order
			if(  left_swap  ) {
				const sint16 XOFF = 2*desc->get_offset_left();
				const sint16 YOFF = desc->get_offset_left();

				if(temp_dir&ribi_t::east) {
					uint8 direction_state = (reserved_direction & ribi_t::east) ? modified_state * 4 : 0;
					image = desc->get_image_id(3+direction_state+offset);
					xoff += XOFF;
					yoff += -YOFF;
				}

				if(temp_dir&ribi_t::north) {
					uint8 direction_state = (reserved_direction & ribi_t::north) ? modified_state * 4 : 0;
					if(image!=IMG_EMPTY) {			
						foreground_image = desc->get_image_id(0+direction_state+offset);
						after_xoffset += -XOFF;
						after_yoffset += -YOFF;
					}
					else {
						image = desc->get_image_id(0+direction_state+offset);
						xoff += -XOFF;
						yoff += -YOFF;
					}
				}

				if(temp_dir&ribi_t::west) {
					uint8 direction_state = (reserved_direction & ribi_t::west) ? modified_state * 4 : 0;
					foreground_image = desc->get_image_id(2+direction_state+offset);
					after_xoffset += -XOFF;
					after_yoffset += YOFF;
				}

				if(temp_dir&ribi_t::south) {
					uint8 direction_state = (reserved_direction & ribi_t::south) ? modified_state * 4 : 0;
					if(foreground_image!=IMG_EMPTY) {
						image = desc->get_image_id(1+direction_state+offset);
						xoff += XOFF;
						yoff += YOFF;
					}
					else {
						foreground_image = desc->get_image_id(1+direction_state+offset);
						after_xoffset += XOFF;
						after_yoffset += YOFF;
					}
				}
			}
			else {
				if(temp_dir&ribi_t::east) {
					uint8 direction_state = (reserved_direction & ribi_t::east) ? modified_state * 4 : 0;
					foreground_image = desc->get_image_id(3+direction_state+offset);
				}

				if(temp_dir&ribi_t::north) {
					uint8 direction_state = (reserved_direction & ribi_t::north) ? modified_state * 4 : 0;
					if(foreground_image==IMG_EMPTY) {
						foreground_image = desc->get_image_id(0+direction_state+offset);
					}
					else {
						image = desc->get_image_id(0+direction_state+offset);
					}
				}

				if(temp_dir&ribi_t::west) {
					uint8 direction_state = (reserved_direction & ribi_t::west) ? modified_state * 4 : 0;
					image = desc->get_image_id(2+direction_state+offset);
				}

				if(temp_dir&ribi_t::south) {
					uint8 direction_state = (reserved_direction & ribi_t::south) ? modified_state * 4 : 0;
					if(image==IMG_EMPTY) {
						image = desc->get_image_id(1+direction_state+offset);
					}
					else {
						foreground_image = desc->get_image_id(1+direction_state+offset);
					}
				}
			}
		}
	}
	set_xoff( xoff );
	set_yoff( yoff );
	set_image(image);
	mark_image_dirty( get_image(), 0 );
}

void signal_t::rdwr_signal(loadsave_t *file)
{
#ifdef SPECIAL_RESCUE_12_5
	if(file->get_extended_version() >= 12 && file->is_saving())
#else
	if(file->get_extended_version() >= 12)
#endif
	{
		signalbox.rdwr(file);

		uint8 state_full = state;
		file->rdwr_byte(state_full); 
		state = state_full;
		
		bool ignore_choose_full = ignore_choose;
		file->rdwr_bool(ignore_choose_full);
		ignore_choose = ignore_choose_full; 
#ifdef SPECIAL_RESCUE_12_6
		if(file->is_saving())
		{
#endif
		file->rdwr_bool(no_junctions_to_next_signal);
		if (file->is_loading() && desc && desc->is_choose_sign())
		{
			no_junctions_to_next_signal = false;
		}
		file->rdwr_longlong(train_last_passed); 
#ifdef SPECIAL_RESCUE_12_6
		}
#endif
	}

	if(no_junctions_to_next_signal && desc && (desc->get_working_method() == time_interval || desc->get_working_method() == time_interval_with_telegraph) && (state == caution || state == caution_no_choose || state == danger))
	{
		welt->add_time_interval_signal_to_check(this); 
	}
}

void signal_t::rotate90()
{
	signalbox.rotate90(welt->get_size().y-1); 
	dir = ribi_t::rotate90(dir);
	obj_t::rotate90();
}