#include "../../dataobj/tabfile.h"

#include "../ware_besch.h"
#include "obj_node.h"
#include "text_writer.h"

#include "good_writer.h"

using std::string;

void good_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	// Hajo: version number
	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 version = 0x8003;

	// This is the overlay flag for Simutrans-Experimental
	// This sets the *second* highest bit to 1. 
	version |= EXP_VER;

	// Finally, this is the experimental version number. This is *added*
	// to the standard version number, to be subtracted again when read.
	// Start at 0x100 and increment in hundreds (hex).
	version += 0x100;

	int pos = 0;
	uint32 len = 3; // 10 (now 11)

	// Used only for the old fixed values (no fare stages)
	const uint16 value = obj.get_int("value", 0);
	len += sizeof(value);

	const uint8 catg = obj.get_int("catg", 0);
	len += sizeof(catg);

	const uint16 speed_bonus = obj.get_int("speed_bonus", 0);
	len += sizeof(speed_bonus);

	const uint16 weight_per_unit = obj.get_int("weight_per_unit", 100);
	len += sizeof(weight_per_unit);

	const uint8 mapcolor = obj.get_int("mapcolor", 255);
	len += sizeof(mapcolor);

	string str;
	uint8 fare_stages = 0;
	bool value_check = false;
	bool distance_check = false;
	const int MAX_FARE_STAGES = 32;
	uint16 staged_values[MAX_FARE_STAGES];
	uint16 staged_distances[MAX_FARE_STAGES];

	do
	{
		char buf_v[40];
		sprintf(buf_v, "value[%d]", staged_values[fare_stages]);
		str = obj.get(buf_v);
		value_check = !str.empty();

		char buf_d[40];
		sprintf(buf_d, "to_distance[%d]", staged_distances[fare_stages]);
		str = obj.get(buf_d);
		distance_check = !str.empty();

		if (value_check && distance_check)
		{
			fare_stages++;
		}
	} while (value_check && distance_check && fare_stages < MAX_FARE_STAGES);

	if(fare_stages >= MAX_FARE_STAGES)
	{
		fare_stages = MAX_FARE_STAGES - 1;
	}

	len += fare_stages;

	obj_node_t node(this, len, &parent);

	node.write_uint16(fp, version, pos);
	pos += sizeof(uint16);
		
	write_head(fp, node, obj);
	text_writer_t::instance()->write_obj(fp, node, obj.get("metric"));

	node.write_uint16(fp, value,			pos);
	pos += sizeof(value);
	node.write_uint8 (fp, catg,				pos);
	pos += sizeof(catg);
	node.write_uint16(fp, speed_bonus,		pos);
	pos += sizeof(speed_bonus);
	node.write_uint16(fp, weight_per_unit,  pos);
	pos += sizeof(weight_per_unit);
	node.write_uint8 (fp, mapcolor,			pos);
	pos += sizeof(mapcolor);
	node.write_uint8(fp, fare_stages,		pos);
	pos += sizeof(fare_stages);

	for(int n = 0; n < fare_stages; n ++)
	{
		node.write_uint16(fp, staged_distances[n], pos);
		pos += sizeof(uint16);
		node.write_uint16(fp, staged_values[n], pos);
		pos += sizeof(uint16);
	}

	node.write(fp);
}
