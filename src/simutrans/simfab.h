/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMFAB_H
#define SIMFAB_H


#include "dataobj/koord3d.h"
#include "dataobj/translator.h"

#include "tpl/slist_tpl.h"
#include "tpl/vector_tpl.h"
#include "tpl/array_tpl.h"
#include "descriptor/factory_desc.h"
#include "halthandle.h"
#include "world/simworld.h"
#include "utils/plainstring.h"


class player_t;
class stadt_t;
class ware_t;
class leitung_t;


/**
 * Factory statistics
 */
#define MAX_MONTH                  (12)
#define FAB_PRODUCTION              (0)
#define FAB_POWER                   (1)
#define FAB_BOOST_ELECTRIC          (2)
#define FAB_BOOST_PAX               (3)
#define FAB_BOOST_MAIL              (4)
#define FAB_PAX_GENERATED           (5)
#define FAB_PAX_DEPARTED            (6)
#define FAB_PAX_ARRIVED             (7)
#define FAB_MAIL_GENERATED          (8)
#define FAB_MAIL_DEPARTED           (9)
#define FAB_MAIL_ARRIVED           (10)
#define MAX_FAB_STAT               (11)

// reference lines
#define FAB_REF_MAX_BOOST_ELECTRIC  (0)
#define FAB_REF_MAX_BOOST_PAX       (1)
#define FAB_REF_MAX_BOOST_MAIL      (2)
#define FAB_REF_DEMAND_ELECTRIC     (3)
#define FAB_REF_DEMAND_PAX          (4)
#define FAB_REF_DEMAND_MAIL         (5)
#define MAX_FAB_REF_LINE            (6)

// statistics for goods
#define MAX_FAB_GOODS_STAT          (4)
// common to both input and output goods
#define FAB_GOODS_STORAGE           (0)
// input goods
#define FAB_GOODS_RECEIVED          (1)
#define FAB_GOODS_CONSUMED          (2)
#define FAB_GOODS_TRANSIT           (3)
// output goods
#define FAB_GOODS_DELIVERED         (1)
#define FAB_GOODS_PRODUCED          (2)


// production happens in every second
#define PRODUCTION_DELTA_T_BITS (10)
#define PRODUCTION_DELTA_T (1 << PRODUCTION_DELTA_T_BITS)
// default production factor
#define DEFAULT_PRODUCTION_FACTOR_BITS (8)
#define DEFAULT_PRODUCTION_FACTOR (1 << DEFAULT_PRODUCTION_FACTOR_BITS)
// precision of apportioned demand (i.e. weights of factories at target cities)
#define DEMAND_BITS (4)

/**
 * JIT2 output scale constants.
 */
// The fixed point precision for work done fraction.
static const uint32 WORK_BITS = 16;
// The minimum allowed production rate for a factory. This is to limit the time outputs take to fill completly (so factories idle sooner).
// Fixed point form range must be between 0.0 and 1.0.
static const sint32 WORK_SCALE_MINIMUM_FRACTION = (5 << WORK_BITS) / 100; // ~5%, 1/20 of the full production rate.
// The number of times minimum_shipment must be in current storage before rampdown starts.
// Must be at least 2 to allow for full production as shipment is not instant.
static const sint32 OUTPUT_SCALE_RAMPDOWN_MULTIPLYER = 2; // Two shipments must be ready.
// The maximum rate at which power boost change change per second.
// This limit is required to help stiffen overloaded networks to combat oscilations caused by feedback.
static const sint32 BOOST_POWER_CHANGE_RATE = (5 << DEFAULT_PRODUCTION_FACTOR_BITS) / 100; // ~5%


/**
 * Shipment size constants.
 */
// The maximum shipment size in whole units.
// Must be greater than 0.
static const uint32 SHIPMENT_MAX_SIZE = 10; // Traditional value.
// The minimum number of whole shipments a facotry can store.
// Must be greater than 0 to prevent division by 0.
static const uint32 SHIPMENT_NUM_MIN = 4; // Quarters should allow reasonably fair distribution.


/**
 * Convert internal values to displayed values
 */
