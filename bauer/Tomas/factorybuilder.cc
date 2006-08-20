/*
 * factorybuilder.cc
 *
 * Copyright (c) 2003-2004   Tomas Kubes
 *							 (some pieces of code are from:   )
 *                           (Hansjörg Malthaner, Volker Meyer)
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*

	Factorybuilder
	**************

  Responsible for all necessary things that are required for
  proper calling:

  build_industries (during map generation)

  build_industry
  Connect_Given_Industry
                   (durign user creation)


  !!THOSE ARE ONLY FUNCTIONS MENT TO BE CALLED BY USER!!!

  (industries will not get linked to cities, that will be done in simcity.cpp)




	External Dependencies
	*********************
  depends on:
  -----------
	factorybuilder.h (header)
	../tpl/vector_tpl.h (definition of simutrans vector type.)
	../tpl/list_tpl.h (definition of simutrans list type.)
	../tpl/array_tpl.h (definition of simutrans array type.)
	../tpl/stringhashtable_tpl.h (definition of simutrans associative array type.)

	methods and types concerning industry (goods management, info retrival)
	methods and types concering map information retrival

  modifies:
  ---------
    simworld - karte_t::fab_list (list of all factories on map)
	and of course map, by placing industries there. :-)

  methods being called outside:
  -----------------------------
	factory_reader.cpp - calls: register_besch (fill the table with all loaded industry beshes)
	to city linking functions - simcity.cpp
*/

#include <math.h>

#include "../simdebug.h"
#include "factorybuilder.h"
#include "../simworld.h"
#include "../simfab.h"
#include "../simdisplay.h"
#include "../simgraph.h"
#include "../simtools.h"
#include "../simcity.h"
#include "../simskin.h"
#include "../simhalt.h"
#include "../simplay.h"
#include "../simdebug.h"

#include "../dings/leitung.h"

#include "../boden/grund.h"
#include "../boden/wege/dock.h"

#include "../dataobj/einstellungen.h"
#include "../dataobj/translator.h"


#include "../sucher/factory_place_finder.h"



////////////////table management //////////////////////////////////////////////////
////////initialize mem. for the factory besh table (holdes beshes to all factories)
stringhashtable_tpl</*const*/ fabrik_besch_t *> factory_builder_t::table;

//returns factorybesh, based on factory object name
/*const*/ fabrik_besch_t *factory_builder_t::gib_fabesch(const char *fabtype)
{
/*    return table.get(fabtype);*/
	return *(table.access(fabtype));

}

//writes factory to the table (called in factory_reader.cpp)
void factory_builder_t::register_besch(fabrik_besch_t *besch)
{
    table.put(besch->gib_name(), besch);
}








/////////////Cell management////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//initializes mem. for static variables
uint8 factory_builder_t::map_cell_info_t::cell_size;
factory_builder_t::map_cell_info_t *factory_builder_t::cell_lists_t::s_last_cell;
uint8 factory_builder_t::cell_lists_t::s_f_tries;


////////////////////////////////////////////////////////////////////////////////
//constructor
factory_builder_t::map_cell_info_t::map_cell_info_t (factory_builder_t *tmp)
{
	builder = tmp;

	counts.land = 0;
	counts.city = 0;
	counts.water = 0;
	counts.slope = 0;

	sum_of_square_heights = 0;

	no_of_factories_in_cell = 0;
	has_special = FALSE;


}



//calculates percents from total counts and other collected values
//percen values will be used to judge type of the cell
void factory_builder_t::map_cell_info_t::Calculate_Cell_Percents (void)
{
	const float cell_squares = (cell_size*cell_size);

	percents.land  = (counts.land * 100) / cell_squares;
	percents.city  = (counts.city * 100) / cell_squares;
	percents.water = (counts.water * 100) / cell_squares;
	percents.slope = (counts.slope * 100) / cell_squares;

	//computes avarege hieght value in cell
	cell_height = (sum_of_square_heights / cell_squares);

	//adds total height to total height of world
	builder->Set_Sum_of_World_Heights (builder->Gib_Sum_of_World_Heights() + sum_of_square_heights);

}

//uses percents calculated above to determine cell type
//nothing more than bunch of IFs
factory_builder_t::recognized_cell_types factory_builder_t::map_cell_info_t::Determine_Cell_Type (const float &min_slope_percent, const float &min_city_percent)
{

	//known cell types
	//{SEA, SHORE, LOW, HIGH, CITY, UNUSABLE};
	//actually city is any cell with roads - so in further versions it can be cells with intercity connections

	if (percents.water > 90)
	{	//if there is 90% of water, square is sea, finish checking
		//cell_type = SEA;
		return SEA;
	}

	//determine if cell is not too slopy
	//good value is between 30 - 40
	if (percents.slope > min_slope_percent)
	{	//if more of x% of land are slopes, terrain will be unbildable, discard it and finish search
		//this might create problems arounnd map corners if there are big hills nearby, tiles get discarded

		/*generally this value determines nuber of discarded cells due to rough terrain, setting it lower will ease work to place finder, but might render rougher areas empty and vice versa */
	//	cell_type = UNUSABLE;
		return UNUSABLE;
	}

	//good value 10% or less for larger cell sizes
	if (percents.city > min_city_percent)
	{	//if at least 10% of cell are roads or buildings
		//might get problematic if city falls into more cells
	    //cell_type = CITY;
		return CITY;
	}

	if ((percents.land > 20) && (percents.water > 10))
	{
		return SHORE;
	}

	if (percents.land > 30){
		//cell_type = (cell_height < builder->Gib_Average_World_Height())?LOW:HIGH;
		return (cell_height < builder->Gib_Average_World_Height())?LOW:HIGH;
	}

	//if some cell falls here it is mix of so many cathegories, that we rather do not want to try to build there
	return UNUSABLE;

}

//called for a cell, it scans terrrain in it and stores results of terrain counts and percents
void factory_builder_t::map_cell_info_t::Collect_Terrain_Data (const koord pos, const karte_t *world) throw (int)
{
	if (cell_size == 0) throw 0;

	uint8 i, j;
	koord k = pos;
	cell_position = pos;

	//scans through all squares in cell and collects the counts of square types
	for (i = 0; i < cell_size; i++)
	{

		for (j = 0; j < cell_size; j++)
		{

			const grund_t *gr = world->lookup(k)->gib_kartenboden();

			//collect the height of each square (>>4 = /16), devide by 16, to get nuber seen on koordinates
			sum_of_square_heights = sum_of_square_heights + ((gr->gib_hoehe() - world->gib_grundwasser())>>4);

			//we will find a square and check what type of terrain is on it
			if (gr->ist_wasser())
			{	//if ground is water, increase water and end search for this sq.
				counts.water++;
			} else {

				if (gr->gib_grund_hang() != hang_t::flach)
				{	//if terrain type is not flat land, increase slopes and finish search on this sq.
					counts.slope++;

				} else { //now we have flat land terrain, what type is it?
					if ( gr->ist_natur())
					{   //if ground is clear - no building, no road, count free lenad
						counts.land++;
					} else {
						//then it must be city building, road or monument (monuments should be built after factories!)
						counts.city++;
					}
				}
			}
			/*
			Drawback of this is, that it counts buildings on slopes as slopes.
			But this is not a big problem, since too slopy land is bad for
			factory building, so it could be discarded.
			*/

			k.y++;

		}

		k.x++;
		k.y = pos.y;

	}

	//calculates percents and average hiegth of cell
	Calculate_Cell_Percents();

	/*
	This is debug output in form of signs.
	//all of this is to nicely print found values in form of text sign
	cstring_t c =  csprintf("Cell (%i, %i); height = %f; l=%i (%i%%), w=%i (%i%%), c=%i (%i%%), s=%i (%i%%)",pos.x, pos.y, cell_height, counts.land, (int)percents.land, counts.water, (int)percents.water, counts.city, (int)percents.city, counts.slope, (int)percents.slope);
	char * b = new (char [200]); //do not delete this - it is needed for name tags, they will use it
	strcpy(b,c.chars());
	world->lookup(pos)->gib_kartenboden()->setze_text(b);
	*/
}







/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////cell list management////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////



//constructor
factory_builder_t::cell_lists_t::cell_lists_t ()
{
	sea = new cell_list_tpl;
	shore = new cell_list_tpl;
	low = new cell_list_tpl;
	high = new cell_list_tpl;
	city = new cell_list_tpl;

	usable_cells_total = 0;
	usable_land_cells_total = 0;

}

