/*
  * @author: jamespetts, April 2011
  * This file is part of the Simutrans project under the artistic licence.
  * (see licence.txt)
  */

#include "livery_scheme.h"
#include "loadsave.h"
#include "../besch/vehikel_besch.h"


const char* livery_scheme_t::get_latest_available_livery(uint16 date, const vehikel_besch_t* besch) const
{
	if(liveries.empty())
	{
		// No liveries available at all
		return NULL;
	}
	const char* livery = NULL;
	uint16 latest_valid_intro_date = 0;
	ITERATE(liveries, i)
	{
		if(date >= liveries[i].intro_date && besch->check_livery(liveries[i].name.c_str()) && liveries[i].intro_date > latest_valid_intro_date)
		{
			// This returns the most recent livery available for this vehicle that is not in the future.
			latest_valid_intro_date = liveries[i].intro_date;
			livery = liveries[i].name.c_str();
		}
	}
	return livery;
}

void livery_scheme_t::rdwr(loadsave_t *file)
{
	file->rdwr_short(retire_date);
	uint16 count = 0;
	if(file->is_saving())
	{
		count = liveries.get_count();
	}

	file->rdwr_short(count);

	for(int i = 0; i < count; i ++)
	{
		const char* n = liveries[i].name.c_str();
		file->rdwr_str(n);
		liveries[i].name = n;
		file->rdwr_short(liveries[i].intro_date);
	}
}