sint64 convert_goods(sint64 value);
sint64 convert_power(sint64 value);
sint64 convert_boost(sint64 value);

class ware_production_t
{
private:
	const goods_desc_t *type;
	// statistics for each goods
	sint64 statistics[MAX_MONTH][MAX_FAB_GOODS_STAT];
	sint64 weighted_sum_storage;

	/// clears statistics, transit, and weighted_sum_storage
	void init_stats();
public:
	ware_production_t() : type(NULL), menge(0), max(0)/*, transit(statistics[0][FAB_GOODS_TRANSIT])*/,
	                      max_transit(0), placing_orders(false), index_offset(0)
	{
#ifdef TRANSIT_DISTANCE
		count_suppliers = 0;
#endif
		init_stats();
	}

	const goods_desc_t* get_typ() const { return type; }
	void set_typ(const goods_desc_t *t) { type=t; }

	// functions for manipulating goods statistics
	void roll_stats(uint32 factor, sint64 aggregate_weight);
	void rdwr(loadsave_t *file);
	const sint64* get_stats() const { return *statistics; }
	void book_stat(sint64 value, int stat_type) { assert(stat_type<MAX_FAB_GOODS_STAT); statistics[0][stat_type] += value; }
	void set_stat(sint64 value, int stat_type) { assert(stat_type<MAX_FAB_GOODS_STAT); statistics[0][stat_type] = value; }
	sint64 get_stat(int month, int stat_type) const { assert(stat_type<MAX_FAB_GOODS_STAT); return statistics[month][stat_type]; }

	/**
	 * convert internal units to displayed values
	 */
	sint64 get_stat_converted(int month, int stat_type) const {
		assert(stat_type<MAX_FAB_GOODS_STAT);
		sint64 value = statistics[month][stat_type];
		if (stat_type==FAB_GOODS_STORAGE  ||  stat_type==FAB_GOODS_CONSUMED) {
			value = convert_goods(value);
		}
		return value;
	}
	void book_weighted_sum_storage(uint32 factor, sint64 delta_time);

	sint32 menge; // in internal units shifted by precision_bits (see step)
	sint32 max;
	/// Cargo currently in transit from/to this slot. Equivalent to statistics[0][FAB_GOODS_TRANSIT].
	sint32 get_in_transit() const { return (sint32)statistics[0][FAB_GOODS_TRANSIT]; }

	/// Annonmyous union used to save memory and readability. Contains supply flow control limiters.
	union{
		// Classic : Current limit on cargo in transit (maximum network capacity), depending on sum of all supplier output storage.
		sint32 max_transit; //JIT<2 Input

		// JIT Version 2 : Current demand for the good. Orders when greater than 0.
		sint32 demand_buffer; //JIT2 Input

		// The minimum shipment size. Used to control delivery to stops and for production ramp-down.
		sint32 min_shipment; // Output
	};

	// Ordering lasts at least 1 tick period to allow all suppliers time to send (fair). Used by inputs.
	bool placing_orders;

#ifdef TRANSIT_DISTANCE
	sint32 count_suppliers; // only needed for averaging
#endif
	uint32 index_offset; // used for haltlist and lieferziele searches in verteile_waren to produce round robin results

	// Production rate for outputs. Returns fixed point with WORK_BITS fractional bits.
	sint32 calculate_output_production_rate() const;

	// Production rate for JIT2 demands. Returns fixed point with WORK_BITS fractional bits.
	sint32 calculate_demand_production_rate() const;
};


/**
 * Factories produce and consume goods (ware_t) and supply nearby halts.
 * @see haltestelle_t
 */
class fabrik_t
{
public:
	/**
	 * Constants
	 */
	enum {
		old_precision_bits = 10,
		precision_bits     = 10,
		precision_mask     = (1 << precision_bits) - 1
	};

private:
	/**
	 * Factory statistics
	 */
	sint64 statistics[MAX_MONTH][MAX_FAB_STAT];
	sint64 weighted_sum_production;
	sint64 weighted_sum_boost_electric;
	sint64 weighted_sum_boost_pax;
	sint64 weighted_sum_boost_mail;
	sint64 weighted_sum_power;
	sint64 aggregate_weight;