factory_builder_t::cell_lists_t::~cell_lists_t ()
{
	//destructor, basically, clean everything we wont need.

	/*we need to check if pointer is not null, since on some wierd maps,
	it can happend that some lits will get merged since cells of some type will not occur.
	Thus pointed lits might get deleted already

	That is problem caused by possible list mergin if cell counts are too small.
	*/

	//no problem with seas, there should be no problems
	if (sea != NULL)
	{
		//get iterator for sea list
		cell_list_iterator_tpl tmp (sea);
		//set it to start
		tmp.begin();
		//scan whole list and freem memory occupied by cells
		while (tmp.next()) delete ((tmp.access_current()));
		//delete the lsit
		delete sea;
		//clean pointer
		sea = NULL;
	}

	//shore should be fine as it is first in the row
	if (shore != NULL)
	{
		cell_list_iterator_tpl tmp (shore);
		tmp.begin();
		while (tmp.next()) delete ((tmp.access_current()));
		delete shore;

		//no do not set it to null here!!
		//we need to know where pointer was pointing
		//shore = NULL;
	}

	//there can be problem here if low and shore lists were merged, test is needed
	/*second part should really compare pointers themselves, because if list were merged,
	  and shore was deleted already they now both refference same palce in free memory with
	  unknown content I do not want to touch.*/
	//refferencing already deleted memory in this case usually causes ahngup in map generation
	if ((low != NULL) && (low != shore))
	{
		cell_list_iterator_tpl tmp (low);
		tmp.begin();
		while (tmp.next()) delete ((tmp.access_current()));
		delete low;
		//do not set it to null here
		//low = NULL;
	}

	//height can be merged with low, se be cautious here too
	if ((high != NULL) && (low != high) && (shore != high))
	{
		cell_list_iterator_tpl tmp (high);
		tmp.begin();
		while (tmp.next()) delete ((tmp.access_current()));
		delete high;
		//high = NULL;
	}

	//make sure that list was not deleted already
	if ((city != NULL) && (low != city) && (shore != city) && (high != city))
	{
		cell_list_iterator_tpl tmp (city);
		tmp.begin();
		while (tmp.next()) delete ((tmp.access_current()));
		delete city;
		city = NULL;
	}

	//set nulls now
	shore = NULL;
	low = NULL;
	high = NULL;

}



//gets some random cell from the given list
factory_builder_t::map_cell_info_t* factory_builder_t::cell_lists_t::Get_Random_Cell (const cell_list_tpl *my_list)
{
	//simple, get list iterator, generate random number 0 to cell count and return cell at given number
	cell_list_iterator_tpl i ( my_list );
	const uint16 index = simrand ( my_list -> count() );
	return my_list -> at (index);
}

//gets some random cell of type land (low, high, shore, cities)
//this function is quite bound to current configuration. If someone would like to make greter changes to used terrain types, this function would need redesign
factory_builder_t::cell_list_tpl* factory_builder_t::cell_lists_t::Get_Random_Land_Cell_List (void) const
{
	//must be signed value to make sure that overflow wont lead to NULL end
	//now guess random number
	sint16 index = simrand (usable_land_cells_total);

	//and find a cell type which has the number is in the range (if all cells would be in one line)
	if (index < (city -> count()) )
	{
		return city;
	} else
	{

		index = index - (city -> count());

		if (index < (low -> count()) )
		{
			return low;

		} else
		{
			index = index - (low -> count());

			if ( index < (high -> count()) )
			{
				return high;

			} else
			{

				return shore;
			}

		}

	}

	//one should palce debug output here, I suspect this place for possible crash
	//this place should be unreachable however!!!
	//at least now if index is signed number
	return NULL;

}


factory_builder_t::cell_list_tpl* factory_builder_t::cell_lists_t::Get_Random_LH_Cell_List (void) const
{
	//must be signed value to make sure that overflow wont lead to NULL end
	//now guess random number
	sint16 index = simrand ( (low -> count()) + (high -> count()) - 1 );

	//and find a cell type which has the number is in the range (if all cells would be in one line)

	if (index < (low -> count()) )
	{
		return low;

	} else
	{
		return high;
	}

	//one should palce debug output here, I suspect this place for possible crash
	//this place should be unreachable however!!!
	//at least now if index is signed number
	return NULL;

}





factory_builder_t::map_cell_info_t* factory_builder_t::cell_lists_t::Gib_nth_Closest_Cell
 (const cell_list_tpl *my_list, const koord pos, const uint16 dist, const uint16 n)
{

	if (n==0)
	{
		s_f_tries = 1;
	}

	cell_list_iterator_tpl tmp_iter (my_list);
	tmp_iter.begin();
	uint16 count = 0;

	while (tmp_iter.next())
	{

		const koord pos2 = tmp_iter.get_current()->Gib_Cell_Position();
		const uint16 calc_dist = abs(pos.x - pos2.x) + abs(pos.y - pos2.y);

		//we have found a cell, that is close enough
		if (calc_dist <= dist)
		{

			count ++;
			s_last_cell = tmp_iter.get_current();

			if (count == (n%s_f_tries))
			{
				return s_last_cell;

			}

		}



	}

	//if we get here, then count stores the total number of cells found in the list
	//so we will remember it
	s_f_tries = count;



	/*			s_last_cell = NULL;
			s_f_tries = 0;
*/

	   /*private:
		//those supplementary variables are here to optimize some cell searching methods
		//of function Gib_nth_Closest_Cell
		static map_cell_info_t *s_last_cell;
		static uint8			s_f_tries;*/

	return s_last_cell;

}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////builder methods/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////


factory_builder_t::factory_builder_t (karte_t *world1, spieler_t *p) : player(p)
//assigns p to player, it was inteded to be const (assignement requiered here), but it was not possible
{
	world = world1;  // assigns pointer to actual map;
	world_size = world1 -> gib_groesse(); // to simplify the code, map size will be stored in temp variable

	//cell size must be divisor of map size, so it must be some power of 2. Also cell size smaller than 8 will cause exception
	//this is default value. It can be overriden (is currently overriden in fucntion build industries).
	map_cell_info_t::Set_Cell_Size (16);
	//this is tmp variable used for counting
	sum_of_world_heights = 0;

	//this will set some supplementary static variables
	cell_lists_t::Init_s_Values();

}




/*this method will run functions to collect terrain data, create terrain information cells
and store them into the cell lists  in cl structure given, it also takes parameters that determine
cell sorting - min slope = %of slopes in cell to make it unusable, min city = %of city squares
to make cell city
NEEDS TO BE CALLED ON PROPER DATA, throws char signalling thet calculation was not correct and needs to be redone*/
void factory_builder_t::Fill_Cell_Info_Lists (cell_lists_t *cl, const float min_slope_percent, const float min_city_percent)
{

	map_cell_info_t *worked_cell;
	uint16 i, j, k;

	//this is temporary array that will store pointers to all info cells after they are explored and before they will be sorted
	/*this might be a problem, since it cannot store more than 32000 cells. ;-), that means that maps larger then about 2600x2600 will need cell size larger than 16*/
	//the 2600x2600 and larger worlds are now cured by function call, cell size is increased to 32, allowing worlds roughly 5000x5000
	const uint16 cells_in_row = (world_size/map_cell_info_t::Gib_Cell_Size());//purely temporary variable - to calculate array size
	array_tpl <map_cell_info_t *> tmp_array_of_cells ( cells_in_row*cells_in_row );

	//in this cycle we will collect all terrain data, and create info about each map cell
	//we will also collect data about total world height (that is done in Collect_Terrain_Data)
	k = 0;
	for (i = 0; i < world_size; i = i + map_cell_info_t::Gib_Cell_Size()){
	for(j = 0; j < world_size; j = j + map_cell_info_t::Gib_Cell_Size())
	{
		worked_cell = new map_cell_info_t (this);
		worked_cell -> Collect_Terrain_Data (koord(i,j), world);
		tmp_array_of_cells.at(k) = worked_cell;
		k++;
	}}

	//from collected data we will calculate also avarage world height (uses and sets variables in factory_builder_t)
	Calculate_Average_World_Height();


	//now we have enough information to determine cell type, so we will do it and sort the cell into appropriate cell info list
	//to run this cycle we will exploit the fact, that no of included elements in tmp array is known and storet in k from info collection cycle
	for (i = 0 ; i < k ; i ++){

		const recognized_cell_types r = tmp_array_of_cells.at(i) -> Determine_Cell_Type( min_slope_percent, min_city_percent);
		//calcualtes cell type and store it into temp variable (cell itself does not know its type)



		/*
		//this prints found cell type onto the map in form of text tag - for debugging purposes
		cstring_t c =  csprintf("type is %s",((r==0)?"SEA":(r==1)?"SHORE":((r==2)?"LOW":(r==3?"HIGH":(r==4?"CITY":"UNUSABLE")))));
		char * b = new (char [20]); //do not delete this - it is needed for name tags, they will use it
		strcpy(b,c.chars());
		world->lookup((tmp_array_of_cells.at(i) -> Gib_Cell_Position()) + koord(1,1))->gib_kartenboden()->setze_text(b);
		*/

		//now we will check the cell type and take appropriate action
		if (r == 5 /*UNUSABLE*/){
			//if cell is unusable, throw it away and free memory
			delete (tmp_array_of_cells.at(i));
			continue;
		}

		//if we got here, cell is usable, so we need to reflect it - increase counter
		cl->usable_cells_total++;

		if (r == SEA){
			//if it is sea cell, insert it into sea list and go exploere next cell
			cl->sea->insert(tmp_array_of_cells.at(i));
			continue;
		}


		//if we got here, we surely work with land cell, so we will reflect it
		cl->usable_land_cells_total++;

		if (r == SHORE){
			//if it is shore cell, insert it into sea list and start exploring next cell
			cl->shore->insert(tmp_array_of_cells.at(i));
			continue;
		}

		if (r == LOW){
			//if it is low land cell, insert it into sea list and start exploring next cell
			cl->low->insert(tmp_array_of_cells.at(i));
			continue;
		}

		if (r == HIGH){
			//if it is high land cell, insert it into sea list and start exploring next cell
			cl->high->insert(tmp_array_of_cells.at(i));
			continue;
		}

		if (r == CITY){
			//if it is city cell, insert it into sea list and start exploring next cell
			cl->city->insert(tmp_array_of_cells.at(i));
			continue;
		}


	}


	/*now we will check counts of the lists and make corrections to eliminate
	pathologies like empty lists or lists with few celss - that will wreck the placement

	generally, if there are to few cells in the list, it will be merged to
	list with the closeset terrain type*/
	const uint16 cells_in_world = (world_size / map_cell_info_t::Gib_Cell_Size()) * (world_size / map_cell_info_t::Gib_Cell_Size());

	if ((cl->low->count()) < (cells_in_world>>4))
	{ //if there is less low cells than 1/16 of all cells

		//we will add all cells from low to cells in shore
		if (cl->low->count() > 0)
		{
			cell_list_iterator_tpl tmp (cl->low);
			tmp.begin();
			while (tmp.next())  cl->shore->insert( tmp.get_current() );
		}

		delete cl->low; //we will delete list for low
		                //we are delting just list of pointers not the palces in the memory

		(cl->low) = cl->shore; //and set low pointer to shore cell list pointer
							   //it will now point to same direction
		//this solution might slightly fraud random cell type selection, but only slightly

	} else if (cl->shore->count() < (cells_in_world>>6))
	{ //there needs to be else since both actions cannot be triggered
	  //if there is less shore cells than 1/64 of all cells

		//we will add all cells from shore to cells in low
		cell_list_iterator_tpl tmp (cl->shore);
		tmp.begin();
		while (tmp.next())  cl->low->insert(tmp.get_current());

		delete cl->shore; //we will delete listassocciated to shore
		cl->shore = cl->low; //and set shore pointer to low cell list pointer
	}

	if ((cl->high->count()) < (cells_in_world>>4))
	{ //if there is less high cells than 1/16 of all cells

		//we will add all cells from high to cells in low
		cell_list_iterator_tpl tmp (cl->high);
		tmp.begin();
		while (tmp.next())  cl->low->insert( tmp.get_current() );

		delete cl->high; //we will delete list assocciated with high
		(cl->high) = cl->low; //and set high pointer to low cell list pointer
	}

	if ((cl->city->count()) < (cells_in_world>>6))
	{ //if there is less high cells than 1/64 of all cells

		//we will add all cells from city to cells in low
		cell_list_iterator_tpl tmp (cl->city);
		tmp.begin();
		while (tmp.next())  cl->low->insert( tmp.get_current() );

		delete cl->city; //we will delete list assocciated with city
		(cl->city) = cl->low; //and set high pointer to low cell list pointer
	}


}




