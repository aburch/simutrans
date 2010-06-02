// Header file for vehicle replacement class
// @author: jamespetts, March 2010
// Distributed under the terms of the Artistic Licence.

#ifndef replace_data_h
#define replace_data_h

#include "../simconvoi.h"
#include "../tpl/vector_tpl.h"

class vehikel_t;
class vehikel_besch_t;
class cbuffer_t;

class replace_data_t
{
private:
	/**
	* The replacing vehicles, if any
	*/
	vector_tpl<const vehikel_besch_t *> *replacing_vehicles;
	
	/**
	* The convoys currently being replaced
	*/
	vector_tpl<convoihandle_t> *replacing_convoys;

	/**
	 * if marked for replacing, once in depot, auto restart the vehicle
	 * @author isidoro
	 */
	bool autostart;

	/**
	 * If this is true, vehicles will be retained in the depot 
	 * when replaced (if they are not used in the new convoy);
	 * otherwise, they are either upgraded (if possible), or 
	 * otherwise sold.
	 * @author: jamespetts, March 2010
	 */
	bool retain_in_depot;

	/**
	 * If this is true, the convoy will go, if it can, to its
	 * home depot for the replacement, even if that is not the
	 * closest depot.
	 * @author: jamespetts, March 2010
	 */
	bool use_home_depot;

	/**
	 * If this is true, when the convoy is replaced, vehicles
	 * already in the depot will be used where available in 
	 * preference to buying new or upgrading.
	 * @author: jamespetts, March 2010
	 */
	bool allow_using_existing_vehicles;

	/* The number of (identical) convoys that use this set of
	 * replace data
	 * @author: jamespetts, March 2010
	 */
	sint16 number_of_convoys;

	bool clearing;

public:
	sint16 get_number_of_convoys() const { return number_of_convoys; }
	
	bool get_autostart() const { return autostart; }

	void set_autostart(bool new_autostart) { autostart=new_autostart; }

	bool get_retain_in_depot() const { return retain_in_depot; }

	void set_retain_in_depot(bool value) { retain_in_depot = value; }

	bool get_use_home_depot() const { return use_home_depot; }

	void set_use_home_depot(bool value) { use_home_depot = value; }

	bool get_allow_using_existing_vehicles() const { return allow_using_existing_vehicles; }
	
	void set_allow_using_existing_vehicles(bool value) { allow_using_existing_vehicles = value; }

	const vector_tpl<const vehikel_besch_t *>* get_replacing_vehicles() const { return replacing_vehicles; }
	const vehikel_besch_t* get_replacing_vehicle(uint16 number) const;
	void set_replacing_vehicles(vector_tpl<const vehikel_besch_t *> *rv) { replacing_vehicles = rv; }
	void add_vehicle(const vehikel_besch_t* vehicle, bool add_at_front = false);

	void increment_convoys(convoihandle_t cnv);
	void decrement_convoys(convoihandle_t cnv);

	/**
	 * fills the given buffer with replace data
	 */
	void sprintf_replace( cbuffer_t &buf ) const;

	/**
	 * converts this string into replace data
	 */
	bool sscanf_replace( const char * );

	void rdwr(loadsave_t *file);

	/**
	 * Will clear the replace data of all convoys currently
	 * being replaced with this dataset. 
	 * WARNING: This is equivalent to *deleting* this object.
	 */
	void clear_all();

	replace_data_t* copy() { replace_data_t *r = new replace_data_t(this); return r; }

	replace_data_t();
	replace_data_t(loadsave_t *file);
	replace_data_t(replace_data_t *copy_from);

	~replace_data_t();
};

#endif