	// Control logic type determines how a factory behaves with regards to inputs and outputs.
	enum CL_TYPE {
		CL_NONE,         // This factory does nothing! (might be useful for scenarios)
		// Producers are at the bottom of every supply chain.
		CL_PROD_CLASSIC, // Classic producer logic.
		CL_PROD_MANY,    // Producer of many outputs.
		// Factories are in the middle of every supply chain.
		CL_FACT_CLASSIC, // Classic factory logic, consume at maximum output rate or minimum input.
		CL_FACT_MANY,    // Enhanced factory logic, consume at average of output rate or minimum input averaged.
		// Consumers are at the top of every supply chain.
		CL_CONS_CLASSIC, // Classic consumer logic. Can generate power.
		CL_CONS_MANY,    // Consumer that consumes multiple inputs, possibly produces power.
		// Electricity producers provide power.
		CL_ELEC_PROD,    // Simple electricity source. (green energy)
		CL_ELEC_CLASSIC  // Classic electricity producer behaviour with no inputs.
	} control_type;

	// Demand buffer order logic;
	enum DL_TYPE {
		DL_NONE,    // Has no inputs to demand.
		DL_SYNC,    // All inputs ordered together.
		DL_ASYNC,   // All inputs ordered separatly.
		DL_OLD      // Use maximum in-transit and storage to determine demand.
	} demand_type;

	// Boost logic determines what factors boost factory production.
	enum BL_TYPE {
		BL_NONE,    // Production cannot be boosted.
		BL_PAXM,    // Production boosted only using passengers/mail.
		BL_POWER,   // Production boosted with power as well. Needs aditional logic for correct ordering.
		BL_CLASSIC  // Production boosted in classic way.
	} boost_type;

	// Functions for manipulating factory statistics
	void init_stats();
	void set_stat(sint64 value, int stat_type) { assert(stat_type<MAX_FAB_STAT); statistics[0][stat_type] = value; }

	// For accumulating weighted sums for average statistics
	void book_weighted_sums(sint64 delta_time);

	/// Possible destinations for produced goods
	vector_tpl <koord> lieferziele;
	uint32 lieferziele_active_last_month;

	/**
	 * suppliers to this factory
	 */
	vector_tpl <koord> suppliers;

	/**
	 * fields of this factory (only for farms etc.)
	 */
	struct field_data_t
	{
		koord location;
		uint16 field_class_index;

		field_data_t() : field_class_index(0) {}
		explicit field_data_t(const koord loc) : location(loc), field_class_index(0) {}
		field_data_t(const koord loc, const uint16 class_index) : location(loc), field_class_index(class_index) {}

		bool operator==(const field_data_t &other) const { return location==other.location; }
	};
	vector_tpl <field_data_t> fields;

	/**
	 * Distribute products to connected stops
	 */
	void verteile_waren(const uint32 product);

	// List of target cities
	vector_tpl<stadt_t *> target_cities;

	player_t *owner;
	static karte_ptr_t welt;

	const factory_desc_t *desc;

	/**
	 * Is construction site rotated?
	 */
	uint8 rotate;

	/**
	 * production base amount
	 */
	sint32 prodbase;

	/**
	 * multipliers for the production base amount
	 */
	sint32 prodfactor_electric;
	sint32 prodfactor_pax;
	sint32 prodfactor_mail;

	array_tpl<ware_production_t> input;  ///< array for input/consumed goods
	array_tpl<ware_production_t> output; ///< array for output/produced goods

	/// Accumulated time since last production
	uint32 delta_sum;
	uint32 delta_menge;

	// production remainder when scaled to PRODUCTION_DELTA_T. added back next step to eliminate cumulative error
	uint32 menge_remainder;

	// number of rounds where there is active production or consumption
	uint8 activity_count;

	// The adjacent connected transformer, if any.
	vector_tpl<leitung_t *>transformers;

	// true, if the factory did produce enough in the last step to require power
	bool currently_requiring_power;

