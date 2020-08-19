/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>
#include <cmath>
#include "../../utils/simstring.h"
#include "../../dataobj/tabfile.h"
#include "../vehicle_desc.h"
#include "../sound_desc.h"
#include "../../boden/wege/weg.h"
#include "obj_pak_exception.h"
#include "obj_node.h"
#include "text_writer.h"
#include "xref_writer.h"
#include "imagelist_writer.h"
#include "imagelist2d_writer.h"
#include "imagelist3d_writer.h"
#include "get_waytype.h"
#include "vehicle_writer.h"

using std::string;

/**
 * Calculate numeric engine type from engine type string
 */
static vehicle_desc_t::engine_t get_engine_type(char const* const engine_type)
{
	vehicle_desc_t::engine_t uv8 = vehicle_desc_t::unknown;

	if (!STRICMP(engine_type, "diesel")) {
		uv8 = vehicle_desc_t::diesel;
	} else if (!STRICMP(engine_type, "electric")) {
		uv8 = vehicle_desc_t::electric;
	} else if (!STRICMP(engine_type, "steam")) {
		uv8 = vehicle_desc_t::steam;
	} else if (!STRICMP(engine_type, "bio")) {
		uv8 = vehicle_desc_t::bio;
	} else if (!STRICMP(engine_type, "sail")) {
		uv8 = vehicle_desc_t::sail;
	} else if (!STRICMP(engine_type, "fuel_cell")) {
		uv8 = vehicle_desc_t::fuel_cell;
	} else if (!STRICMP(engine_type, "hydrogene")) {
		uv8 = vehicle_desc_t::hydrogene;
	} else if (!STRICMP(engine_type, "battery")) {
		uv8 = vehicle_desc_t::battery;
	} else if (!STRICMP(engine_type, "petrol")) {
		uv8 = vehicle_desc_t::petrol;
	} else if (!STRICMP(engine_type, "turbine")) {
		uv8 = vehicle_desc_t::turbine;
	} else if (!STRICMP(engine_type, "unknown")) {
		uv8 = vehicle_desc_t::unknown;
	}

	// printf("Engine type %s -> %d\n", engine_type, uv8);

	return uv8;
}


/**
 * Writes vehicle node data to file
 *
 * NOTE: The data must be written in _exactly_
 * the same sequence as it is to be read in the
 * relevant reader file. The "total_len" field is
 * the length in bytes of the VHCL node of the
 * pak file. The VHCL node is the first node
 * beneath the header node, and contains all of
 * the _numerical_ information about the vehicle,
 * such as the introduction date, running costs,
 * etc.. Text (including filenames of sound files),
 * and graphics are _not_ part of the VHCL node,
 * and therefore do not count towards total length.
 * Furthermore, the third argument to the node.write
 * method must ascend sequentially with the number
 * of bytes written so far (up 1 for a uint8, 2 for
 * a uint16, 4 for a uint32 and so forth). Failure
 * to observe these rules will result in data
 * corruption and errors when the pak file is read
 * by the main program.
 * @author of note: jamespetts
 */
