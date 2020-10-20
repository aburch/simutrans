/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../dataobj/tabfile.h"

#include "../goods_desc.h"
#include "obj_node.h"
#include "text_writer.h"

#include "good_writer.h"

using std::string;

void good_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	// Version needs high bit set as trigger -> this is required
	// as marker because formerly nodes were unversioned
	uint16 version = 0x8003;

	// This is the overlay flag for Simutrans-Extended
	// This sets the *second* highest bit to 1.
	version |= EX_VER;

	// Finally, this is the extended version number. This is *added*
	// to the standard version number, to be subtracted again when read.
	// Start at 0x100 and increment in hundreds (hex).
	// 0x200 - number of classes (12.3)
	version += 0x200;

	int pos = 0;
	uint32 len = 3; // Should end up as 11 for Extended version 1 combined with Standard version 3.

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

	const uint8 number_of_classes = obj.get_int("number_of_classes", 1);
	len += sizeof(number_of_classes);

	uint16 val = 0;
	uint16 to_distance = 0;
	uint8 fare_stages = 0;
	const int MAX_FARE_STAGES = 32;
	uint16 staged_values[MAX_FARE_STAGES];
	uint16 staged_distances[MAX_FARE_STAGES];

	do
	{
		char buf_v[40];
		char buf_d[40];
		sprintf(buf_v, "value[%i]", fare_stages);
		sprintf(buf_d, "to_distance[%i]", fare_stages);
		val = obj.get_int(buf_v, 65535);
		to_distance =  obj.get_int(buf_d, to_distance);
		if(val != 65535)
		{
			staged_values[fare_stages] = val;
			staged_distances[fare_stages] = to_distance;
			fare_stages ++;
		}
		else
		{
			break;
		}

	} while (fare_stages < MAX_FARE_STAGES);


	// For each fare stage, there are 2x 16-bit variables.
	len += (fare_stages * 4);

	vector_tpl<uint16> class_revenue_percents(number_of_classes);
	uint16 class_revenue_percent;
	char buf_crp[56];

	for (uint8 i = 0; i < number_of_classes; i++)
	{
		// The revenue factor percentages for each class
		sprintf(buf_crp, "class_revenue_percent[%i]", i);
		class_revenue_percent = obj.get_int(buf_crp, 100);
		class_revenue_percents.append(class_revenue_percent);
		len += 2;
	}

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
	node.write_uint8(fp, mapcolor,			pos);
	pos += sizeof(mapcolor);
	node.write_uint8(fp, number_of_classes,	pos);
	pos += sizeof(number_of_classes);
	node.write_uint8(fp, fare_stages,		pos);
	pos += sizeof(fare_stages);

	for(uint8 n = 0; n < fare_stages; n ++)
	{
		node.write_uint16(fp, staged_distances[n], pos);
		pos += sizeof(uint16);
		node.write_uint16(fp, staged_values[n], pos);
		pos += sizeof(uint16);
	}

	for (uint8 i = 0; i < number_of_classes; i++)
	{
		node.write_uint16(fp, class_revenue_percents[i], pos);
		pos += sizeof(uint16);
	}

	node.write(fp);
}