	// there is input or output and we do something with it ...
	bool currently_producing;

	uint32 last_sound_ms;

	uint32 total_input, total_transit, total_output;
	uint8 status;

	/**
	 * Inactive caches, used to speed up logic when dealing with inputs and outputs.
	 */
	uint8 inactive_outputs;
	uint8 inactive_inputs;
	uint8 inactive_demands;

	/// Position of a building of the factory.
	koord3d pos;

	/// Position of the nw-corner tile of the factory.
	koord3d pos_origin;

	/**
	 * Number of times the factory has expanded so far
	 * Only for factories without fields
	 */
	uint16 times_expanded;

	/**
	 * Electricity amount scaled with prodbase
	 */
	uint32 scaled_electric_demand;

	/**
	 * Pax/mail demand scaled with prodbase and month length
	 */
	uint32 scaled_pax_demand;
	uint32 scaled_mail_demand;

	/**
	 * Update scaled electricity amount
	 */
	void update_scaled_electric_demand();

	/**
	 * Update scaled pax/mail demand
	 */
	void update_scaled_pax_demand();
	void update_scaled_mail_demand();

	/**
	 * Update production multipliers for pax and mail
	 */
	void update_prodfactor_pax();
	void update_prodfactor_mail();

	/**
	 * Re-calculate the pax/mail demands of factory at target cities
	 */
	void recalc_demands_at_target_cities();

	/**
	 * Recalculate storage capacities based on prodbase or capacities contributed by fields
	 */
	void recalc_storage_capacities();

	/**
	 * Class for collecting arrival data and calculating pax/mail boost with fixed period length
	 */
	#define PERIOD_BITS   (18)              // determines period length on which boost calculation is based
	#define SLOT_BITS     (6)               // determines the number of time slots available
	#define SLOT_COUNT    (1<<SLOT_BITS)    // number of time slots for accumulating arrived pax/mail

	class arrival_statistics_t
	{
	private:
		uint16 slots[SLOT_COUNT];
		uint16 current_slot;
		uint16 active_slots;      // number of slots covered since aggregate arrival last increased from 0 to +ve
		uint32 aggregate_arrival;
		uint32 scaled_demand;
	public:
		void init();
		void rdwr(loadsave_t *file);
		void set_scaled_demand(const uint32 demand) { scaled_demand = demand; }
		#define ARRIVALS_CHANGED (1)
		#define ACTIVE_SLOTS_INCREASED (2)
		sint32 advance_slot();
		void book_arrival(const uint16 amount);
		uint16 get_active_slots() const { return active_slots; }
		uint32 get_aggregate_arrival() const { return aggregate_arrival; }
		uint32 get_scaled_demand() const { return scaled_demand; }
	};

	void update_transit_intern( const ware_t *ware, bool add );

	/**
	 * Arrival data for calculating pax/mail boost
	 */
	arrival_statistics_t arrival_stats_pax;
	arrival_statistics_t arrival_stats_mail;

	plainstring name;

	/**
	 * For advancement of slots for boost calculation
	 */
	sint32 delta_slot;

	void recalc_factory_status();

	// create some smoke on the map
	void smoke();

	// scales the amount of production based on the amount already in storage
	uint32 scale_output_production(const uint32 product, uint32 menge) const;

	/**
	 * Convenience method that deals with casting.
	 */
	void set_power_supply(uint32 supply);

	/**
	 * Convenience method that deals with casting.
	 */
	uint32 get_power_supply() const;

	/**
	 * Convenience method that deals with casting.
	 */
	sint32 get_power_consumption() const;

	/**
	 * Convenience method that deals with casting.
	 */
	void set_power_demand(uint32 demand);

	/**
	 * Convenience method that deals with casting.
	 */
	uint32 get_power_demand() const;

	/**
	 * Convenience method that deals with casting.
	 */
	sint32 get_power_satisfaction() const;

	/**
	 *
	 */
	 sint64 get_power() const;

public:
	fabrik_t(loadsave_t *file);
	fabrik_t(koord3d pos, player_t* owner, const factory_desc_t* factory_desc, sint32 initial_prod_base);
	~fabrik_t();