uint32 factory_builder_t::Compute_Total_Demand (const ware_besch_t *product, const factory_counts_table_tpl &factory_counts) const
{

	uint32 total_demand = 0;

	factory_counts_table_iterator_tpl demand_iterator (factory_counts);
	demand_iterator.begin();

	//scan all factories, that should be now placed on the map
	while (demand_iterator.next())
	{

		//currently scanned factory
		const fabrik_besch_t *current_fab = table.get (demand_iterator.get_current_key());


		uint8 i = 0;

		//for each factory, scan all its inputs
		while ( (current_fab -> gib_lieferant(i))  != NULL )
		{

			//if it is not our goods, stop
			if ( (current_fab -> gib_lieferant(i) -> gib_ware ()) != product )
			{
				i++;
				continue;
			}


			//average productivity of one tile of given factory = base productivity + 1/2 of range
			const uint16 tile_prod  = current_fab->gib_produktivitaet() + (current_fab->gib_bereich())/2;


			//get its current size
			koord dim = ( hausbauer_t::finde_fabrik( current_fab->gib_name()) ) -> gib_groesse (0);

			//total demand will be increased for production per tile * factory size * factory count
			total_demand += (tile_prod * dim.x * dim.y) * demand_iterator.get_current_value();

			//go for next product
			i++;

		}

	}

	return total_demand;

}


uint32 factory_builder_t::Compute_Total_Supply (const ware_besch_t *product, const factory_counts_table_tpl &factory_counts) const
{

	uint32 total_supply = 0;

	factory_counts_table_iterator_tpl supply_iterator (factory_counts);
	supply_iterator.begin();

	//scan all factories, that should be now placed on the map
	while (supply_iterator.next())
	{

		//currently scanned factory
		const fabrik_besch_t *current_fab = table.get (supply_iterator.get_current_key());


		uint8 j = 0;

		//for each factory, scan all its inputs
		while ( (current_fab -> gib_produkt(j))  != NULL )
		{



			//if it is not our goods, stop
			if ( (current_fab -> gib_produkt(j) -> gib_ware ()) != product )
			{
				j++;
				continue;
			}


			//average productivity of one tile of given factory = base productivity + 1/2 of range
			const uint16 tile_prod  = current_fab->gib_produktivitaet() + (current_fab->gib_bereich())/2;


			//get its current size
			koord dim = ( hausbauer_t::finde_fabrik( current_fab->gib_name()) ) -> gib_groesse (0);

			//total demand will be increased for production per tile * factory size * factory count
			total_supply += (tile_prod * dim.x * dim.y) * supply_iterator.get_current_value();

			//go for next product
			j++;

		}

	}

	return total_supply;

}



/* Calculates occurence for each industry (fills results to a hastable in form of (ind name),(occurence))
uses table and world size from factory builder
NEEDS TO BE CALLED ON PROPER DATA, IT THROWS 100 IF SOME SUPPLIER COULD NOT BE FOUND
(but it will take care of situation where there is no sea (will add no water industries))
*/

// ->gib_anzahl();
// unused, number of suppliers to spawn

