/*
 * Copyright (c) 2010 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * way_constraints_t: a hood for James Petts' permissive and prohibitive way constraints.
 */

#pragma once

#ifndef way_constraints_h
#define way_constraints_h

#include "../simtypes.h"


typedef
	uint8 way_constraints_mask;


class way_constraints_t
{
private:
	way_constraints_mask permissive;		// If vehicle has it, way must have it.
	way_constraints_mask prohibitive;	// If way has it, vehicle must have it.
public:
	way_constraints_t() {}
	way_constraints_t(const way_constraints_mask permissive, const way_constraints_mask prohibitive) {
		this->permissive = permissive;
		this->prohibitive = prohibitive;
	}

	// number of constaints == number of bits in type way_constraints
	static uint8 get_count() { return sizeof(way_constraints_mask) * 8; }
	// getters
	way_constraints_mask get_permissive() const { return permissive; } 
	way_constraints_mask get_prohibitive() const { return prohibitive; }
	bool get_permissive(uint8 i) const { return permissive & (1<<i); }
	bool get_prohibitive(uint8 i) const { return prohibitive & (1<<i); }
	// setters
	void set_permissive(const way_constraints_mask permissive) { this->permissive = permissive; }
	void set_prohibitive(const way_constraints_mask prohibitive) { this->prohibitive = prohibitive; }
	void add(const way_constraints_t& add) { permissive |= add.permissive; 	prohibitive |= add.prohibitive;	}
};

class way_constraints_of_vehicle_t : public way_constraints_t
{
public:
	way_constraints_of_vehicle_t() : way_constraints_t(0, 0) {}
};

class way_constraints_of_way_t : public way_constraints_t
{
public:
	way_constraints_of_way_t() : way_constraints_t(0, 0) {}
} GCC_PACKED;

class missing_way_constraints_t : public way_constraints_t
{
public:
	missing_way_constraints_t(const way_constraints_of_vehicle_t &vehicle, const way_constraints_of_way_t &way) {
		set_permissive(vehicle.get_permissive() & ~way.get_permissive());
		set_prohibitive(~vehicle.get_prohibitive() & way.get_prohibitive());
	}

	/**
	 * can vehicle use way? returns true, if vehicle can use the way according to its and the way's way constraints.
	 */
	bool ist_befahrbar() { return !get_permissive() && !get_prohibitive(); }
};


#endif