	/**
	 * Return/book statistics
	 */
	const sint64* get_stats() const { return *statistics; }
	sint64 get_stat(int month, int stat_type) const { assert(stat_type<MAX_FAB_STAT); return statistics[month][stat_type]; }
	void book_stat(sint64 value, int stat_type) { assert(stat_type<MAX_FAB_STAT); statistics[0][stat_type] += value; }

	// This updates maximum in-transit. Important for loading.
	static void update_transit( const ware_t *ware, bool add );

	// This updates transit stats for freshly produced goods. Includes logic to decrement demand counters.
	static void apply_transit( const ware_t *ware );

	/**
	 * convert internal units to displayed values
	 */
	sint64 get_stat_converted(int month, int stat_type) const {
		assert(stat_type<MAX_FAB_STAT);
		sint64 value = statistics[month][stat_type];
		switch(stat_type) {
			case FAB_POWER:
				value = convert_power(value);
				break;
			case FAB_BOOST_ELECTRIC:
			case FAB_BOOST_PAX:
			case FAB_BOOST_MAIL:
				value = convert_boost(value);
				break;
			default: ;
		}
		return value;
	}

	static fabrik_t * get_fab(const koord &pos);

	/**
	 * @return vehicle description object
	 */
	const factory_desc_t *get_desc() const {return desc; }

	void finish_rd();

	/// gets position of a building belonging to factory
	koord3d get_pos() const { return pos; }

	void rotate90( const sint16 y_size );

	void link_halt(halthandle_t halt);
	void unlink_halt(halthandle_t halt);

	bool is_within_players_network( const player_t* player ) const;

	const vector_tpl<koord>& get_lieferziele() const { return lieferziele; }
	bool is_active_lieferziel( koord k ) const;

	const vector_tpl<koord>& get_suppliers() const { return suppliers; }

	/**
	 * Functions for manipulating the list of connected cities
	 */
	void add_target_city(stadt_t *const city);
	void remove_target_city(stadt_t *const city);
	void clear_target_cities();
	const vector_tpl<stadt_t *>& get_target_cities() const { return target_cities; }

	void  add_lieferziel(koord ziel);
	void  rem_lieferziel(koord pos);

	/**
	 * adds a supplier
	 */
	void  add_supplier(koord pos);
	void  rem_supplier(koord pos);

	/**
	 * @return menge der ware typ
	 *   -1 wenn typ nicht produziert wird
	 *   sonst die gelagerte menge
	 */
	sint32 input_vorrat_an(const goods_desc_t *ware);        // Vorrat von Warentyp
	sint32 vorrat_an(const goods_desc_t *ware);        // Vorrat von Warentyp

	// true, if there was production requiring power in the last step
	bool is_currently_producing() const { return currently_requiring_power; }

	/**
	 * True if a transformer is connected to this factory.
	 */
	bool is_transformer_connected() const { return !transformers.empty(); }

	vector_tpl<leitung_t*> const &get_transformers() { return transformers; }

	/**
	 * Connect transformer to this factory.
	 */
	void add_transformer_connected(leitung_t *transformer) { transformers.append_unique(transformer); }
	void remove_transformer_connected( leitung_t* transformer ) { transformers.remove( transformer ); }

	/**
	 * @return 1 wenn consumption,
	 * 0 wenn Produktionsstopp,
	 * -1 wenn Ware nicht verarbeitet wird
	 */
	sint8 is_needed(const goods_desc_t *) const;

	sint32 liefere_an(const goods_desc_t *, sint32 menge);

	/**
	 * Calculate the JIT2 logic power boost amount using the currently attached transformer.
	 */
	sint32 get_jit2_power_boost() const;

	void step(uint32 delta_t);                  // factory muss auch arbeiten
	void new_month();

	char const* get_name() const;
	void set_name( const char *name );

	PIXVAL get_color() const { return desc->get_color(); }

	player_t *get_owner() const
	{
		grund_t const* const p = welt->lookup(pos);
		return p ? p->first_obj()->get_owner() : 0;
	}