void factory_builder_t::Calculate_Factory_Counts (factory_counts_table_tpl &factory_counts, const uint8 no_of_cities, const uint16 industry_density, const uint16 land_cell_count, const uint16 sea_cell_count) throw (int)
{

	//assumes that industry density is some integer number ranging from 1 to about 16

	if (table.count()==0) throw (101); //no industries were found

	//compute the total number of end consumers that will occur in the game
	//world size - temp variable storing world->gib_grosse()
	//calcualtes no of end consumer so that there is 1 per square of size 128x128 times level of industry density (or less if there are to few cities);
	//number 12 might need adjusting according to size of trees spawned by each consumer
	//2^14 = 16384 = 128x128
	const uint32 tcc = (no_of_cities>=((world_size*world_size)>>14))?(((world_size*world_size)>>14) * industry_density):(no_of_cities*industry_density);

	//compute no. of cells in the world - this variable is here only to make things easy to read
	//optimizer will probably kick it out.
	const uint16 cells_in_world = ((world_size/map_cell_info_t::Gib_Cell_Size()) * (world_size/map_cell_info_t::Gib_Cell_Size()));

	//ensure, that too watery or hilly worlds will get fewer factories
	//if land cells are less than 1/2 of the world, reduce number of end consumer
	const uint16 total_consumer_count = 1 + ((tcc*land_cell_count)/cells_in_world);

	//debug output
	printf ("tcc: %i, cells: %i, land cells: %i, consumers: %i\n",tcc, cells_in_world, land_cell_count, total_consumer_count);

	//iterator to scan list of all loaded factory besches
	stringhashtable_iterator_tpl<fabrik_besch_t *> factory_besch_iterator(table);
	factory_besch_iterator.begin();

	//  USD TYPES:
	//	typedef uint16 factory_count_tpl;
	//	typedef stringhashtable_tpl<factory_count_tpl> factory_counts_table_tpl;
	//	typedef stringhashtable_iterator_tpl<factory_count_tpl> factory_counts_table_iterator_tpl;

	//create main iterator for factory table
    //factory_counts_table_iterator_tpl fc_iterator(factory_counts);


	//creates list of all end consumers by scanning through existing factory beshes
    while(factory_besch_iterator.next())
	{

		const fabrik_besch_t *info = factory_besch_iterator.get_current_value();
		if(info->gib_produkt(0) == NULL)
		{
			//stores fonud consumers in a table
			factory_counts.put(info->gib_name(), 0);
		}

		if (factory_counts.count() >= total_consumer_count) break;	//emergency break to prohibit too wild industry counts on.
																//(limit number of found consumers)

    }

	//something must gone wrong (bad memory?), this situation should not occur
	if (factory_counts.count() == 0) throw (102);


	/*now we have consumer table filled with data we will cycle through it
	and on each run we will add their distribution weight to their occurence
	until the total of all consumers on map reaches over computed consumer count
	value*/
	factory_counts_table_iterator_tpl iter2 (factory_counts);
	uint16 consumer_count = 0;
	uint8 distribution_weight;

	//outer loop, loop until we have enough of final consumers
	while (consumer_count < total_consumer_count){

		iter2.begin();

		//inner loop: go through all found consumers and increase their counts
		//with respect to ditribution weights
		while((consumer_count < total_consumer_count) && iter2.next())
		{
			//we will find value of dist. weight the consumer to which iter2 is pointing to
			distribution_weight = (table.get(iter2.get_current_key())) -> gib_gewichtung();

			//emergency break to prevent too many consumers if distribution weight is high, or to prohibit consumers with too high weight to occupy all spots
			if ( (consumer_count + distribution_weight) > total_consumer_count ) distribution_weight = 1;

			//increase count for this consumer by dist weight
			iter2.access_current_value() += distribution_weight;

			//increase total count of spawned consumers
			consumer_count += distribution_weight;

		}

    }


	factory_counts_table_iterator_tpl fc_iterator1 (factory_counts);
	stringhashtable_iterator_tpl<fabrik_besch_t *> iter1 (table);


	bool finished = FALSE;


	/*
	Factory count algorithm:

    Algorithm uses factory counts table (filled by end consumers).
	It takes each factory in the table, scans through its inputs
	and computes how many of each input will be needed to satisfy need of all factories that need this
	kind of input on the map (so not only the need of current factory)

    (One might think why not scan goods table directly? Method with factories is faster on smaller maps
	and is easier to program - uses only datastructures know to me already.)

	Then algorithm scans through the table of all loaded factories and checks if some factory is not producing
	required kind of goods. If yes, then it adds it to counts table - this is repeated while total supply is
	lower than total demand.

    Algorithm loops over the factory counts table until there is a loop which makes no changes.

	SO IF SOMEONE MAKES FATORY THAT PRODUCES SAME KIND OF GOODS AS IT CONSUMES (AND OUTPUT IS LOWER THAN INPUT)
	THERE WILL BE ENDLESS LOOP.
	*/

	while ( !finished )
	{
		//begin new loop if factory count table
		fc_iterator1.begin();

		finished = TRUE;

		//go through factory counts table
		while (fc_iterator1.next())
		{

			//currently scanned factory
			const fabrik_besch_t *demand_fab = table.get (fc_iterator1.get_current_key());

			//input list counter
			uint8 i = 0;

			//for each factory, scan all its inputs
			while ( (demand_fab -> gib_lieferant(i))  != NULL )
			{

				//input goods besch
				const ware_besch_t *demanded_goods = demand_fab -> gib_lieferant(i) -> gib_ware ();

				//find demand for this input
				//decrease it a bit, as otherwise there will be allways excess supply (since this counting
				//generates allways supply >= demand. So if demand is decreased we have better chance for equilibrium
				const uint32 demand = (Compute_Total_Demand (demanded_goods, factory_counts)) * 0.9;

				uint32 supply = Compute_Total_Supply (demanded_goods, factory_counts);


				//debug
				//dbg->message("factory_builder_t::Calculate_Factory_Counts()", "Processing:%s, count:%i, demanded product:%s, supply:%i, demand:%i",demand_fab->gib_name(),fc_iterator1.get_current_value(),demanded_goods->gib_name(),supply,demand);


				//until supply existing on the map is not sufficient
				while (supply < demand)
				{

					//if we got here, it means that we will add factory (mark dirty)
					finished = FALSE;

					//iterator to table of all loaded factory besches
					iter1.begin();

					//scans through table (of all possible facotries), try to find suppliers, add them
					while (iter1.next())
					{
						const fabrik_besch_t *current_fab = iter1.get_current_value();

						//terrain appropriacy check - it will reduce counts of water factories
						//on maps with small amounths of water:
						//check if factory is water factory
						if (current_fab -> gib_platzierung () == fabrik_besch_t::Wasser)
						{

							//random number from range (0 ; cells_in_world/4) is chosen. If it is less than
							//sea cell count, skip this loop, go to next factory;
							//this means that worlds with water coverage less than 25% will
							//have smaller count of water based industries (count = 0 on worlds with no water)

							/*THIS CAN LEAD TO EXCEPTION IF SOME GOODS CAN BE SUPPLIED ONLY BY WATER BASED INDUSTRIES!*/
							if ( (1 + simrand (cells_in_world/4)) > sea_cell_count )
							{
								//go to next factory
								continue;
							}

						}



						uint8 j = 0; //product counter

						//scans through all products of this factory to find if it produces our type
						while (current_fab -> gib_produkt(j) != NULL)
						{
							const fabrik_produkt_besch_t *current_output = current_fab -> gib_produkt(j);
							j++;

							//check if product is same as the input product we are searching for
							//(actually this is only comparism of pointers to warebesh)
							if ((current_output -> gib_ware()) != demanded_goods)
							{
								continue;
							}

							//Compute tile productivity of given industry
							const uint16 tile_prod = current_fab->gib_produktivitaet() + ((current_fab->gib_bereich())/2);

							//get its current size
							koord dim = ( hausbauer_t::finde_fabrik( current_fab->gib_name()) ) -> gib_groesse (0);

							//compute its total productivity
							const uint16 total_sup = tile_prod * dim.x * dim.y;

							//compute how many factories of this kind will be placed
							// = distribution_weight or 1 if distribution weight will lead to too high count
							const uint8	 dist_w = ((supply + (total_sup*(current_fab -> gib_gewichtung()))) < demand) ? (current_fab -> gib_gewichtung()) : 1;

							supply += total_sup*dist_w;


							//debug
							//dbg->message("factory_builder_t::Calculate_Factory_Counts()", "adding:%s, count:%i",current_fab->gib_name(),dist_w);

							//add the factory to counts
							//does it exist there?
							if (factory_counts.get(current_fab -> gib_name()))
							{
								//exist, increase count
								*(factory_counts.access(current_fab -> gib_name())) += dist_w;

							} else {
								//does not, add it
								factory_counts.put(current_fab ->gib_name(), dist_w);
							}


						}

					}

					//if we got here and count is still 0, we were unable to find supplier
					if (supply == 0) throw 100;
				}

				i++;


			}


		}



	}


/*
	//debug output to std out

	//THIS CAUSED RANDOM CRASHES ON MY SYSTEM!
	//(memory cannot be rerad - output halted in the middel of the word)

	//However stats are nice and informative, so maybe better compiler can
	//compile this properly and it can be then turned on...

	uint8 df = 0;
	factory_counts_table_iterator_tpl iter3 (factory_counts);
	printf("Computed factory counts:\n");
	while(iter3.next()){
		printf("%s=%i;",iter3.get_current_key(),iter3.get_current_value());
		for (uint8 a = strlen (csprintf("%s=%i;",iter3.get_current_key(),iter3.get_current_value())); a <26; a++) printf(" ");
		if ((df%3) == 2) printf("\n");
		df++;
	}

	printf("\n");
*/
}



//builds stop around factory if it is on owater. If factory is on ground, does nothing
void factory_builder_t::Build_Stop_Around_Water_Factory (const fabrik_besch_t *info, const uint8 rotate, koord3d pos)
{
	///////////////////////////////////////////////////////////
	////this piece of code is adopted from old fabrik bauer////
	///////////////////////////////////////////////////////////

	//stop here if we are not on the water
    if(info->gib_platzierung() != fabrik_besch_t::Wasser) return;

	//get description of building (finds its size)
	const haus_besch_t *besch = hausbauer_t::finde_fabrik( info->gib_name() );

	//get its current size with respect to rotation
	koord dim = besch -> gib_groesse (rotate);
    koord k;

	//create new stop entry in stop list
	halthandle_t halt = world -> gib_spieler(0) -> halt_add(pos.gib_2d());

	//set its properties
	halt->set_pax_enabled( true );
	halt->set_ware_enabled( true );
	halt->set_post_enabled( true );

	//mark rectangle arounfd the factory as stop area.
	for(k.x=pos.x-1; k.x<=pos.x+dim.x; k.x++)
	{
	    for(k.y=pos.y-1; k.y<=pos.y+dim.y; k.y++)
		{

			if(! halt->ist_da(k))
			{
				const planquadrat_t *plan = world->lookup(k);

				if(plan != NULL)
				{

					grund_t *gr = plan->gib_kartenboden();

					if(gr->ist_wasser() && gr->gib_weg(weg_t::wasser) == 0)
					{
						gr -> neuen_weg_bauen (new dock_t(world), ribi_t::alle, world->gib_spieler(0));
						halt -> add_grund( gr );
					}

				}

		    }

		}

	}

	//set stop sign
    world->lookup( halt->gib_basis_pos() )->gib_kartenboden()->setze_text( translator::translate(info->gib_name()) );

}






