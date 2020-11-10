/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
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

	if(desc->is_station_signal())
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
	if(desc->is_station_signal())
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

void signal_t::info(cbuffer_t & buf) const
{
	// well, needs to be done
	obj_t::info(buf);

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
	if (desc->is_station_signal())
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

	koord3d sig_pos = this->get_pos();
	const grund_t *sig_gr = welt->lookup_kartenboden(sig_pos.x, sig_pos.y);

	if (sig_gr->get_hoehe() > sig_pos.z)
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
	if (desc->is_station_signal())
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
		const grund_t* sig_gr3d = welt->lookup(sig_pos);
		const weg_t* way = sig_gr3d->get_weg(desc->get_wtyp() != tram_wt ? desc->get_wtyp() : track_wt);
		uint8 direction = get_dir();

		ribi_t::ribi ribi = way->get_ribi_unmasked();

		// If signal is single headed, we need to alter the direction for diagonals, as those will display wrong
		if (get_dir() == 1 || get_dir() == 2 || get_dir() == 4 || get_dir() == 8)
		{
			switch (ribi)
			{
			case ribi_t::_ribi::northeast:
				direction = get_dir() == 1 ? 8 : 4;
				break;
			case ribi_t::_ribi::northwest:
				direction = get_dir() == 1 ? 2 : 4;
				break;
			case ribi_t::_ribi::southeast:
				direction = get_dir() == 4 ? 8 : 1;
				break;
			case ribi_t::_ribi::southwest:
				direction = get_dir() == 4 ? 2 : 1;
				break;
			default:
				break;
			}
		}
		buf.append(translator::translate(get_directions_name(direction)));
	}
	buf.append("\n");

	// Show the signal state in the info window // Ves
	// Does this signal have any aspects to show?
	if (desc->get_working_method() == drive_by_sight || desc->get_aspects() <= 1)
	{
		buf.append("\n\n");
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
				if (desc->is_station_signal())
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
		{
			buf.append(translator::translate("danger"));
		}
	}
	buf.append(translator::translate("\n"));
	if (get_state() == danger)
	{
	}
	else
	{
		// Signal is not at danger! Display info about the reservation that is made through this signal
	}
	buf.append(translator::translate("\n"));
		// When the signal is at danger, we display some properties of the signal.
		// Check wether this signal protects a junction otherwise count how far the next stop signal is
		// However, display nothing for station signals, one train staffs and drive by sight, since the information would be very random dependent on where you put the signal and would not be very informative anyway.
	if (!desc->is_station_signal() && desc->get_working_method() != one_train_staff && desc->get_working_method() != drive_by_sight)
	{
		const waytype_t waytype = this->get_waytype();
		uint8 initial_direction = get_dir();
		uint8 initial_direction_2 = get_dir();
		uint8 directions = 1;
		uint8 coming_from_direction = get_dir();
		uint8 blocks_amount = desc->get_aspects();
		int tiles = 0;
		bool dead_end = false;
		bool crossing = false;
		bool signal = false;
		uint32 max_tiles_to_look = 1000;
		char direction[20];
		char spaces[5];
		char block_text[20];

		if (desc->get_working_method() == track_circuit_block || (desc->get_working_method() == absolute_block && desc->is_combined_signal()))
		{
			blocks_amount = desc->get_aspects() - 1;
		}
		else
		{
			blocks_amount = 1;
		}

		// Reorder the directions so that the RIBI understands them.
		/*
		* Signal get_dir() values
		* 1 = South
		* 2 = West
		* 4 = North
		* 8 = East
		--------------------
		* Ribi enum direction values
		* 1 = North
		* 4 = South
		* 2 = East
		* 8 = West
		*/
		initial_direction =
			get_dir() == 1 ? 4 : /*South*/
			get_dir() == 2 ? 8 : /*West*/
			get_dir() == 3 ? 1 : /*north_and_east*/
			get_dir() == 4 ? 1 : /*North*/
			get_dir() == 5 ? 1 : /*north_and_south*/
			get_dir() == 6 ? 4 : /*south_and_east*/
			get_dir() == 8 ? 2 : /*East*/
			get_dir() == 9 ? 1 : /*north_and_west*/
			get_dir() == 10 ? 2 : /*east_and_west*/
			get_dir() == 12 ? 4 : /*south_and_west*/
			initial_direction;

		// For bidirectional signals, add the "backwards" direction to initial_direction_2.
		initial_direction_2 =
			get_dir() == 3 ? 2 : /*north_and_east*/
			get_dir() == 5 ? 4 : /*north_and_south*/
			get_dir() == 6 ? 2 : /*south_and_east*/
			get_dir() == 9 ? 8 : /*north_and_west*/
			get_dir() == 10 ? 8 : /*east_and_west*/
			get_dir() == 12 ? 8 : /*south_and_west*/
			0;

		// Set the amount of directions this signal has (1 or 2)
		directions = initial_direction_2 > 0 ? 2 : 1;

		grund_t *gr;
		signal_t* previous_signal = NULL;
		for (int j = 0; j < directions; j++)
		{
			// If the signal is doubleheaded, apply the new "initial_direction" and write the direction to char
			initial_direction = j == 1 ? initial_direction_2 : initial_direction;
			if (directions == 2) {
				sprintf(direction,  "%s:", translator::translate(get_directions_name(
					initial_direction == 1 ? 4 :
					initial_direction == 2 ? 8 :
					initial_direction == 4 ? 1 :
					initial_direction == 8 ? 2 :
					initial_direction)));
				sprintf(spaces, "  ");
			}
			else {
				direction[0] = '\0';
				spaces[0] = '\0';
			}

			coming_from_direction = get_dir();
			tiles = 0;
			crossing = false;
			dead_end = false;
			previous_signal = NULL;
			for (int b = 0; b < blocks_amount; b++)
			{
				gr = previous_signal ? welt->lookup(previous_signal->get_pos()) : welt->lookup(get_pos());
				if (blocks_amount > 1) {
					sprintf(block_text, translator::translate(" (block %i)"), b + 1);
				}
				else {
					block_text[0] = '\0';
				}

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
							gr = welt->lookup(weg->get_pos());
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
										// Determine if the signal are facing the right way.
										uint8 sig_dir = next_signal->get_dir();
										uint8 sig_ribi_dir = 0;

										if (next_signal->get_desc()->is_station_signal())
										{
											// Station signal, always true.
											signal = true;
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
											default:
												break;
											}
											// Single headed. Reorder the directions so the RIBI understands them.
											sig_ribi_dir =
												sig_dir == 1 ? 4 :
												sig_dir == 2 ? 8 :
												sig_dir == 4 ? 1 :
												sig_dir == 8 ? 2 :
												sig_ribi_dir;

											if (ribi_t::nsew[r] == sig_ribi_dir)
											{
												signal = true;
											}
											if (signal)
											{
												previous_signal = next_signal;
												break;
											}
										}
									}
								}
							}
							break;
						}
					}
					// If the last tile is a signal or a dead end, count that tile as well. The 'break' will make sure we dont count the tile twice
					// It it is a crossing, dont count the tile since the crossing starts at the edge of the tile
					if (signal)
					{
						tiles++;
						break;
					}
					if (dead_end)
					{
						tiles++;
						previous_signal = NULL;
						break;
					}
					if (crossing)
					{
						previous_signal = NULL;
						break;
					}
					tiles++;
				}

				// Convert the tiles counted to actual distance
				const double km_per_tile = welt->get_settings().get_meters_per_tile() / 1000.0;
				const double distance_km = (double)tiles * km_per_tile;
				char distance[20];

				if (distance_km < 1)
				{
					float distance_m = distance_km * 1000;
					sprintf(distance, "%i%s", (int)distance_m, "m");
				}
				else
				{
					uint decimal = distance_km < 20 ? 1 : 0;
					char number_actual[10];
					number_to_string(number_actual, distance_km, decimal);
					sprintf(distance, "%s%s", number_actual, "km");
				}

				if (direction[0] != '\0')
				{
					buf.printf("%s\n", direction);
					direction[0] = '\0';
				}

				if (crossing)
				{
					// If there is no signal before the first crossing, erase the "(block X)" display
					if (b == 0)
					{
						block_text[0] = '\0';
					}

					// We want to emphasize the crossings importance in time interval (with telegraph) working method, by explicitly saying that the signal is protecting the crossing
					// However, presignals in that WM would not protect the crossing, so just tell the distance.
					if (!desc->is_pre_signal() && (desc->get_working_method() == time_interval || desc->get_working_method() == time_interval_with_telegraph))
					{
						buf.printf("%s%s (%s)\n", spaces, translator::translate("this_signal_protects_a_junction"), distance);
					}
					else
					{
						buf.printf("%s%s%s: %s\n", spaces, translator::translate("distance_to_junction"), block_text, distance);
					}

					break; // break out of the "block counts"
				}
				else if (signal)
				{
					buf.printf("%s%s%s: %s\n", spaces, translator::translate("distance_to_stop_signal"), block_text, distance);
				}
				else if (dead_end)
				{
					// A presignal in this position would be pointles. Tell the player!
					if (desc->is_pre_signal() && b == 0)
					{
						buf.printf("%s (%s)\n", spaces, translator::translate("no_signal_before_dead_end"), distance);
						break; // break out of the "block counts"
					}
					else
					{
						if (b == 0)
						{
							block_text[0] = '\0';
						}

						buf.printf("%s%s%s: %s\n", spaces, translator::translate("distance_to_dead_end"), block_text, distance);
						break; // break out of the "block counts"
					}
				}
			}
			buf.append("\n");
		}
	}

	if (((desc->is_longblock_signal() || get_dir() == 3 || get_dir() == 6 || get_dir() == 9 || get_dir() == 12 || get_dir() == 5 || get_dir() == 10) && (desc->get_working_method() == time_interval_with_telegraph || desc->get_working_method() == track_circuit_block || desc->get_working_method() == cab_signalling || desc->get_working_method() == moving_block)) && (desc->is_pre_signal()) == false)
	{
		buf.append(translator::translate("This signal creates directional reservations"));
		buf.append(translator::translate("\n"));
		buf.append(translator::translate("\n"));
	}

	if (desc->get_double_block() == true)
	{
		buf.append(translator::translate("this_signal_will_not_clear_until_next_signal_is_clear"));
		buf.append(translator::translate("\n"));
		buf.append(translator::translate("\n"));
	}

	// Deal with station signals where the time since the train last passed is standardised for the whole station.
	halthandle_t this_tile_halt = haltestelle_t::get_halt(this->get_pos(), get_owner());
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
		welt->sprintf_ticks(time_since_train_last_passed, sizeof(time_since_train_last_passed), welt->get_ticks() - this->get_train_last_passed());
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

	// "Controlled from signalbox" section. The "\n" is in this first section set before the entry to help accommodate the layout in the name, coordinates etc.
	buf.append(translator::translate("Controlled from"));
	buf.append(":");
	koord3d sb = this->get_signalbox();
	if(sb == koord3d::invalid)
	{
		buf.append("\n");
		buf.append(translator::translate("keine"));
		buf.append("\n");
	}
	else
	{
		const grund_t* gr = welt->lookup(sb);
		if (gr)
		{
			const gebaeude_t* gb = gr->get_building();
			if (gb)
			{
				uint8 textlines = 1; // to locate the clickable signalbox button
				const grund_t *ground = welt->lookup_kartenboden(sb.x, sb.y);
				bool sb_underground = ground->get_hoehe() > sb.z;

				char sb_coordinates[20];
				sprintf(sb_coordinates, "<%i,%i>", sb.x, sb.y);
				buf.printf("  %s", sb_coordinates);
				if (sb_underground)
				{
					buf.printf(" (%s)", translator::translate("underground"));
				}
				char sb_name[1024] = { '\0' };
				int max_width = 250;
				int max_lines = 5; // Set a limit
				sprintf(sb_name, "%s", translator::translate(gb->get_name()));

				//sprintf(sb_name,"This is a very very long signal box name which is so long that no one remembers what it was actually called before the super long name of the signalbox got changed to its current slightly longer name which is still too long to display in only one line therefore splitting this very long signalbox name into several lines although maximum five lines which should suffice more than enough to guard against silly long signal box names");
				int next_char_index = 0;

				for (int l = 0; l < max_lines; l++)
				{
					textlines += 1;
					char temp_name[1024] = { '\0' };
					next_char_index = display_fit_proportional(sb_name, max_width, 0);

					if (sb_name[next_char_index] == '\0')
					{
						buf.append("\n   ");
						buf.append(sb_name);
						break;
					}
					else
					{
						for (int i = 0; i < next_char_index; i++)
						{
							temp_name[i] = sb_name[i];
						}
						buf.append("\n   ");
						buf.append(temp_name);
						if (l + 1 == max_lines)
						{
							buf.append("...");
						}

						for (int i = 0; sb_name[i] != '\0'; i++)
						{
							sb_name[i] = sb_name[i + next_char_index];
						}
					}

				}
				buf.append("\n   ");

				// Show the distance between the signal and its signalbox, along with the signals maximum range
				const uint32 tiles_to_signalbox = shortest_distance(get_pos().get_2d(), sb.get_2d());
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

				this->textlines_in_signal_window = textlines;
			}
			else
			{
				buf.append("\n");
				buf.append(translator::translate("keine"));
				dbg->warning("signal_t::info()", "Signalbox could not be found from a signal on valid ground");
			}
		}
		else
		{
			buf.append("\n");
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
			if(desc->is_station_signal())
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