	void open_info_window();

	// infostring on production
	void info_prod(cbuffer_t& buf) const;

	// infostring on targets/sources
	void info_conn(cbuffer_t& buf) const;

	void rdwr(loadsave_t *file);

	/*
	 * Fills the vector with the koords of the tiles.
	 */
	void get_tile_list( vector_tpl<koord> &tile_list ) const;

	/// @returns a vector of factories within a rectangle
	static vector_tpl<fabrik_t *> & sind_da_welche(koord min, koord max);

	// hier die methoden zum parametrisieren der Fabrik

	/// Builds buildings (gebaeude_t) for the factory.
	void build(sint32 rotate, bool build_fields, bool force_initial_prodbase);

	uint8 get_rotate() const { return rotate; }
	void set_rotate( uint8 r ) { rotate = r; }

	/* field generation code
	 * spawns a field for sure if probability>=1000
	 */
	bool add_random_field(uint16 probability);

	void remove_field_at(koord pos);

	uint32 get_field_count() const { return fields.get_count(); }

	uint32 get_min_field_count() const
	{
		const field_group_desc_t *fg = get_desc()->get_field_group();
		return fg ?  fg->get_min_fields() : 0;
	}

	/**
	 * total and current procduction/storage values
	 */
	const array_tpl<ware_production_t>& get_input() const { return input; }
	const array_tpl<ware_production_t>& get_output() const { return output; }

	/**
	 * Production multipliers
	 */
	sint32 get_prodfactor_electric() const { return prodfactor_electric; }
	sint32 get_prodfactor_pax() const { return prodfactor_pax; }
	sint32 get_prodfactor_mail() const { return prodfactor_mail; }
	sint32 get_prodfactor() const { return DEFAULT_PRODUCTION_FACTOR + prodfactor_electric + prodfactor_pax + prodfactor_mail; }

	/* does not takes month length into account */
	sint32 get_base_production() const { return prodbase; }
	void set_base_production(sint32 p);

	sint32 get_current_production() const { return (sint32)welt->scale_with_month_length( ((sint64)prodbase * (sint64)get_prodfactor())>>8 ); }

	/* returns the status of the current factory, as well as output */
	enum {
		STATUS_BAD,
		STATUS_MEDIUM,
		STATUS_GOOD,
		STATUS_INACTIVE,
		STATUS_NOTHING
	};
	static uint8 status_to_color[5];

	uint8  get_status() const { return status; }
	uint32 get_total_in() const { return total_input; }
	uint32 get_total_transit() const { return total_transit; }
	uint32 get_total_out() const { return total_output; }

	/**
	 * Draws some nice colored bars giving some status information
	 */
	void display_status(sint16 xpos, sint16 ypos);

	/**
	 * Crossconnects all factories
	 */
	void add_all_suppliers();

	/* adds a new supplier to this factory
	 * fails if no matching goods are there
	 */
	bool add_supplier(fabrik_t* fab);

	/**
	 * Return the scaled electricity amount and pax/mail demand
	 */
	uint32 get_scaled_electric_demand() const { return scaled_electric_demand; }
	uint32 get_scaled_pax_demand() const { return scaled_pax_demand; }
	uint32 get_scaled_mail_demand() const { return scaled_mail_demand; }

	bool is_end_consumer() const { return (output.empty() && !desc->is_electricity_producer()); }

	// Returns a list of goods produced by this factory.
	slist_tpl<const goods_desc_t*> *get_produced_goods() const;

	// Rebuild the factory inactive caches.
	void rebuild_inactive_cache();

	double get_production_per_second() const;

	/* Calculate work rate using a ramp function.
	 * amount: The current amount.
	 * minimum: Minimum amount before work rate starts ramp down.
	 * maximum: Maximum before production stops.
	 * (opt) precision: Work rate fixed point fractional precision.
	 * returns: Work rate in fixed point form.
	 */
	static sint32 calculate_work_rate_ramp(sint32 const amount, sint32 const minimum, sint32 const maximum, uint32 const precision = WORK_BITS);
};

#endif