fabrik_t* factory_builder_t::Build_Industry(const fabrik_besch_t *info, const uint8 rotate, koord3d pos, spieler_t *sp)
{

	//method itself will check wether the factory is water type or not.
	Build_Stop_Around_Water_Factory (info, rotate, pos);


	///////////////////////////////////////////////////////////
	////this piece of code is adopted from old fabrik bauer////
	///////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////
	////this method might need some update reagarding
	////new factory data
	///////////////////////////////////////////////////////////


    fabrik_t * fab = new fabrik_t(world, pos, sp, info);
    int	i;

    vector_tpl<ware_t> *eingang = new vector_tpl<ware_t> (info->gib_lieferanten());
    vector_tpl<ware_t> *ausgang = new vector_tpl<ware_t> (info->gib_produkte());

    for(i=0; i < info->gib_lieferanten(); i++)
	{
		const fabrik_lieferant_besch_t *lieferant = info->gib_lieferant(i);
		ware_t ware(lieferant->gib_ware());

		ware.max = lieferant->gib_kapazitaet() << fabrik_t::precision_bits;

		eingang->append(ware);
    }

    for(i=0; i < info->gib_produkte(); i++)
	{
		const fabrik_produkt_besch_t *produkt = info->gib_produkt(i);
		ware_t ware(produkt->gib_ware());

		ware.max = produkt->gib_kapazitaet() << fabrik_t::precision_bits;

		ausgang->append(ware);
    }

    fab->set_eingang( eingang );
    fab->set_ausgang( ausgang );
    fab->set_prodbase( 1 );
    fab->set_prodfaktor( info->gib_produktivitaet() +  simrand(info->gib_bereich()) );

	//physically place the object on the map
    fab->baue(rotate, true);

	//add newly created factory to word list of existign factories
    world->add_fab(fab);

	//and now create special for the factory if any
	//this includes powerplant wires, stops are build in the begining of this method
    Construct_Industry_Special (fab);

    return fab;

}




bool factory_builder_t::Place_Factory_Into_Cell (map_cell_info_t *cell, const fabrik_besch_t *factory, const uint8 rotation, const uint8 priority)
{

	//store cell position to a vriable to make things more clear
	const koord cell_pos = cell -> Gib_Cell_Position();


	//to prevent overcrowding of some cells
	//we will not put factory into cell if there are some already here and
	//priority of placement is low
	if ((cell -> Gib_No_of_Factories_in_Cell()) > (priority + 1) )
	{

		/*
		//debugging tag
		cstring_t c =  csprintf("This cell was considered full. %i factories allready.", cell -> Gib_No_of_Factories_in_Cell());
		char * b = new (char [200]); //do not delete this - it is needed for name tags, they will use it
		strcpy(b,c.chars());
		world->lookup(cell_pos+koord(4 + cell -> Gib_No_of_Factories_in_Cell(),4 + cell -> Gib_No_of_Factories_in_Cell()))->gib_kartenboden()->setze_text(b);
		*/

		return FALSE;

	}


	const koord dimension = (factory -> gib_haus()) -> gib_groesse(rotation); //we will find size of factroy (already accounts rotation)
	koord position = koord::invalid;
	uint8 i = 0;
	factory_place_finder_t *place_finder; //this will be used to store proper placefinder (type will be determined on base of factory type)

	//at first we will try to place factory randomly in the cell
	while (i < NUMBER_OF_RANDOM_TRIES_IN_CELL)
	{
		//choose some random location
		const koord current_pos = cell_pos + koord (simrand( map_cell_info_t::Gib_Cell_Size () - 1),simrand( map_cell_info_t::Gib_Cell_Size () - 1));

		//create appropriate instance of placefinder
		switch (factory->gib_platzierung())
		{
			case (fabrik_besch_t::Wasser): place_finder = new water_factory_place_finder_t (current_pos, dimension.x, dimension.y, world, priority);
			break;

			case (fabrik_besch_t::Stadt) : place_finder = new city_factory_place_finder_t (current_pos, dimension.x, dimension.y, world, priority);
			break;

			default: place_finder = new land_factory_place_finder_t (current_pos, dimension.x, dimension.y, world, priority);

		}

		//if place can be found remeber position and stop
		if (place_finder->Check_Place())
		{
			//it needs to be done this way (retrieve pos from finder) since
			//finder is inteded to be able to change terrain and eventually offer nearby place
			position = place_finder->Get_Place();

			//stop the cycle
			break;

		}

		delete place_finder;
		i++;
	}


	if ( position == koord::invalid ){
	//if we got here, factory still was not build, so we wil systematically search whole cell
	//fact is that even on most rough terrain, random will succed with about rate of 95%.
		for ( i = 0; i < map_cell_info_t::Gib_Cell_Size (); i++)
		{
			for ( uint8 j = 0; j < map_cell_info_t::Gib_Cell_Size (); j++){

				const koord current_pos = cell_pos + koord (j,i);

				//initiate place finder for the place
				switch (factory->gib_platzierung())
				{
					case (fabrik_besch_t::Wasser): place_finder = new water_factory_place_finder_t (current_pos, dimension.x, dimension.y, world, priority);
					break;

					case (fabrik_besch_t::Stadt): place_finder = new city_factory_place_finder_t (current_pos, dimension.x, dimension.y, world, priority);
					break;

					default: place_finder = new land_factory_place_finder_t (current_pos, dimension.x, dimension.y, world, priority);

				}

				//if place can be found remeber it and stop
				if (place_finder->Check_Place())
				{
					//it needs to be done this way (retrieve pos from finder) since
					//finder is inteded to be able to change terrain and eventually offer nearby place
					position = place_finder->Get_Place();

					//stop the cycle
					break;

				}

				delete place_finder;


			}

			//do not search if place was already found
			if ( position != koord::invalid )
			{
				break;
			}

		}

	}


	//end if no siutable location was found
	if ( position == koord::invalid )
	{
		/*
		//debugging let us know we were unable to find place in this cell
		cstring_t c =  csprintf("finding of place in this cell was not successful");
		char * b = new (char [50]); //do not delete this - it is needed for name tags, they will use it
		strcpy(b,c.chars());
		world->lookup(cell_pos+koord(4,4))->gib_kartenboden()->setze_text(b);
		*/


		return FALSE;
	}

	//build factory          //makes 3d coordinate from 2d one
	Build_Industry(factory , rotation, (world -> lookup(position)) -> gib_kartenboden() -> gib_pos(), player);

	//this function only increses factory count in this cell (does no building at all)
	//it is for statistical purposes only
	cell -> Add_Factory_to_Cell ();

	delete place_finder; //remeber, placefinder remanided undeleted upon succesfull try
	return TRUE;
}



void factory_builder_t::Place_Resource_Bound_Factories (factory_counts_table_tpl &factory_counts, cell_lists_t &cl)
{

//factory_builder_t::map_cell_info_t *factory_builder_t::cell_lists_t::Gib_nth_Closest_Cell
// (const cell_list_tpl *my_list, const koord pos, const uint16 dist, const uint16 n) const

	//get table with factory types and numbers how much examples of each factory should be built
	factory_counts_table_iterator_tpl count_iterator (factory_counts);
	count_iterator.begin();

	//scan through the table
	while (count_iterator.next())
	{
		//get description for factory type we will work with this round
		const fabrik_besch_t *current_factory = gib_fabesch( count_iterator.get_current_key() );

		//resource bound (can be replaced by gib resource bound, once it will be in pak files)
		uint8 resource_bound = 0;

		/*until the end of loop, all this are temp structures to find the value of resource_bound
		  which once it will be stored in pak files, there will be no more need for them
		*/
		factory_counts_table_iterator_tpl tmp_resource_bound_iterator (tmp_resource_bound_table);
		//set it to start
		tmp_resource_bound_iterator.begin();
		//scan whole table and each run compare, if name of pointed factory is same as name of
		//factory which is being placed now. If such is found, get resource_bound and stop
		while (tmp_resource_bound_iterator.next())
		 if ( strcmp( tmp_resource_bound_iterator.get_current_key(), current_factory -> gib_name()) == 0)
		{
			 resource_bound = tmp_resource_bound_iterator.get_current_value();
			 break;

		 }
		 //end of temp structures

		 if (resource_bound == 0)
		 {
			 //if resource bound is 0, skip this factory type in resource bound placing
			 //by going back to the while
			 continue;

		 }



		//factory count tpl is just ordinal variable type - put number of factories to place in it,
		const factory_count_tpl count_to_be_placed = (count_iterator.get_current_value() * resource_bound) / 100;

		//this will store cell list we will be placing factory into
		cell_list_tpl *cell_list;



		//for each single factory do place choosing and palcement
		//this style allows to choose different location on each try

		/*we need to choose list each time whne placing factory,
		since otherwise all factoris of one type will fall to the same list
		and that looks wierd if city or shore list is chosen (using random land list)*/

		//for each factory from given factory count
		//factory_count_tpl is ordinal variable
		for (factory_count_tpl placed_factories = 0; placed_factories < count_to_be_placed; )
		{

			//choose proper list, store result as list pointer
			switch (current_factory -> gib_platzierung())
			{
				//factory with placement land can be on shore, low, high or in the city
				case fabrik_besch_t::Land   : cell_list = cl.Get_Random_LH_Cell_List();
				break;

				case fabrik_besch_t::Wasser : cell_list = cl.sea;
				break;

				case fabrik_besch_t::Stadt  : cell_list = cl.city;
				break;

				case fabrik_besch_t::Low    : cell_list = cl.low;
				break;

				case fabrik_besch_t::High   : cell_list = cl.high;
				break;

				case fabrik_besch_t::Shore  : cell_list = cl.shore;
				break;

				//this should be unreachable, but to make sure
				default: cell_list = cl.low;

			}

			const koord pos = (cell_lists_t::Get_Random_Cell(cell_list)->Gib_Cell_Position()) + koord (map_cell_info_t::Gib_Cell_Size ()/2, map_cell_info_t::Gib_Cell_Size ()/2);

			/*
			//this prints found cell type onto the map in form of text tag - for debugging purposes
			cstring_t c =  csprintf("Beacon type: %s",current_factory -> gib_name());
			char * b = new (char [50]); //do not delete this - it is needed for name tags, they will use it
			strcpy(b,c.chars());
			world->lookup(pos)->gib_kartenboden()->setze_text(b);
			*/



			for (uint8 j = 0; ((j < 5) && (placed_factories < count_to_be_placed)); j++)
			{

				if (Place_Factory_Into_Cell( cell_lists_t::Gib_nth_Closest_Cell (cell_list, pos, map_cell_info_t::Gib_Cell_Size (), j ) , current_factory, simrand (4), j + 3))
				{
					placed_factories++;
				}

			}


		//end of for cycle
		}


		count_iterator.access_current_value() -= placed_factories;


	//end of while
	}



}





