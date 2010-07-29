#include "replace_data.h"

#include "loadsave.h"
#include "translator.h"

#include "../utils/cbuffer_t.h"

#include "../bauer/vehikelbauer.h"

#include "../vehicle/simvehikel.h"
#include "../besch/vehikel_besch.h"


replace_data_t::replace_data_t()
{
	autostart = true;
	retain_in_depot = false;
	use_home_depot = false;
	allow_using_existing_vehicles = true;
	replacing_vehicles = new vector_tpl<const vehikel_besch_t *>;
	replacing_convoys = new vector_tpl<convoihandle_t>();
	number_of_convoys = 0;
	clearing = false;
}

replace_data_t::replace_data_t(replace_data_t* copy_from)
{
	autostart = copy_from->get_autostart();
	retain_in_depot = copy_from->get_retain_in_depot();
	use_home_depot = copy_from->get_use_home_depot();
	allow_using_existing_vehicles = copy_from->get_allow_using_existing_vehicles();
	const uint16 replace_count = copy_from->get_replacing_vehicles()->get_count();
	replacing_vehicles = new vector_tpl<const vehikel_besch_t*>(replace_count);
	replacing_convoys = new vector_tpl<convoihandle_t>();
	for(uint16 i = 0; i < replace_count; i ++)
	{
		replacing_vehicles->append(copy_from->get_replacing_vehicle(i));
	}
		
	number_of_convoys = copy_from->get_number_of_convoys();
}

replace_data_t::replace_data_t(loadsave_t *file)
{
	replacing_vehicles = new vector_tpl<const vehikel_besch_t *>;
	replacing_convoys = new vector_tpl<convoihandle_t>();
	rdwr(file);
	// When replace data are loaded, there is no easy way of checking
	// to see which are identical, so they are assigned to convoys one
	// each, rather than being pooled as is the case when replacing is
	// first set without a save/load cycle.
	number_of_convoys = 1;
}


void replace_data_t::sprintf_replace( cbuffer_t &buf) const
{
	// First of all, general information
	buf.append(autostart ? "1" : "0");
	buf.append(retain_in_depot ? "1" : "0");
	buf.append(use_home_depot ? "1" : "0");
	buf.append(allow_using_existing_vehicles ? "1" : "0");
		
	// Secondly, the number of convoys. Use leading zeros
	// to keep a constant number of characters. 
	sint8 zeros = 0;
	if(number_of_convoys < 10)
	{
		zeros = 4;
	}
	else if(number_of_convoys < 100)
	{
		zeros = 3;
	}
	else if(number_of_convoys < 1000)
	{
		zeros = 2;
	}
	else if(number_of_convoys < 10000)
	{
		zeros = 1;
	}
	while(zeros > 0)
	{
		buf.append("0");
		zeros--;
	}
	buf.append((int)number_of_convoys);

	// Finally, the replacing vehicles.
	// Separate each name with the "|" character.
	buf.append("|");
	ITERATE_PTR(replacing_vehicles, n)
	{
		buf.append(replacing_vehicles->get_element(n)->get_name());
		buf.append("|");
	}
	// Terminating character	
	buf.append("~");
}


bool replace_data_t::sscanf_replace(const char *ptr)
{
	const char *p = ptr;
	// Firstly, get the general settings.
	autostart = atoi(p++);
	const char rid = *p++;
	retain_in_depot = atoi(&rid);
	const char uhd = *p++;
	use_home_depot = atoi(&uhd);
	const char auev = *p++;
	allow_using_existing_vehicles = atoi(&auev);

	//Secondly, get the number of replacing vehicles
	char rv[5];
	for(uint8 i = 0; i < 5; i ++)
	{
		rv[i] = *p++;
	}
	number_of_convoys = atoi(rv);

	// Thirdly, get the replacing vehicles.
	replacing_vehicles->clear();

	while(  *p  &&  *p!='|'  ) 
	{
		p++;
	}
	if(  *p!='|'  ) 
	{
		dbg->error( "replace_data_t::sscanf_replace()","incomplete entry termination!" );
		return false;
	}
	p++;
	// now scan the entries
	while(*p!='~') 
	{
		char vehicle_name[256];
		uint8 n = 0;
		while(*p!='|')
		{
			vehicle_name[n++] = *p++;
		}
		vehicle_name[n] = '\0';
		
		const vehikel_besch_t* besch = vehikelbauer_t::get_info(vehicle_name);
		if(besch == NULL) 
		{
			besch = vehikelbauer_t::get_info(translator::compatibility_name(vehicle_name));
		}
		if(besch == NULL)
		{
			dbg->warning("replace_data_t::sscanf_replace()","no vehicle pak for '%s' search for something similar", vehicle_name);
		}
		else
		{
			replacing_vehicles->append(besch);
		}
		p++;
	}
	return true;
}

replace_data_t::~replace_data_t()
{
	delete replacing_vehicles;
	delete replacing_convoys;
}

void replace_data_t::rdwr(loadsave_t *file)
{
	file->rdwr_bool(autostart);
	file->rdwr_bool(retain_in_depot);
	file->rdwr_bool(use_home_depot);
	file->rdwr_bool(allow_using_existing_vehicles);
	
	uint16 replacing_vehicles_count;

	if(file->is_saving())
	{
		replacing_vehicles_count = replacing_vehicles->get_count();
		file->rdwr_short(replacing_vehicles_count);
		ITERATE_PTR(replacing_vehicles, i)
		{
			const char *s = replacing_vehicles->get_element(i)->get_name();
			file->rdwr_str(s);
		}
	}
	else
	{
		file->rdwr_short(replacing_vehicles_count);
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

void replace_data_t::decrement_convoys(convoihandle_t cnv)
{
	if(!clearing)
	{
		replacing_convoys->remove(cnv);
	}

	if(--number_of_convoys <= 0 && !clearing)
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

void replace_data_t::increment_convoys(convoihandle_t cnv)
{
	replacing_convoys->append(cnv); 
	number_of_convoys ++; 
}

void replace_data_t::clear_all()
{
	clearing = true;
	ITERATE_PTR(replacing_convoys, i)
	{
		if(replacing_convoys->get_element(i).is_bound())
		{
			replacing_convoys->get_element(i)->clear_replace();
			replacing_convoys->get_element(i)->set_depot_when_empty(false);
			replacing_convoys->get_element(i)->set_no_load(false);
		}
	}
	delete this;
}
