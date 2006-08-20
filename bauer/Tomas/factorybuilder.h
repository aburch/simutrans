/*
 * factorybuilder.h
 *
 * Copyright (c) 1997 - 2003 Tomas Kubes
 *                           Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
   Factory Builder
   ---------------

  This header holds all necessary class definitions that are
  necessary for industry creation.

  For dependencies see: factorybuilder.cc

*/



/*
  Exception table:
  ----------------

  Integer exception - those means that something bad had happend, they should only be thrown
  if some function woud have been called inproperly by user, standard execution should not
  throw them. They are ment only to be caught for output error msg. and prgram termination.
  0 = Cell size not set or 0
  1 = World size not set or 0
  2 = Cell size too small



  100 = was not able to find supplier for some type of goods that is required!
  101 = could not find any consumers


*/
#ifndef factorybuilder_t_h
#define factorybuilder_t_h

#include "../tpl/vector_tpl.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/array_tpl.h"
#include "../dataobj/koord3d.h"
#include "../simtypes.h"


//this is here for definition of enum platzierung
#include "../besch/fabrik_besch.h"


//temporary definitions
class karte_t;
class spieler_t;
class fabrik_t;


//constant definitions
//might br overridden by user configutrable data one day
#define NUMBER_OF_RANDOM_TRIES_IN_CELL 5




class factory_builder_t {

	factory_builder_t (); 	 //disabling call of default constructor - private
	factory_builder_t (const factory_builder_t&);	//disables call of copy constructor - private
	factory_builder_t (int); //disables call of conversion constructor - private
	class map_cell_info_t;   //pre-declaration

	//types declaration
	enum   recognized_cell_types {SEA, SHORE, LOW, HIGH, CITY, UNUSABLE};

	/*to maintain consistency around the code,
	I decided that tpyes used for one logical purposed will be uniquely defined*/
	typedef uint16 factory_count_tpl;
	typedef stringhashtable_tpl<factory_count_tpl> factory_counts_table_tpl;
	typedef stringhashtable_iterator_tpl<factory_count_tpl> factory_counts_table_iterator_tpl;

	typedef  slist_tpl<map_cell_info_t*> cell_list_tpl;
	typedef  slist_iterator_tpl<map_cell_info_t*> cell_list_iterator_tpl;

	spieler_t *player; //standard industry owner
	karte_t *world;    //pointer to map
	uint16 world_size; //this variable will safe some function calls

	//those variables store general information about map (filled and used when filling and typing cells)
	uint32 sum_of_world_heights;
	float  average_world_height;


	//this table will hold beshes to all factories
    static stringhashtable_tpl</*const*/ fabrik_besch_t *> table;



	factory_counts_table_tpl tmp_loacation_looseness_table, tmp_resource_bound_table;


	//this is encapsulating structure for lists holding informations about found cells
	//in fact lists can be directly here, but encapsulating them makes handling convinient and more readable
	struct cell_lists_t
	{
		//all of this should be public:

		//those variables simplyfy determintion of genral map information
		uint16 usable_cells_total;
		uint16 usable_land_cells_total;

		//the pointer is best representation, since lists will be passed through many functions
		cell_list_tpl *sea;
		cell_list_tpl *shore;
		cell_list_tpl *low;
		cell_list_tpl *high;
		cell_list_tpl *city;

		//constructors sets the lists and destructor destroys them DELETING all cells in lists!!!
		cell_lists_t ();
		~cell_lists_t ();

		//here are few functions to simplify operations with lists (too make code more readable)
		static map_cell_info_t* Get_Random_Cell (const cell_list_tpl *my_list);
		cell_list_tpl* Get_Random_Land_Cell_List (void) const;
		cell_list_tpl* Get_Random_LH_Cell_List (void) const;


		//this function will return pointer to a cell that is closer than dist to a pos and
		//will be selected as nth when searching the lsit from begining
		//(this is usefull for beacons)
		static map_cell_info_t* Gib_nth_Closest_Cell (const cell_list_tpl *my_list, const koord pos, const uint16 dist, const uint16 n);

	   private:
		//those supplementary variables are here to optimize some cell searching methods
		//of function Gib_nth_Closest_Cell
		static map_cell_info_t *s_last_cell;
		static uint8			s_f_tries;


	   public:
		inline static void Init_s_Values (void)
		{
			//this sets supplementary static variables of map_cell_info_t
			s_last_cell = NULL;
			s_f_tries = 1;
		}



	};

	//actually i have no idea why it should be friend, but MSVC refused to compile Get_Cell functions otherwise
	friend cell_lists_t;


	/*this class will hold data and functions to retrieve and work with data about
	terrain in one map cell (size given by static variable cell_size)*/
	class map_cell_info_t
	{

		//this varibable will hold info about size of map cell used
		static uint8 cell_size;

		//this will relate cell to its builder
		factory_builder_t *builder;

		sint16 sum_of_square_heights;
		float  cell_height;
		koord  cell_position;
	//	recognized_cell_types cell_type;

		uint8 no_of_factories_in_cell;
		bool  has_special;

		/*next structures will hold data about counted number of squares
		//might as well be separate variables, but grouping them is logically correct
		//structure will be accessed as whole by functions, variables will bereferenced by */
		struct	map_cell_land_counts
		{
			uint16	land, water, city, slope;
		};

		struct	map_cell_land_percents
		{
			float	land, water, city, slope;
		};


		//those will store info about found types of squares in cell.
		map_cell_land_counts counts;
		map_cell_land_percents percents;


		//calculates percent values from counts
		void Calculate_Cell_Percents (void);



	   public:

		map_cell_info_t (factory_builder_t *tmp);