void factory_builder_t::Randomly_Place_Factories (const factory_counts_table_tpl &factory_counts, cell_lists_t &cl)
{

	//get table with factory types and numbers how much examples of each factory should be built
	factory_counts_table_iterator_tpl count_iterator (factory_counts);
	count_iterator.begin();

	//scan through the table
	while (count_iterator.next())
	{
		//get description for factory type we will work with this round
		const fabrik_besch_t *current_factory = gib_fabesch( count_iterator.get_current_key() );

		//factory count tpl is just ordinal variable type - put number of factories to place in it,
		const factory_count_tpl current_factory_count = count_iterator.get_current_value();

		//this will store cell list we will be placing factory into
		cell_list_tpl *cell_list;

		//this variable will store looseness for given factory. (it can be omitted once
		//data can be retrieved from pak file - replaced bi function gib_looseness ;-)
		uint8 looseness = 0;


		/*this whole set of lines is here to find looseness value in the table.
		Once this date will be stored in the pak files, all these lines can be repalaced
		by lookup to pak file during looseness declaration - no variable needed.
		*/
		//create iterator for looosness table
		factory_counts_table_iterator_tpl tmp_looseness_iterator (tmp_loacation_looseness_table);
		//set it to start
		tmp_looseness_iterator.begin();
		//scan whole table and each rune compare, if name of pointed factory is same as name of
		//factory which is being placed now. If such is found, get looseness and stop
		while (tmp_looseness_iterator.next())
		 if ( strcmp( tmp_looseness_iterator.get_current_key(), current_factory -> gib_name()) == 0)
		{
			 looseness = tmp_looseness_iterator.get_current_value();
			 break;

		}

		//for each single factory do place choosing and palcement
		//this style allows to choose different location on each try

		/*we need to choose list each time whne placing factory,
		since otherwise all factoris of one type will fall to the same list
		and that looks wierd if city or shore list is chosen (using random land list)*/

		//for each factory from given factory count
		for (factory_count_tpl i = 0; i < current_factory_count; i++)
		{

			bool is_loose = FALSE;

			//check if this current factory will be "lose" from its preffered placement or not
			//as this property does not apply to water factories, omit them
			if ( ( (1+simrand(1000)) <= (looseness*10) ) && ((current_factory -> gib_platzierung()) != fabrik_besch_t::Wasser) )
			{
				//if factory will be loose we choose its placement from all available land places
				//not just preffered ones
				cell_list = cl.Get_Random_Land_Cell_List();
				is_loose = TRUE;

			//if factory is not chosen to be loose, set its location to preffered location list
			} else
			{

				//choose proper list, store result as list pointer
				switch (current_factory -> gib_platzierung())
				{
					//factory with placement land can be on shore, low, high or in the city
					case fabrik_besch_t::Land   : cell_list = cl.Get_Random_Land_Cell_List();
					break;

					case fabrik_besch_t::Wasser : cell_list = cl.sea;
					break;

					case fabrik_besch_t::Stadt  : cell_list = cl.city;
					break;

					case fabrik_besch_t::Low    : cell_list = cl.low;
					break;

					case fabrik_besch_t::High   : cell_list = cl.high;
					break;

					case fabrik_besch_t::Shore  : cell_list = cl.shore;
					break;

					//this should be unreachable, but to make sure
					default: cell_list = cl.low;

				}

			}

			//this basically means, that it will try to place factory x times to random different cells before geving up, each placement has higher priority
			// -- with priority there is small "hack" - we will set it to high, if factory is loose. This allows to override "near road" check for buildings that have
			//location "city" as default (this caused troubles when placing such buildings in regular non-city land cells)
			for (uint8 j = 0; (j < MAX_TRIED_CELLS) && !Place_Factory_Into_Cell( cell_lists_t::Get_Random_Cell(cell_list) , current_factory, simrand (4), (is_loose)?(MAX_TRIED_CELLS/2 +1):j); j++);
		}

	}

}




void factory_builder_t::Construct_Industry_Special (fabrik_t *fab)
{

	////////////add powerline to powerplant///////////////////////////
	//if facotry name is powerplant and powerlines exist as pak///////
	if( (strcmp(fab->gib_name(), "Kohlekraftwerk") == 0) && (skinverwaltung_t::pumpe) )
	{
		const koord3d pos = (fab->gib_pos()) + koord3d(2,0,0);
		sint8 k = 0;

		//search on the koordinates near powerplant
		//for a free spot
		//have tree tries in row east from powerplant
		//(it can be easily extended to more places)
		while(k<3)
		{

			//at first check if such place exist (for sure)
			if(world->lookup(pos+koord3d(0,k,0)))
			{
				if
				(
				  //check if square is free and if it is flat
				  //this line looks so ugly because we have 3d koordinates, we need to make 2d from them
				  //and get a ground type for given square and check its properties
				  (world->lookup(((pos+koord3d(0,k,0)).gib_2d()))->gib_kartenboden()->ist_natur()) &&
				  (world->lookup(((pos+koord3d(0,k,0)).gib_2d()))->gib_kartenboden()->gib_grund_hang() == hang_t::flach)
			    )
				{
					//if everything matches kill while, we have good spot
					break;
				}

			}

			k++;
		}


		//if one of 3 tries was successful k is at most 2
		//build powerline
		if( k<3)
		{

			pumpe_t *pumpe = new pumpe_t(world, pos+koord3d(0,k,0), player);
			pumpe->setze_fabrik(fab);

			world->lookup(pos+koord3d(0,k,0))->obj_loesche_alle(NULL);
			world->lookup(pos+koord3d(0,k,0))->obj_add(pumpe);

		}

    }

}



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
////this piece of code uses parts of old Volkers code fabrik bauer////
//////////////////////////////////////////////////////////////////////

//connects industry to closest supplier - no matter how far it is
//if it does not exist or facotry does not require any does nothing
//this method has been updated to link 2 closest suppliers (if they exist)
void factory_builder_t::Connect_Industry_to_Closest_Supplier (const fabrik_t *worked_factory, const karte_t *world)
{

	//get list of all existing factories from world structures
	const slist_tpl <fabrik_t *> & list = world->gib_fab_list();

	//get list of all input goods for given factory
	const vector_tpl <ware_t> * ein = worked_factory->gib_eingang();

	//scans through the list of input goods
	for(uint8 i=0; i<(ein->get_count()); i++)
	{

		//tries to find closest factory so we will rember distance of each possible supplier
		//and remeber the distance and pointer to closest one so far
		uint16 shortest_dist = world -> gib_groesse();
		uint16 shortest_dist2 = world -> gib_groesse();
		fabrik_t *closest_factory = NULL;
		fabrik_t *closest_factory2 = NULL;

		//get given type of input goods
		const ware_t ware = ein->get(i);

		//creates another iterator for factory list
		slist_iterator_tpl <fabrik_t *> ziele (list);

		//scans through the factory list using this iterator
		while(ziele.next())
		{

			//gets list of output goods for given factory from the list
			const vector_tpl <ware_t> * aus = ziele.get_current()->gib_ausgang();
			bool ok = false;

			//scan through the list of outputs
			for(uint8 i=0; i<(aus->get_count()); i++)
			{

				//if match between goods types is found, mark this facotry usable
				if(aus->get(i).gib_typ() == ware.gib_typ())
				{
					ok = true;
					break;
				}

			}

			//if match for goods between current factory in quellen and ziele was found
			if(ok)
			{

				//calcualte distance between factories
				const koord pos1 = worked_factory->gib_pos().gib_2d();
				const koord pos2 = ziele.get_current()->gib_pos().gib_2d();
				const uint16 dist = sqrt (((pos1.x - pos2.x)*(pos1.x - pos2.x)) + ((pos1.y - pos2.y)*(pos1.y - pos2.y)));

				//if current factory is closer than older one update the values;
				if(dist < shortest_dist)
				{
					//move currenctlly svaved factory to second position
					shortest_dist2 = shortest_dist;
					closest_factory2 = closest_factory;

					//add newly found closest factory
					shortest_dist = dist;
					closest_factory = ziele.get_current();

				}

			}


		}

		//if we have found closest consumer for given output goods - link it!
		if (closest_factory != NULL)
		{
			//connect
			closest_factory->add_lieferziel(worked_factory->gib_pos().gib_2d());
		}

		//if second closest supplier exists and IF it is not end consumer
		//and the list of consumers of current factory is not very full, connect it
		//end consumer - has no outpus (that is important as there usually is lot of end consumers
		//consumer list can get full (especially critical for raffineries))
		if ((closest_factory2 != NULL) && (worked_factory -> gib_ausgang() -> get_count() != 0)
		 && (closest_factory2 -> gib_lieferziele().get_count() < 5))
		{
			closest_factory2->add_lieferziel(worked_factory->gib_pos().gib_2d());
		}

	//end of cycle scan through goods list
	}

}