void vehicle_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	int i;
	uint8  uv8;

	int total_len = 88;

	// prissi: must be done here, since it may affect the len of the header!
	string sound_str = ltrim( obj.get("sound") );
	sint16 sound_id=NO_SOUND;
	if (!sound_str.empty()) {
		// ok, there is some sound
		sound_id = atoi(sound_str.c_str());
		if (sound_id == 0 && sound_str[0] == '0') {
			sound_id = 0;
			sound_str = "";
		}
		else if (sound_id != 0) {
			// old style id
			sound_str = "";
		}
		if (!sound_str.empty()) {
			sound_id = LOAD_SOUND;
			total_len += sound_str.size() + 1;
		}
	}

	// The maximum payload of the vehicle, by each class that the vehicle
	// accommodates. If this is not specified, class 0 will be assumed.

	// This must be done here, as it will affect the length of the header.

	uint32 current_class_capacity;
	vector_tpl<uint16> class_capacities(1);

	uint8 j = 0;
	do
	{
		// Check for multiple classes with a separate capacity each
		char buf[13];
		sprintf(buf, "payload[%u]", j);
		current_class_capacity = obj.get_int(buf, UINT32_MAX_VALUE);
		if (current_class_capacity == UINT32_MAX_VALUE)
		{
			if (j == 0)
			{
				// If there are no class defined capacities, look for the old/basic
				// keyword defining a single capacity.
				class_capacities.append(obj.get_int("payload", 0));
			}
			else
			{
				// Increase the length of the header by 2 for each additional
				// class capacity stored: one for capacity and one for comfort.
				total_len += 2;
			}
			break;
		}
		else
		{
			// Increase the length of the header by 2 for each additional
			// class capacity stored: one for capacity and one for comfort.
			total_len += 2;
			class_capacities.append(current_class_capacity);
		}
	} while (j++ < 255);


	obj_node_t node(this, total_len, &parent);

	write_head(fp, node, obj);

	// This variable counts the number of bytes written so far in the file semi-automatically.
	// It is important that data be read back in exactly the same order as they were written.
	uint16 pos = 0;

	// Hajo: version number
	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversioned
	uint16 version = 0x800B;

	// This is the overlay flag for Simutrans-Extended
	// This sets the *second* highest bit to 1.
	version |= EX_VER;

	// Finally, this is the extended version number. This is *added*
	// to the standard version number, to be subtracted again when read.
	// Start at 0x100 and increment in hundreds (hex).
	// Counting can restart at 0x100 if the Standard version increases.
	// Standard 10, 0x100 - everything from minimum runway length and earlier.
	// Standard 10, 0x200 - range, wear factor.
	// Standard 10, 0x300 - whether tall (for height restricted bridges)
	// Standard 11, 0x400 - 16-bit sound
	// Standard 11, 0x500 - classes
	// Standard 11, 0x600 - prev=any, cab_setting
	// Standard 11, 0x700 - prev=any, cab_setting
	version += 0x700;

	node.write_uint16(fp, version, pos);

	pos += sizeof(uint16);

	// Hajodoc: Price of this vehicle in Simucents
	uint32 cost = obj.get_int("cost", 0);
	node.write_uint32(fp, cost, pos);
	pos += sizeof(uint32);

	// The number of classes.
	uint8 number_of_classes = min(255, class_capacities.get_count());
	node.write_uint8(fp, number_of_classes, pos);
	pos += sizeof(uint8);

	// Capacities by class
	FOR(vector_tpl<uint16>, capacity, class_capacities)
	{
		node.write_uint16(fp, capacity, pos);
		pos += sizeof(uint16);
	}

	/*
	* This duplicates a system used in Extended for some time.
	* The keyword is the same.
	*
	// ms per loading/unloading everything
	uint16 loading_time = obj.get_int("loading_time", 1000 );
	node.write_uint16(fp, loading_time, pos);
	pos += sizeof(uint16);
	*/

	// Hajodoc: Top speed of this vehicle. Must be greater than 0
	uint16 topspeed  = obj.get_int("speed", 0);
	node.write_uint16(fp, topspeed, pos);
	pos += sizeof(topspeed);

	// Hajodoc: Total weight of this vehicle in tons
	const char *weight_str = obj.get("weight");
	uint32 weight = (uint32)(atof( weight_str )*1000.0 + 0.5);
	node.write_uint32(fp, weight, pos);
	pos += sizeof(uint32);

	char const* const waytype_name = obj.get("waytype");
	waytype_t   const waytype      = get_waytype(waytype_name);

	// For automatic calculation of axle load
	// (optional). This value is not written to file.
	const uint8 axles_default = waytype == water_wt || waytype == maglev_wt ? 1 : 2;
	const uint8 axles = obj.get_int("axles", axles_default);

	// axle_load (determine ways usage)
	uint16 axle_load = obj.get_int("axle_load", (weight / axles) / 1000);
	node.write_uint16(fp, axle_load, pos);
	pos += sizeof(uint16);

	// Hajodoc: Power of this vehicle in KW
	uint32 power = obj.get_int("power", 0);
	node.write_uint32(fp, power, pos);
	pos += sizeof(uint32);

	// Hajodoc: Running costs, given in cent per square
	uint16 running_cost = obj.get_int("runningcost", 0);
	node.write_uint16(fp, running_cost, pos);
	pos += sizeof(uint16);

	/*
	* uint16 size used in Standard. Stored in a different position
	* in the file in Extended, and is a uint32.
	*
	// monthly maintenance
	uint16 fixed_cost = obj.get_int("fixed_cost", 0);
	node.write_uint16(fp, fixed_cost, pos);
	pos += sizeof(uint16);*/

	// Hajodoc: Introduction date (year * 12 + month)
	uint16 intro_date = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;
	node.write_uint16(fp, intro_date, pos);
	pos += sizeof(uint16);

	// Hajodoc: retire date (year * 12 + month)
	uint16 retire_date = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire_date += obj.get_int("retire_month", 1) - 1;
	node.write_uint16(fp, retire_date, pos);
	pos += sizeof(uint16);

	// Hajodoc: Engine gear (power multiplier)
	uint16 gear = (obj.get_int("gear", 100) * 64) / 100;
	node.write_uint16(fp, gear, pos);
	pos += sizeof(uint16);

	// Hajodoc: Type of way this vehicle drives on
	// These need to be earlier for the purposes of axle loads.
	// char const* const waytype_name = obj.get("waytype");
	// waytype_t   const waytype      = get_waytype(waytype_name);
	uv8 = waytype != overheadlines_wt ? waytype : track_wt;
	node.write_uint8(fp, uv8, pos);
	pos += sizeof(uint8);

	// sound id byte
	node.write_sint16(fp, sound_id, pos);
	pos += sizeof(sint16); // Was sint8

	// engine
	if (waytype == overheadlines_wt) {
		// Hajo: compatibility for old style DAT files
		uv8 = vehicle_desc_t::electric;
	}
	else {
		const char* engine_type = obj.get("engine_type");
		uv8 = get_engine_type(engine_type);
	}
	node.write_uint8(fp, uv8, pos);
	pos += sizeof(uint8);

	// the length (default 8)
	uint8 length = obj.get_int("length", 8);
	node.write_uint8(fp, length, pos);
	pos += sizeof(uint8);

	// Hajodoc: The freight type
	// Hajoval: string
	const char* freight = obj.get("freight");
	if (!*freight) {
		freight = "None";
	}
	xref_writer_t::instance()->write_obj(fp, node, obj_good, freight, true);
	xref_writer_t::instance()->write_obj(fp, node, obj_smoke, obj.get("smoke"), false);

	// Now comes the Image-list
	static const char* const dir_codes[] = {
		"s", "w", "sw", "se", "n", "e", "ne", "nw"
	};
	slist_tpl<string> emptykeys;
	slist_tpl<slist_tpl<string> > freightkeys;
	slist_tpl<string> freightkeys_old;
	slist_tpl<slist_tpl<string> > liverykeys_empty;
	slist_tpl<slist_tpl<string> > liverykeys_freight_old;
	slist_tpl<slist_tpl<slist_tpl<string> > > liverykeys_freight;
	string str;

	int  freight_image_type  = 0;
	int	 livery_max	  = 0;
	bool has_8_images = false;

	// first: find out how many freight and liveries
	bool multiple_livery_freight_images_detected = false;
	for (int j = 0; j < 127; j++)
	{
		// Freight with multiple types and multiple liveries
		for (i = 0; i < 127; i++)
		{
			char buf[64];
			sprintf(buf, "freightimage[%d][%s][%d]", i, dir_codes[0], j);
			str = obj.get(buf);
			if (str.empty())
			{
				freight_image_type = max(freight_image_type, i);

				// Do not increment livery_max here, as this will double count:
				// liveries for loaded vehicles should be the same as for unloaded vehicles,
				// and the liveries will have been counted already in empty images.
				break;
			}
			else
			{
				multiple_livery_freight_images_detected = true;
			}
		}
		if (!multiple_livery_freight_images_detected)
		{
			break;
		}
	}

	if (!multiple_livery_freight_images_detected)
	{
		for (i = 0; i < 127; i++)
		{
			// Freight with multiple types and single liveries
			char buf[40];
			sprintf(buf, "freightimage[%d][%s]", i, dir_codes[0]);
			str = obj.get(buf);
			if (str.empty())
			{
				freight_image_type = i;
				break;
			}
		}
	}

	for (i = 0; i < 127; i++)
	{
		// Empty images with multiple liveries
		char buf[40];
		sprintf(buf, "emptyimage[%s][%d]", dir_codes[0], i);
		str = obj.get(buf);
		if(str.empty())
		{
			livery_max += i;
			break;
		}
	}

	// Freight with a single type and multiple liveries
	uint8 basic_freight_images = 0;
	for (i = 0; i < 127; i++)
	{
		char buf[40];
		sprintf(buf, "freightimage[%s][%d]", dir_codes[0], i);
		str = obj.get(buf);
		if(str.empty())
		{
			// Do not increment livery_max here, as this will double count:
			// liveries for loaded vehicles should be the same as for unloaded vehicles,
			// and the liveries will have been counted already in empty images.
			basic_freight_images = i;
			break;
		}
	}

	// Freight with a single type and single liveries
	for (i = 0; i < 127; i++)
	{
		char buf[40];
		sprintf(buf, "freightimage[%s]", dir_codes[0]);
		str = obj.get(buf);
		if(str.empty())
		{
			break;
		}
	}

	// now load the images strings
	for (i = 0; i < 8; i++)
	{
		char buf[64];

		// Hajodoc: Empty vehicle image for direction, direction in "s", "w", "sw", "se", asymmetric vehicles need also "n", "e", "ne", "nw"
		if (livery_max == 0)
		{
			// Empty images without multiple liveries
			sprintf(buf, "emptyimage[%s]", dir_codes[i]);
			str = obj.get(buf);
			if(!str.empty())
			{
				emptykeys.append(str);
				if (i >= 4)
				{
					has_8_images = true;
				}
			}
			else
			{
				// stop when empty string is found
				break;
			}
		}
		else
		{
			// Empty images with multiple liveries
			liverykeys_empty.append();
			for(int livery = 0; livery < livery_max; livery++)
			{
				sprintf(buf, "emptyimage[%s][%d]", dir_codes[i], livery);
				str = obj.get(buf);
				if(!str.empty())
				{
					liverykeys_empty.at(i).append(str);
					if (i >= 4)
					{
						has_8_images = true;
					}
				}
				else
				{
					// stop when empty string is found
					break;
				}

			}
		}

		if (freight_image_type == 0 && livery_max == 0)
		{
			// a single freight image
			// old style definition - just [direction]
			sprintf(buf, "freightimage[%s]", dir_codes[i]);
			str = obj.get(buf);
			if(!str.empty())
			{
				printf("Appending freightimage[%s]\n", dir_codes[i]);
				freightkeys_old.append(str);
			}
		}
		else if (freight_image_type > 0 && livery_max == 0)
		{
			freightkeys.append();
			for(int freight = 0; freight < freight_image_type; freight++)
			{
				sprintf(buf, "freightimage[%d][%s]", freight, dir_codes[i]);
				str = obj.get(buf);
				if (str.empty()) {
					dbg->fatal( "Vehicle", "Missing freightimage[%d][%s]!", freight, dir_codes[i]);
					exit(1);
				}
				freightkeys.at(i).append(str);
			}
		}
		else if(freight_image_type == 0 && livery_max > 0)
		{
			// a single freight image
			// old style definition - just [direction]
			// With liveries
			liverykeys_freight_old.append();
			for(int livery = 0; livery < livery_max; livery++)
			{
				sprintf(buf, "freightimage[%s][%d]", dir_codes[i], livery);
				str = obj.get(buf);
				if(str.empty())
				{
					break;
				}
				printf("Appending freightimage[%s][%d]", dir_codes[i], livery);
				liverykeys_freight_old.at(i).append(str);
			}
		}
		else if (freight_image_type > 0 && livery_max > 0)
		{
			// Liveries *and* freight
			liverykeys_freight.append();
			for(int livery = 0; livery < livery_max; livery++)
			{
				liverykeys_freight.at(i).append(); // See http://forum.simutrans.com/index.php?topic=9841.0
				for(int freight = 0; freight < freight_image_type; freight++)
				{
					sprintf(buf, "freightimage[%d][%s][%d]", freight, dir_codes[i], livery);
					str = obj.get(buf);
					if(str.empty())
					{
						break;
						dbg->fatal("Vehicle", "\nMissing freightimage[%d][%s][%d]!\n", freight, dir_codes[i], livery);
						fflush(NULL);
						exit(1);
					}
					liverykeys_freight.at(i).at(livery).append(str);
				}
			}
		}
		else
		{
			// This should never be reached.
			dbg->fatal("Vehicle", "Unknown error");
			exit(1);
		}
	}

	// prissi: added more error checks
	if (has_8_images && emptykeys.get_count() < 8 && liverykeys_empty.get_count() < 8)
	{
		dbg->fatal( "Vehicle", "Missing images (must be either 4 or 8 directions (but %i found)!)\n", emptykeys.get_count() + liverykeys_empty.get_count());
		exit(1);
	}

	if (!(freightkeys_old.empty() || liverykeys_freight_old.empty()) && (emptykeys.get_count() != freightkeys_old.get_count() || liverykeys_empty.get_count() != liverykeys_freight_old.get_count()))
	{
		dbg->fatal( "Vehicle", "Missing freigthimages (must be either 4 or 8 directions (but %i found)!)\n", freightkeys_old.get_count());
		exit(1);
	}

	if(livery_max == 0)
	{
		// Empty images, no multiple liveries
		imagelist_writer_t::instance()->write_obj(fp, node, emptykeys);
	}
	else
	{
		// Empty images, multiple liveries
		imagelist2d_writer_t::instance()->write_obj(fp, node, liverykeys_empty);
	}

	if (freight_image_type > 0 && livery_max == 0)
	{
		// Multiple freight images, no multiple liveries
		imagelist2d_writer_t::instance()->write_obj(fp, node, freightkeys);
	}
	else if(freight_image_type > 0 && livery_max > 0)
	{
		// Multiple frieght images, multiple liveries
		//fprintf(stderr, "Writing %d multiple livery multiple type freight images\n", (liverykeys_freight.get_count() * liverykeys_freight.front().get_count() * liverykeys_freight.front().front().get_count()));
		imagelist3d_writer_t::instance()->write_obj(fp, node, liverykeys_freight);
		//fprintf(stderr, "Completed\n");
	}
	else if(freight_image_type == 0 && livery_max > 0 && basic_freight_images > 0)
	{
		// Single freight images, multiple liveries
		//fprintf(stderr, "Writing %d single type multiple livery freight images\n", liverykeys_freight_old.get_count() * liverykeys_freight_old.front().get_count());
		imagelist2d_writer_t::instance()->write_obj(fp, node, liverykeys_freight_old);
		freight_image_type = 255; // To indicate that there are indeed single freight images and multiple liveries available.
		//fprintf(stderr, "Completed\n");
	}
	else if(freight_image_type == 0 && freight_image_type < 255 && livery_max == 0)
	{
		// Single freight images, no multiple liveries
		if (freightkeys_old.get_count() == emptykeys.get_count())
		{
			imagelist_writer_t::instance()->write_obj(fp, node, freightkeys_old);
		}
		else
		{
			// really empty list ...
			xref_writer_t::instance()->write_obj(fp, node, obj_imagelist, "", false);
		}
	}

	// following/leader vehicle constrains
	// can_lead_from_rear is deprecated but used for conversion processing to maintain compatibility
	uint8 bidirectional = (obj.get_int("bidirectional", 0));
	uint8 can_lead_from_rear = (obj.get_int("can_lead_from_rear", 0));
	if (can_lead_from_rear) { bidirectional = 1; }

	uint8 has_front_cab = (obj.get_int("has_front_cab", 255));
	uint8 has_rear_cab = (obj.get_int("has_rear_cab", 255));
	uint8 basic_constraint_prev = 0;
	uint8 basic_constraint_next = 0;

	uint8 leader_count = 0;
	bool can_be_at_front = true;
	bool found;
	bool prev_has_none = false;
	do {
		char buf[40];

		// Hajodoc: Constraints for previous vehicles
		// Hajoval: string, "none" means only suitable at front of an convoi
		sprintf(buf, "constraint[prev][%d]", leader_count);

		str = obj.get(buf);
		found = !str.empty();
		if (found) {
			if (!STRICMP(str.c_str(), "none")) {
				str = "";
				prev_has_none = true;
			}
			if (!STRICMP(str.c_str(), "any"))
			{
				// "Any" should not be specified with anything else.
				can_be_at_front = false;
				break;
			}
			else
			{
				xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str.c_str(), false);
				leader_count++;
			}
		}
	} while (found);

	// set front-end flags
	if (can_be_at_front) {
		if (!leader_count) {
			basic_constraint_prev |= vehicle_desc_t::can_be_tail | vehicle_desc_t::can_be_head; // no constraint setting = free
		}
		else if (prev_has_none) {
			basic_constraint_prev |= vehicle_desc_t::can_be_tail;
			if (!bidirectional) {
				basic_constraint_prev |= vehicle_desc_t::can_be_head;
			}
			if (has_front_cab || can_lead_from_rear) {
				basic_constraint_prev |= vehicle_desc_t::can_be_head;
			}
			if (leader_count == 1) {
				basic_constraint_prev |= vehicle_desc_t::unconnectable;
			}
		}
		else {
			// intermediate side
			if (leader_count == 1) {
				basic_constraint_prev |= vehicle_desc_t::intermediate_unique;
			}
		}
	}
	// auto setting
	if (has_front_cab == 255) {
		if (!power && bidirectional && !can_lead_from_rear) {
			basic_constraint_prev &= ~vehicle_desc_t::can_be_head; // brake van
		}
	}

	uint8 trailer_count = 0;
	bool can_be_at_end = true;
	bool next_has_none = false;
	do {
		char buf[40];

		// Hajodoc: Constraints for successing vehicles
		// Hajoval: string, "none" to disallow any followers
		sprintf(buf, "constraint[next][%d]", trailer_count);

		str = obj.get(buf);

		found = !str.empty();
		if (found)
		{
			if (!STRICMP(str.c_str(), "none"))
			{
				str = "";
				next_has_none = true;
			}
			if(!STRICMP(str.c_str(), "any"))
			{
				// "Any" should not be specified with anything else.
				can_be_at_end = false;
				break;
			}
			else
			{
				xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str.c_str(), false);
				trailer_count++;
			}
		}
	} while (found);

	// set back-end flags
	if (can_be_at_end) {
		if (!trailer_count) {
			basic_constraint_next |= vehicle_desc_t::can_be_tail | vehicle_desc_t::can_be_head; // no constraint setting = free
		}
		else if (next_has_none) {
			basic_constraint_next |= vehicle_desc_t::can_be_tail;
			if (bidirectional) {
				basic_constraint_next |= vehicle_desc_t::can_be_head;
			}
			if (trailer_count == 1) {
				basic_constraint_next |= vehicle_desc_t::unconnectable;
			}
			if (has_rear_cab || can_lead_from_rear) {
				basic_constraint_next |= vehicle_desc_t::can_be_head;
			}
		}
		else {
			// intermediate side
			if (trailer_count == 1) {
				basic_constraint_next |= vehicle_desc_t::intermediate_unique;
			}
		}
	}
	// auto setting (support old dat)
	if (has_rear_cab == 255 && (basic_constraint_next & vehicle_desc_t::can_be_head)) {
		if ((bidirectional && !can_lead_from_rear && !power) || !bidirectional) {
			basic_constraint_next &= ~vehicle_desc_t::can_be_head; // brake van or single derection vehicle
		}
	}

	// Upgrades: these are the vehicle types to which this vehicle
	// can be upgraded. "None" means that it cannot be upgraded.
	// @author: jamespetts
	uint8 upgrades = 0;
	do
	{
		char buf[40];
		sprintf(buf, "upgrade[%d]", upgrades);
		str = obj.get(buf);
		found = !str.empty();
		if (found)
		{
			if (!STRICMP(str.c_str(), "none"))
			{
				str = "";
			}
			else
			{
				xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str.c_str(), false);
				upgrades++;
			}
		}
	} while (found);

	// multiple freight image types - define what good uses each index
	// good without index will be an error
	if(freight_image_type < 255)
	{
		for (i = 0; i <= freight_image_type; i++)
		{
			char buf[40];
			sprintf(buf, "freightimagetype[%d]", i);
			str = obj.get(buf);
			if (i == freight_image_type)
			{
				// check for superflous definitions
				if(!str.empty())
				{
					dbg->warning( obj_writer_t::last_name, "More freightimagetype (%i) than freight_images (%i)!", i, freight_image_type);
					fflush(NULL);
				}
				break;
			}
			if(str.empty())
			{
				dbg->fatal( obj_writer_t::last_name, "Missing freightimagetype[%i] for %i freight_images!", i, freight_image_type + 1);
				exit(1);
			}
			xref_writer_t::instance()->write_obj(fp, node, obj_good, str.c_str(), false);
		}
	}

	// multiple liveries - define what liveries use each index
	// liveries without an index will be an error
	for (i = 0; i <= livery_max; i++)
	{
		char buf[128];
		sprintf(buf, "liverytype[%d]", i);
		str = obj.get(buf);
		if (i == livery_max)
		{
			// check for superflous definitions
			if(!str.empty())
			{
				dbg->fatal( obj_writer_t::last_name, "More livery types (%i) than liveries (%i)!", i, livery_max);
				fflush(NULL);
			}
			break;
		}
		if(str.empty())
		{
			dbg->fatal( obj_writer_t::last_name, "Missing liverytype[%i] for %i liveries!", i, livery_max + 1);
			exit(1);
		}
		text_writer_t::instance()->write_obj(fp, node, str.c_str());
	}

	// if no index defined then add default as vehicle good
	// if not using freight images then store zero string
	if (freight_image_type > 0 && freight_image_type < 255)
	{
		xref_writer_t::instance()->write_obj(fp, node, obj_good, freight, false);
	}

	if (livery_max > 0)
	{
		text_writer_t::instance()->write_obj(fp, node, "default");
	}

	node.write_sint8(fp, leader_count, pos);
	pos += sizeof(sint8);
	node.write_sint8(fp, trailer_count, pos);
	pos += sizeof(sint8);
	node.write_uint8(fp, (uint8) freight_image_type, pos);
	pos += sizeof(uint8);

	// Whether this is a tilting train
	// int
	//@author: jamespetts
	uint8 tilting = (obj.get_int("is_tilting", 0));
	node.write_uint8(fp, tilting, pos);
	pos += sizeof(uint8);

	// Way constraints
	// One byte for permissive, one byte for prohibitive.
	// Therefore, 8 possible constraints of each type.
	// Permissive: way allows vehicles with matching constraint:
	// vehicles not allowed on any other sort of way. Vehicles
	// without that constraint also allowed on the way.
	// Prohibitive: way allows only vehicles with matching constraint:
	// vehicles with matching constraint allowed on other sorts of way.
	// @author: jamespetts

	uint8 permissive_way_constraints = 0;
	uint8 prohibitive_way_constraints = 0;
	char buf_permissive[60];
	char buf_prohibitive[60];
	//Read the values from a file, and put them into an array.
	for(uint8 i = 0; i < 8; i++)
	{
		sprintf(buf_permissive, "way_constraint_permissive[%d]", i);
		sprintf(buf_prohibitive, "way_constraint_prohibitive[%d]", i);
		uint8 tmp_permissive = (obj.get_int(buf_permissive, 255));
		uint8 tmp_prohibitive = (obj.get_int(buf_prohibitive, 255));

		//Compress values into a single byte using bitwise OR.
		if(tmp_permissive < 8)
		{
			permissive_way_constraints = (tmp_permissive > 0) ? permissive_way_constraints | (uint8)pow(2, (double)tmp_permissive) : permissive_way_constraints | 1;
		}
		if(tmp_prohibitive < 8)
		{
			prohibitive_way_constraints = (tmp_prohibitive > 0) ? prohibitive_way_constraints | (uint8)pow(2, (double)tmp_prohibitive) : prohibitive_way_constraints | 1;
		}
	}
	node.write_uint8(fp, permissive_way_constraints, pos);
	pos += sizeof(uint8);
	node.write_uint8(fp, prohibitive_way_constraints, pos);
	pos += sizeof(uint8);

	// Catering level. 0 = no catering.
	// Higher numbers, better catering.
	// Catering boosts passenger revenue.
	// @author: jamespetts
	uint8 catering_level = (obj.get_int("catering_level", 0));
	node.write_uint8(fp, catering_level, pos);
	pos += sizeof(uint8);

	//Reverseing settings.
	//@author: jamespetts

	// Bidirectional: vehicle can travel backwards without turning around.
	// Function is disabled for road and air vehicles.
	node.write_uint8(fp, bidirectional, pos);
	pos += sizeof(uint8);

	// Can lead from rear: train can run backwards without turning around.
	//node.write_uint8(fp, can_lead_from_rear, pos);
	node.write_uint8(fp, basic_constraint_prev, pos);
	pos += sizeof(uint8);

	// Passenger comfort rating - affects revenue on longer journies.
	// Now segregated by class of passenger.
	//@author: jamespetts
	uint32 current_class_comfort;
	uint8 last_comfort = 100;
	for (uint32 i = 0; i < number_of_classes; i++)
	{
		// Check for multiple classes with a separate capacity each
		char buf[12];
		sprintf(buf, "comfort[%u]", i);
		current_class_comfort = obj.get_int(buf, UINT32_MAX_VALUE);
		if (current_class_comfort == UINT32_MAX_VALUE)
		{
			if (i == 0)
			{
				// If there are no class defined capacities, look for the old/basic
				// keyword defining a single level of comfort.
				uint8 comfort = (obj.get_int("comfort", 100));
				node.write_uint8(fp, comfort, pos);
				last_comfort = comfort;
				pos += sizeof(uint8);
			}
			else
			{
				// It is essential to write the same number of comfort
				// entries as payload entries, as this is what the
				// reader expects.
				node.write_uint8(fp, last_comfort, pos);
				pos += sizeof(uint8);
			}
		}
		else
		{
			node.write_uint8(fp, (uint8)current_class_comfort, pos);
			pos += sizeof(uint8);
		}
	}


	// Overcrowded capacity - can take this much *in addition to* normal capacity,
	// but revenue will be lower and dwell times higher. Mainly for passengers.
	// A single overcrowded capacity is maintained in spite of multiple classes
	// as it is not necessary to have different classes of, in effect, standing
	// passengers.

	//@author: jamespetts
	uint16 overcrowded_capacity = (obj.get_int("overcrowded_capacity", 0));
	node.write_uint16(fp, overcrowded_capacity, pos);
	pos += sizeof(uint16);

	// The time in ms that it takes the vehicle to load and unload at stations (i.e.,
	// the dwell time). The default is 2,000 because that is the value used in
	// Simutrans-Standard. This is the old system, and is superceded by a min/max
	// loading time now.
	//@author: jamespetts

	uint16 default_loading_time;

	switch(waytype)
	{
		default:
		case tram_wt:
		case road_wt:
			default_loading_time = 2000;
			break;

		case monorail_wt:
		case maglev_wt:
		case narrowgauge_wt:
		case track_wt:
			default_loading_time = 4000;
			break;

		case water_wt:
			default_loading_time = 20000;
			break;

		case air_wt:
			default_loading_time = 30000;
			break;
	}
	/**
	 * This is the old system for storing
	 * journey times. It is retained only
	 * for backwards compatibility. Journey
	 * times are now (10.0 and higher)
	 * stored as seconds, and converted to
	 * ticks when set_scale() is called.
	 * @author: jamespetts
	 */
	uint16 loading_time = (obj.get_int("loading_time", default_loading_time));
	node.write_uint16(fp, loading_time, pos);
	pos += sizeof(uint16);

	// Upgrading settings
	//@author: jamespetts
	node.write_sint8(fp, upgrades, pos);
	pos += sizeof(sint8);

	// This is the cost of upgrading to this vehicle, rather than buying it new.
	// By default, the cost is the same as a new purchase.
	uint32 upgrade_price = (obj.get_int("upgrade_price", cost));
	upgrade_price = (obj.get_int("upgrade_cost", upgrade_price));
	node.write_uint32(fp, upgrade_price, pos);
	pos += sizeof(uint32);

	// If this is set to true (is read as a bool), this will only be able to be purchased
	// as an upgrade to another vehicle, not as a new vehicle.
	uint8 available_only_as_upgrade = (obj.get_int("available_only_as_upgrade", 0));
	node.write_uint8(fp, available_only_as_upgrade, pos);
	pos += sizeof(uint8);

	// Fixed monthly maintenance costs
	// @author: jamespetts
	// (The original Extended name was "fixed_maintenance".
	// The new Standard name is "fixed_cost". It is necessary
	// to accommodate both.)
	uint32 fixed_cost = obj.get_int("fixed_maintenance", 0);
	fixed_cost = obj.get_int("fixed_cost", fixed_cost);
	node.write_uint32(fp, fixed_cost, pos);
	pos += sizeof(uint32);

	// Tractive effort
	// @author: jamespetts
	uint16 tractive_effort = obj.get_int("tractive_effort", 0);
	node.write_uint16(fp, tractive_effort, pos);
	pos += sizeof(uint16);

	// Air resistance
	// @author: jamespetts & Bernd Gabriel
	uint16 air_default = vehicle_desc_t::get_air_default(waytype);

	uint16 air_resistance_hundreds = obj.get_int("air_resistance", air_default);
	node.write_uint16(fp, air_resistance_hundreds, pos);
	pos += sizeof(uint16);

	// can_be_at_rear was integrated into one variable
	node.write_uint8(fp, basic_constraint_next, pos);
	pos += sizeof(uint8);

	// Obsolescence. Zeros indicate that simuconf.tab values should be used.
	// @author: jamespetts
	uint16 increase_maintenance_after_years = obj.get_int("increase_maintenance_after_years", 0);
	node.write_uint16(fp, increase_maintenance_after_years, pos);
	pos += sizeof(uint16);

	uint16 increase_maintenance_by_percent = obj.get_int("increase_maintenance_by_percent", 0);
	node.write_uint16(fp, increase_maintenance_by_percent, pos);
	pos += sizeof(uint16);

	uint8 years_before_maintenance_max_reached = obj.get_int("years_before_maintenance_max_reached", 0);
	node.write_uint8(fp, years_before_maintenance_max_reached, pos);
	pos += sizeof(uint8);

	node.write_uint8(fp, (uint8) livery_max, pos);
	pos += sizeof(uint8);

	/**
	 * The loading times (minimum and maximum) of this
	 * vehicle in seconds. These are converted to ticks
	 * after being read in simworld.cc's set_scale()
	 * method. Using these values is preferable to using
	 * the old "loading_time", which sets the ticks
	 * directly and therefore bypasses the scale. A value
	 * of 65535, the default, indicates that this value
	 * has not been set manually, and reverts to the
	 * default loading_time. This retains backwards
	 * compatibility with previous versions of paksets.
	 * @author: jamespetts, August 2011
	 */
	uint16 min_loading_time = obj.get_int("min_loading_time", 65535);
	node.write_uint16(fp, min_loading_time, pos);
	pos += sizeof(uint16);
	uint16 max_loading_time = obj.get_int("max_loading_time", 65535);
	node.write_uint16(fp, max_loading_time, pos);
	pos += sizeof(uint16);

	uint16 rolling_default = vehicle_desc_t::get_rolling_default(waytype);

	uint16 rolling_resistance_tenths_thousands = obj.get_int("rolling_resistance", rolling_default);
	node.write_uint16(fp, rolling_resistance_tenths_thousands, pos);
	pos += sizeof(rolling_resistance_tenths_thousands);

	uint16 brake_force = obj.get_int("brake_force", 65535);
	node.write_uint16(fp, brake_force, pos);
	pos += sizeof(brake_force);

	uint16 minimum_runway_length = obj.get_int("minimum_runway_length", 0);
	node.write_uint16(fp, minimum_runway_length, pos);
	pos += sizeof(minimum_runway_length);

	uint16 range = obj.get_int("range", 0);
	node.write_uint16(fp, range, pos);
	pos += sizeof(range);

	uint32 way_wear_factor = obj.get_int("way_wear_factor", UINT32_MAX_VALUE);
	node.write_uint32(fp, way_wear_factor, pos);
	pos += sizeof (way_wear_factor);

	uint8 is_tall = obj.get_int("is_tall", 0);
	node.write_uint8(fp, is_tall, pos);
	pos += sizeof(is_tall);

	uint8 mixed_load_prohibition = obj.get_int("mixed_load_prohibition", 0);
	node.write_uint8(fp, mixed_load_prohibition, pos);
	pos += sizeof(mixed_load_prohibition);

	uint8 override_way_speed = obj.get_int("override_way_speed", 0);
	node.write_uint8(fp, override_way_speed, pos);
	pos += sizeof(override_way_speed);

	sint8 sound_str_len = sound_str.size();
	if (sound_str_len > 0) {
		node.write_sint8  (fp, sound_str_len, pos);
		pos += sizeof(sint8);
		node.write_data_at(fp, sound_str.c_str(), pos, sound_str_len);
		pos += sound_str_len;
	}

	node.write(fp);
}

