/*
 * Copyright (c) 2009 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef convoy_metrics_h
#define convoy_metrics_h

#include "simtypes.h"
#include "besch/vehikel_besch.h"
#include "simworld.h"


/**
 * Calculate some convoy metrics. 
 *
 * Extracted from gui_convoy_assembler_t::zeichnen() and gui_convoy_label_t::zeichnen()
 * @author: Bernd Gabriel
 */
class convoy_metrics_t {
private:
	float power;  // maximum/nominal power in kW
	uint32 length; // length in 1/TILE_STEPSth of a tile
	uint32 vehicle_weight; // in tons
	uint32 current_weight; // in tons (vehicle and currently loaded freight)
	// several freight of the same category may weigh different: 
	uint32 min_freight_weight; // in tons
	uint32 max_freight_weight; // in tons
	// max top speed of convoy limited by minimum top speed of the vehicles.
	uint32 max_top_speed; // in kmh

	double cf;
	double fr;
	double mcos; // m * cos(alpha)
	double msin; // m * sin(alpha)

	void add_friction(int friction, int weight);
	void add_vehicle_besch(const vehikel_besch_t &b);
	void add_vehicle(const vehikel_besch_t &b);
	void add_vehicle(const vehikel_t &v);
	void calc(karte_t &world, vector_tpl<const vehikel_besch_t *> &vehicles);
	void calc(convoi_t &cnv);
	void calc();
	void reset();
public:
	convoy_metrics_t(karte_t &world, vector_tpl<const vehikel_besch_t *> &vehicles) { calc(world, vehicles); };
	convoy_metrics_t(convoi_t &cnv) { calc(cnv); }
public:
	double get_power() { return power; }
	uint32 get_length() { return length; }
	uint32 get_vehicle_weight() { return vehicle_weight; }
	uint32 get_max_freight_weight() { return max_freight_weight; }
	uint32 get_min_freight_weight() { return min_freight_weight; }
	uint32 get_speed(uint32 weight);
	//
	uint32 get_tile_length() { return (length + TILE_STEPS - 1) / TILE_STEPS; }
};

#endif