//connect industry to closest consumer - no matter how far it is
//if it does not exist or facotry does not require any does nothing
void factory_builder_t::Connect_Industry_to_Closest_Consumer (fabrik_t *worked_factory, const karte_t *world)
{


	//get list of all existing factories from world structures
    const slist_tpl <fabrik_t *> & list = world->gib_fab_list();

	//get list of all output goods for given factory
	const vector_tpl <ware_t> * aus = worked_factory->gib_ausgang();

	//scans through the list
	for(uint8 i=0; i<aus->get_count(); i++)
	{

		//tries to find closest factory so we will rember distance of each possible consumer
		//and remeber the distance and pointer to closest one so far
		uint16 shortest_dist = world -> gib_groesse();
		fabrik_t *closest_factory;

		//paranoid check - ensure proper break if no consumer is found (this SHOULD not happen)
		bool found = false;


		//get given type of goods
		const ware_t ware = aus->get(i);

		//creates iterator for factory list
		slist_iterator_tpl <fabrik_t *> ziele (list);

		//scans through the factory list using this iterator
		while(ziele.next())
		{

			//gets list of input goods for given factory
			const vector_tpl <ware_t> * ein = ziele.get_current()->gib_eingang();
			bool ok = false;

			//scan through it
			for(uint8 i=0; i<(ein->get_count()); i++)
			{

				//if match between goods types is found, mark this facotry usable
				if(ein->get(i).gib_typ() == ware.gib_typ())
				{
					ok = true;
					break;
				}

			}

			//if match for goods between current factory in quellen and ziele was found
			if(ok)
			{

				//calcualte distance between factories
				const koord pos1 = worked_factory->gib_pos().gib_2d();
				const koord pos2 = ziele.get_current()->gib_pos().gib_2d();
				const uint16 dist = sqrt (((pos1.x - pos2.x)*(pos1.x - pos2.x)) + ((pos1.y - pos2.y)*(pos1.y - pos2.y)));

				//if current factory is closer than older one update the values;
				if(dist < shortest_dist)
				{
					shortest_dist = dist;
					closest_factory = ziele.get_current();
					found = true;
				}

			}


		}

		//if we have found closest consumer for given output goods - link it!
		if (found == true)
		{
			//connect
			worked_factory->add_lieferziel(closest_factory->gib_pos().gib_2d());

		}



	//end of cycle scan through output goods list
	}

}




//connect factory to all consumers in distance below argument n, and to some
//random consumers above that distance (chance of connecting decreases with distance)
void factory_builder_t::Connect_Industry_to_Random_Neighbour_Consumers (const uint32 min_dist, fabrik_t *worked_factory, const karte_t *world)
{
	///////////////////////////////////////////////////////////
	////this piece of code is adopted from old fabrik bauer////
	///////////////////////////////////////////////////////////

	//list of all factories existing on the map
    const slist_tpl <fabrik_t *> & list = world->gib_fab_list();


	const vector_tpl <ware_t> * aus = worked_factory->gib_ausgang();

	for(uint8 i=0; i<aus->get_count(); i++)
	{

		ware_t ware = aus->get(i);
		slist_iterator_tpl <fabrik_t *> ziele (list);

		while(ziele.next())
		{

			const vector_tpl <ware_t> * ein = ziele.get_current()->gib_eingang();
			bool ok = false;

			for(uint8 i=0; i<ein->get_count(); i++)
			{

				if(ein->get(i).gib_typ() == ware.gib_typ())
				{
					ok = true;
					break;
				}

			}

			if(ok)
			{

				const koord pos1  = worked_factory->gib_pos().gib_2d();
				const koord pos2  = ziele.get_current()->gib_pos().gib_2d();
				const uint16 dist = sqrt (((pos1.x - pos2.x)*(pos1.x - pos2.x)) + ((pos1.y - pos2.y)*(pos1.y - pos2.y)));

				//connect factories together if they are very close (given in parameter) or at some random case
				//expressin behind || tries to create some randome with prefference of closer distances
				if(dist < min_dist || ( sqrt( dist*(world->gib_groesse()) ) < simrand( (world->gib_groesse())*0.75 ) ) )
				{

					//connect
					//connectrs works only if there is still free place in vecotr of consumers
					worked_factory->add_lieferziel(ziele.get_current()->gib_pos().gib_2d());


				}

			}

		}

	}

}


/////////////////////////////////////////////////////////////////
//This procedures call procedures above to connect everything////
/////////////////////////////////////////////////////////////////
//calls methods above for all factories in the world
void factory_builder_t::Make_Connections ( const uint16 min_dist, const karte_t *world )
{

	//collect functuions does not check for duplicate entries
	//thes needs to be done on higher level. (when inserting to the vector)


	//get list of all existing factories from world structures
    const slist_tpl <fabrik_t *> & list = world->gib_fab_list();
	//create iterator for it
    slist_iterator_tpl <fabrik_t *> quellen (list);

	//scan through whole list
    while(quellen.next())
	{
		//call this function for each factory in it
		Connect_Industry_to_Closest_Supplier (quellen.get_current(), world);
	}


	quellen.begin();
	while(quellen.next())
	{
		Connect_Industry_to_Closest_Consumer (quellen.get_current(), world);
	}


	quellen.begin();
	while(quellen.next())
	{
		Connect_Industry_to_Random_Neighbour_Consumers (min_dist, quellen.get_current(), world);
	}

}



//calls methods above for given factory/////////
//it is intended to be called by user///////////
void factory_builder_t::Connect_Given_Industry ( fabrik_t *worked_factory, const karte_t *world )
{

	Connect_Industry_to_Closest_Supplier (worked_factory, world);
	Connect_Industry_to_Closest_Consumer (worked_factory, world);

	//min distance is here calculated bit roughly
	Connect_Industry_to_Random_Neighbour_Consumers (world->gib_groesse()/3, worked_factory, world);

}




////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//Special buildigns functions///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

//place special building to a cell (works for land cells only now)
bool factory_builder_t::Place_Spc_Bdg_into_Cell (map_cell_info_t *cell, const haus_besch_t *bldg)
{

	//do not build special building to a cell, which already has one or has more than one factory in it.
	if ((cell -> Has_Special()) || (cell -> Gib_No_of_Factories_in_Cell() > 0))
	{
		//stop
		return FALSE;

	}

	factory_place_finder_t *my_finder; // (pos -> gib2d(), , , world, 0);
	//	factory_place_finder_t (const koord k, const uint8 x, const uint8 y, const karte_t *w, const uint8 p);

	//store cell position
	const koord cell_pos = cell -> Gib_Cell_Position();
	//choose rotation for a factory
	const uint8 rotation = simrand(4);
	//we will store size of a factroy
	const koord dimension = bldg -> gib_groesse (rotation);
	koord position = koord::invalid;

	//at first we will try to place factory randomly in the cell
	//do this until max tries were reached or place was found
	for (uint8 i = 0; (i < NUMBER_OF_RANDOM_TRIES_IN_CELL) && (position == koord::invalid) ; i++)
	{
		//choose some random location
		const koord current_pos = cell_pos + koord (simrand( map_cell_info_t::Gib_Cell_Size () - 1),simrand( map_cell_info_t::Gib_Cell_Size () - 1));

		my_finder = new land_factory_place_finder_t (current_pos, dimension.x, dimension.y, world, 0);

		//this function checks the suitability of the place
		//(it can find new one nearby durign this procedure)
		if (my_finder -> Check_Place())
		{
			//if check for suitability of the place was successful, get place

			//it needs to be done this way (retrieve pos from finder) since
			//finder is inteded to be able to change terrain and eventually offer nearby place
			position = my_finder->Get_Place();

		}

		//ths placefinder will be of no more use to us now
		delete my_finder;

	}

	//if we got here, place was not found yet, so we will systematically search whole cell
	//fact is that even on most rough terrain, random will succed with about rate of 95%.
	for (uint8 i1 = 0; (i1 < map_cell_info_t::Gib_Cell_Size ()) && (position == koord::invalid) ; i1++)
	{
		for ( uint8 j = 0; (j < map_cell_info_t::Gib_Cell_Size ())  && (position == koord::invalid) ; j++){

			my_finder = new land_factory_place_finder_t (cell_pos + koord (j,i1), dimension.x, dimension.y, world, 0);


			//this function checks the suitability of the place
			//(it can find new one nearby durign this procedure)
			if (my_finder -> Check_Place())
			{
				//it needs to be done this way (retrieve pos from finder) since
				//finder is inteded to be able to change terrain and eventually offer nearby place
				position = my_finder ->Get_Place();

			}

			delete my_finder;

		}

	}


	//end if no siutable location was found
	if ( position == koord::invalid )
	{

		return FALSE;
	}



	//build the building                             //makes 3d coordinate from 2d one
	hausbauer_t::baue(world, world -> gib_spieler(1), (world -> lookup(position)) -> gib_kartenboden() -> gib_pos(), rotation, bldg);

	//it is for statistical purposes only
	//does not build anything
	cell -> Add_Special_to_Cell ();

	//all was successfull, finish
	return TRUE;

}