		inline static uint8 Gib_Cell_Size (void)
		{
			return cell_size;
		}

		inline static void Set_Cell_Size (const uint8 a) throw (int)
		{
			if ((a != 8) && (a != 16) && (a != 32) && (a != 64)) throw 0;
			cell_size = a;
		}

		inline const map_cell_land_counts& Gib_Cell_Counts (void) const
		{
			return counts;
		}

		inline map_cell_land_counts& Gib_Writable_Cell_Counts (void)
		{
			return counts;
		}

		inline koord Gib_Cell_Position (void) const
		{
			return cell_position;
		}

		inline uint8 Gib_No_of_Factories_in_Cell (void) const
		{
			return no_of_factories_in_cell;
		}

		//this is only to keep statistics, that prvents overcrowding of single cells
		inline Add_Factory_to_Cell (void)
		{
			no_of_factories_in_cell++;
		}

		inline uint8 Has_Special (void) const
		{
			return has_special;
		}

		//this is only to keep statistics, that prvents overcrowding of single cells
		inline Add_Special_to_Cell (void)
		{
			has_special = true;
		}


		//collects data about types of terrain in cell and sets variables
		void Collect_Terrain_Data (const koord pos, const karte_t *world) throw (int);

		//determines type of cell in terms of most common terrain
		recognized_cell_types Determine_Cell_Type (const float &min_slope_percent, const float &min_city_percent);


	};

	//map_cell info is friend so that it can call next functions (well, thos surely need not to be public)
	friend map_cell_info_t;

/////some basic functions for variable menagemetn//////////////////////////////////
  private:

	inline void Set_Sum_of_World_Heights (const uint32 a)
	{
		sum_of_world_heights = a;
	}

	inline uint32 Gib_Sum_of_World_Heights (void) const
	{
		return sum_of_world_heights;
	}

	inline void Calculate_Average_World_Height (void) throw (int)
	{
		if (world_size == 0) throw 1;
		average_world_height = (sum_of_world_heights / ((float)(world_size*world_size)));
	}
	inline float Gib_Average_World_Height (void) const
	{
		return average_world_height;
	}


//////advanced functions for information collecting//////////////////////////////
	void Fill_Cell_Info_Lists (cell_lists_t *cl, const float min_slope_percent, const float min_city_percent);

	uint32 Compute_Total_Demand (const ware_besch_t *product, const factory_counts_table_tpl &factory_counts) const;

	uint32 Compute_Total_Supply (const ware_besch_t *product, const factory_counts_table_tpl &factory_counts) const;


	void Calculate_Factory_Counts (factory_counts_table_tpl &factory_counts,
	 const uint8 no_of_cities, const uint16 industry_density, const uint16 land_cell_count,
	  const uint16 sea_cell_count) throw (int);

	bool Place_Factory_Into_Cell (map_cell_info_t *cell, const fabrik_besch_t *factory, const uint8 rotation, const uint8 priority);

	void Build_Stop_Around_Water_Factory (const fabrik_besch_t *info, const uint8 rotate, koord3d pos);

	void Construct_Industry_Special (fabrik_t *fab);
	//this function will construc specials like, powerlines to powerplants.
	//no parameters necessary, works with information from karte_t - pointed by variable world (member of factory builder)
	//is called by Build_Industry method


	void Place_Resource_Bound_Factories (factory_counts_table_tpl &factory_counts, cell_lists_t &cl);

	//randomly place all factories in factory counts
	void Randomly_Place_Factories (const factory_counts_table_tpl &factory_counts, cell_lists_t &cl);

	//connects industry to closest supplier - no matter how far it is
	//if it does not exist or facotry does not require any does nothing
	//this method has been updated to link 2 closest suppliers (if they exist)
	//this makes connections bit better
	void Connect_Industry_to_Closest_Supplier (const fabrik_t *worked_factory, const karte_t *world);

	//connect industry to closest consumer - no matter how far it is
	//if it does not exist or facotry does not require any does nothing
	void Connect_Industry_to_Closest_Consumer (fabrik_t *worked_factory, const karte_t *world);

	//connect factory to all consumers in distance lower than min_dist, and to some
	//random consumers above that distance (chance of connecting decreases with distance)
	void Connect_Industry_to_Random_Neighbour_Consumers (const uint32 min_dist, fabrik_t *worked_factory, const karte_t *world);

	//calls 3 connecting methods above for all factories registered in the world
	void Make_Connections (const uint16 min_dist, const karte_t *world );

	//places selected special building into a given cell
	bool Place_Spc_Bdg_into_Cell (map_cell_info_t *cell, const haus_besch_t *bldg);

	//places all special buildings
	void Build_Special_Buildings (const uint8 density, const uint8 city_count, cell_lists_t &cl);

/////////////////////////////////////////////////////////////////////////////////
   public:

	//some necessarly data management functions

	//loads factory besh into table
	static void register_besch (fabrik_besch_t *besch);

	 //returns factory besh based on its object name
	static /*const*/ fabrik_besch_t * gib_fabesch (const char *fabtype);


	//only usable constructor. This one should be called eachtime before calling build industries
    factory_builder_t (karte_t *world1, spieler_t *p);


	//calls methods above for all factories
	void Connect_Given_Industry ( fabrik_t *worked_factory, const karte_t *world );

	//builds factory at given coordinates. No crosslinking is done
	fabrik_t* Build_Industry(const fabrik_besch_t *info, const uint8 rotate, koord3d pos, spieler_t *sp);

	/*builds industries on map. Completely (except for city passanger linking)
	throwing of int means severe problem - catch it to write error and exit program
	    -> (see error descriptor at the top) continuing impossible*/
	void Build_Industries (const uint16 industry_density) throw (int);


};

#endif
