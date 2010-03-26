#include "replace_data.h"

#include "loadsave.h"
#include "translator.h"

#include "../bauer/vehikelbauer.h"

#include "../vehicle/simvehikel.h"


replace_data_t::replace_data_t()
{
	autostart = true;
	retain_in_depot = false;
	use_home_depot = false;
	allow_using_existing_vehicles = true;
	replacing_vehicles = new vector_tpl<const vehikel_besch_t *>;
	number_of_convoys = 0;
}

replace_data_t::replace_data_t(replace_data_t* copy_from)
{
	autostart = copy_from->get_autostart();
	retain_in_depot = copy_from->get_retain_in_depot();
	use_home_depot = copy_from->get_use_home_depot();
	allow_using_existing_vehicles = copy_from->get_allow_using_existing_vehicles();
	const uint16 replace_count = copy_from->get_replacing_vehicles()->get_count();
	replacing_vehicles = new vector_tpl<const vehikel_besch_t*>(replace_count);
	for(uint16 i = 0; i < replace_count; i ++)
	{
		replacing_vehicles->append(copy_from->get_replacing_vehicle(i));
	}
		
	number_of_convoys = copy_from->get_number_of_convoys();
}

replace_data_t::replace_data_t(loadsave_t *file)
{
	rdwr(file);
	replacing_vehicles = new vector_tpl<const vehikel_besch_t *>;
	// When replace data are loaded, there is no easy way of checking
	// to see which are identical, so they are assigned to convoys one
	// each, rather than being pooled as is the case when replacing is
	// first set without a save/load cycle.
	number_of_convoys = 1;
}

replace_data_t::~replace_data_t()
{
	delete replacing_vehicles;
}

void replace_data_t::rdwr(loadsave_t *file)
{
	file->rdwr_bool(autostart, "");
	file->rdwr_bool(retain_in_depot, "");
	file->rdwr_bool(use_home_depot, "");
	file->rdwr_bool(allow_using_existing_vehicles, "");
	
	uint16 replacing_vehicles_count;

	if(file->is_saving())
	{
		replacing_vehicles_count = replacing_vehicles->get_count();
		file->rdwr_short(replacing_vehicles_count, "");
		ITERATE_PTR(replacing_vehicles, i)
		{
			const char *s = replacing_vehicles->get_element(i)->get_name();
			file->rdwr_str(s);
		}
	}
	else
	{
		file->rdwr_short(replacing_vehicles_count, "");
		for(uint16 i = 0; i < replacing_vehicles_count; i ++)
		{
			char vehicle_name[256];
			file->rdwr_str(vehicle_name, 256);
			const vehikel_besch_t* besch = vehikelbauer_t::get_info(vehicle_name);
			if(besch == NULL) 
			{
				besch = vehikelbauer_t::get_info(translator::compatibility_name(vehicle_name));
			}
			if(besch == NULL)
			{
				dbg->warning("replace_data_t::rdwr()","no vehicle pak for '%s' search for something similar", vehicle_name);
			}
			else
			{
				replacing_vehicles->append(besch);
			}
		}
	}
}

const vehikel_besch_t*  replace_data_t::get_replacing_vehicle(uint16 number) const
{
	return replacing_vehicles->get_element(number);
	
}

void replace_data_t::decrement_convoys()
{
	if(--number_of_convoys <= 0)
	{
		// See http://www.parashift.com/c++-faq-lite/freestore-mgmt.html#faq-16.15
		// When maintaining this code, ensure that the above criteria remain satisfied.
		delete this;
	}
}

void replace_data_t::add_vehicle(const vehikel_besch_t* vehicle, bool add_at_front)
{
	if(add_at_front)
	{
		replacing_vehicles->insert_at(0, vehicle);
	}
	else
	{
		replacing_vehicles->append(vehicle);
	}
}