//this method places randomly special buildings. It is now very crude, but its structure is shaped so
//that it allows simple extension
void factory_builder_t::Build_Special_Buildings (const uint8 density, const uint8 city_count, cell_lists_t &cl)
{

	//determines total number of special buildings to be built
	const uint8 final_count = MAX ((city_count * density) / 2, 1);
	uint8 total_count = 0;

	//loop
	while (total_count < final_count)
	{
		//find special building type that will be placed now
		//it is now random, but it can be more sophisticated someday
		const haus_besch_t *current = hausbauer_t::waehle_sehenswuerdigkeit();

		//how many of the will be placed in this run?
		const uint8 current_count = 1;


		//find a location adn place all of them
		for (uint8 i = 0; i < current_count; i++)
		{

			//choose some cell list
			const cell_list_tpl *cell_list = cl.Get_Random_LH_Cell_List();

			for (uint8 j = 0; (j < MAX_TRIED_CELLS) && !(Place_Spc_Bdg_into_Cell ( cell_lists_t::Get_Random_Cell(cell_list), current)); j++);

		}

		total_count++;

	}


}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//this method can be called from outside and builds and connects all factories//////
////////////////////////////////////////////////////////////////////////////////////
void factory_builder_t::Build_Industries (const uint16 industry_density) throw (int)
{
	//there cannot be more than 32000 cells, so cell size is adjusted
	//to allow worlds of size 5700x5700 (not tested)
	const uint8 tmp_cell_size = ((world->gib_groesse())<=2700)?16:32;
	map_cell_info_t::Set_Cell_Size (tmp_cell_size);

	cell_lists_t map_cell_lists;
	factory_counts_table_tpl map_factory_counts;

	//collects all terrain cell data, computes values and sorts cells into lists
	printf("Scannig map terrain for industry building ...\n");
 	Fill_Cell_Info_Lists (&map_cell_lists, 40.0, 9.0);

	/*now we have enough information about terrain, so it is a point where we will
	compute number of spawned industries*/
	printf("Calculating industry counts ...\n");
	Calculate_Factory_Counts (map_factory_counts, world->gib_einstellungen()->gib_anzahl_staedte(),
	 industry_density, map_cell_lists.usable_land_cells_total, (map_cell_lists.usable_cells_total - map_cell_lists.usable_land_cells_total));

	//global variable TABLE holds beshes to all factory types
    stringhashtable_iterator_tpl<fabrik_besch_t *> factory_iterator (table);
	factory_iterator.begin();



	///////////////////////////////////////////////////////////////////////////////////
	////////////this is workaround used to fit more data than current paks hold
	////////////it is very poor in temrs of effectivity of coding.
	////////////it can be replaced immediately once data are stored in pak files
	///////////////////////////////////////////////////////////////////////////////////
	//used tmp structure:
	//tmp_loacation_looseness_table
	//tmp_resource_bound_table
	//all those will be unnecessary once data can be retrieved from paks

	while (factory_iterator.next())
	{

		//enum platzierung {Land, Wasser, Stadt, Low, High, Shore};
		//uint8 ResourceBound;

		if ( strcmp (factory_iterator.get_current_key(),"Nutzwald") == 0)
		{
			//str cmp returns 0 if strings match
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::High;
			tmp_resource_bound_table.put(factory_iterator.get_current_key(),50);
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),20);

			//continue means return to beginig of cycle
			continue;
		}

		if ( strcmp (factory_iterator.get_current_key() , "Saegewerk") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::High;
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),30);
			continue;
		}

		if ( strcmp (factory_iterator.get_current_key() , "Papierfabrik") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::High;
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),40);
			continue;
		}

		if ( strcmp (factory_iterator.get_current_key() , "Mobelfabrik") == 0){
			//default location for it is city
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),50);
			continue;
		}

		if ( strcmp (factory_iterator.get_current_key() , "Oelfeld") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::Low;
			tmp_resource_bound_table.put(factory_iterator.get_current_key(),75);
			continue;
		}

		if ( strcmp (factory_iterator.get_current_key() , "Oelbohrinsel") == 0){
			tmp_resource_bound_table.put(factory_iterator.get_current_key(),50);
			continue;
		}


		if ( (strcmp( factory_iterator.get_current_key() , "Raffinerie") == 0) )
		{
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::Shore;
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),30);
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "goods_factory") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::Low;
			continue;
		}


		if ( strcmp ( factory_iterator.get_current_key() , "Sandquarry") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::Low;
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),20);
			tmp_resource_bound_table.put(factory_iterator.get_current_key(),40);
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "Stonequarry") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::High;
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),20);
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "grain_farm") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::Low;
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),30);
			tmp_resource_bound_table.put(factory_iterator.get_current_key(),50);
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "fish_pond") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::Low;
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),50);
			continue;
		}

		if ( strcmp (factory_iterator.get_current_key() , "fish_swarm") == 0){
			tmp_resource_bound_table.put(factory_iterator.get_current_key(),50);
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "cow_farm") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::High;
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),30);
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "chicken_farm") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::High;
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),40);
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "brewery") == 0){
			//default location for it is city
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),50);
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "dairy") == 0){
			//default location for it is city
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),70);
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "grain_mill") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::Low;
			continue;
		}


		if ( strcmp ( factory_iterator.get_current_key() , "bakery") == 0){
			//default location for it is city
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),50);
			continue;
		}

			if ( strcmp ( factory_iterator.get_current_key() , "food_processing_plant") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::Low;
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "Kohlegrube") == 0){
			tmp_resource_bound_table.put(factory_iterator.get_current_key(),100);
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "Stahlwerk") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::Stadt;
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),60);
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "textile_factory") == 0){
			//default location for it is city
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),70);
			continue;
		}


		if ( strcmp ( factory_iterator.get_current_key() , "sheep_farm") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::High;
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "cotton_field") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::Low;
			continue;
		}

		if ( strcmp ( factory_iterator.get_current_key() , "Glass_factory") == 0){
			((factory_iterator.access_current_value()) -> platzierung) = fabrik_besch_t::High;
			tmp_loacation_looseness_table.put(factory_iterator.get_current_key(),30);
			continue;
		}


	}


	//couts number of all factories that will be created
 	factory_counts_table_iterator_tpl tmp_iter (map_factory_counts);
	uint16 total_no_spawned_factories = 0;

	//this is done through summing all counts of various types of factories
	//scan the factory count table and sum it.
	tmp_iter.begin();
	while(tmp_iter.next())
	{
		total_no_spawned_factories += tmp_iter.get_current_value();
	}


	printf("Placing industries ...\n");
	Place_Resource_Bound_Factories (map_factory_counts, map_cell_lists);

	//remaining factories will get placed randomly on the map accordin to their terrain prefferences (this function will not modify the count table)
	Randomly_Place_Factories (map_factory_counts, map_cell_lists);


	/*next lines calculate average distance between factories
	  all computation can fit three lines, but I think that splitting it
	  will make it more clear and bug proof.
	  This is one time only operation, so there is no push for speed.
	*/
	//calculate how many world squares are in average per one factory and get square root to find average distance between them
	const uint32 avg_dist_btw_factories = sqrt( ( (world->gib_groesse()) * (world->gib_groesse()) ) / total_no_spawned_factories);

	//debug output
    printf("Average. dist. btw. factories: %i; no. of factories: %i\n",avg_dist_btw_factories, total_no_spawned_factories);

	//now interconnects all industries
	printf("Connecting industries ...\n");
	Make_Connections (3*avg_dist_btw_factories, world);


	printf("Building special buildings ...\n");
	Build_Special_Buildings (1, world->gib_einstellungen()->gib_anzahl_staedte(), map_cell_lists);


}


/*
list from fabrik bauer

    const int display_offset = groesse/2 + staedte*2;
    const int display_part = groesse/2 + staedte*2;
    const int display_total = groesse+staedte*4;



  	    if(is_display_init()) {
		display_progress(display_offset + display_part * (i * reihen + j) / reihen / reihen,
		    display_total);

		display_flush(0, 0, 0, "", "");
	    }

  */



  ////////////////////////
  //end of file///////////
  ////////////////////////
