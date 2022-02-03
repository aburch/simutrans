/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMUNITS_H
#define SIMUNITS_H


/*
 * This file is designed to contain the unit conversion routines
 * which were previously scattered all over the Simutrans code.
 *
 * Some are still scattered all over the code.  So this file also documents
 * where the other conversion routines are.
 *
 * I have tried to leave out stuff which should be internal to the display
 * and have no gameplay consequences.
 *
 * DISTANCE units:
 * 1 -- tiles
 * 2 -- "internal pixels" -- 16 per tile.  Old steps per tile.
 *      -- OBJECT_OFFSET_STEPS, located in simconst.h
 *      -- important to the internal rendering engine
 *      -- used for locating images such as trees within a tile
 *      -- same value as carunits, but conceptually different....
 * 3 -- "carunits" or vehicle length units -- same as old steps per tile
 *      -- Length of trains & other vehicles (as specified in paks)
 *      -- is measured in this unit.  Currently this is very hard to alter.
 * 4 -- vehicle steps
 *      -- trains can be on one of 2^8 = 256 locations along a tile horizontally (not
 *         diagonally).
 * 5 -- "yards" -- tiny distance units used internally
 *      -- 2^12 per vehicle steps, by definition
 *      -- 2^16 per vehicle steps in older code
 *      -- chosen to maximize precision of certain interfaces
 * 6 -- km -- 1/tile in standard
 *
 * TIME units:
 * 1 -- ticks -- referred to as ms or milliseconds in old code
 * 2 -- months -- determined by karte_t::ticks_per_world_month
 *      -- you may also use karte_t::ticks_per_world_month_shift
 * 3 -- days -- derived from months
 * 4 -- years -- derived from months
 * 5 -- hours & minutes -- NOT derived from months, implied by vehicle speed
 * 6 -- actual real-time milliseconds -- only for game speed setting
 *      -- no meaning in-game
 *
 * SPEED units:
 * 1 -- "internal" speed -- yards per tick.
 *      -- this is multiplied by delta_t, which is in ticks (in convoi_t::sync_step)
 *      -- to get distance travelled in yards (passed to vehikel_t:fahre_basis)
 * 2 -- km/h -- core setting here is VEHICLE_SPEED_FACTOR
 * 3 -- tiles per tick
 * 4 -- steps per tick
 *
 * ELECTRICITY units:
 * 1 -- "internal" units
 * 2 -- megawatts
 * This conversion still lives in dings/leitung2.h, not here
 */

/**
 * Distance units: conversion between "vehicle steps" and "yards"
 * In bitshift form
 */
#define YARDS_PER_VEHICLE_STEP_SHIFT (12)

/**
 * Mask, applied to yards, to eliminate anything smaller than a vehicle step
 * Assumes yards are in uint32 variables.... derivative of YARDS_PER_VEHICLE_STEP_SHIFT
 * #define YARDS_VEHICLE_STEP_MASK (0xFFFFF000)
 */
#define YARDS_VEHICLE_STEP_MASK ~((1<<YARDS_PER_VEHICLE_STEP_SHIFT)-1)

/**
 * Distance units: vehicle steps per tile
 * A vehicle travelling across a tile horizontally can be in this many
 * distinct locations along the tile
 * This is 256, and making it larger would require changing datatypes within
 * vehikel_t.
 */
#define VEHICLE_STEPS_PER_TILE (256)
#define VEHICLE_STEPS_PER_TILE_SHIFT 8

/**
 * Shift from yards to tiles, derived quantity
 */
#define YARDS_PER_TILE_SHIFT (VEHICLE_STEPS_PER_TILE_SHIFT + YARDS_PER_VEHICLE_STEP_SHIFT)

/**
 * Distance units: "carunits" per tile
 * Forced by history of pak files to 16
 */
#define CARUNITS_PER_TILE (16)

/**
 * Distance units: steps per carunit
 * Derived from the above two....
 */
#define VEHICLE_STEPS_PER_CARUNIT (VEHICLE_STEPS_PER_TILE/CARUNITS_PER_TILE)

/**
 * "Unlimited" speed depends on size of "speed" unit (sint32)
 */
#define SPEED_UNLIMITED (2147483647)    // == SINT32_MAX

/**
 * Global vehicle speed conversion factor between Simutrans speed
 * and km/h
 */
#define VEHICLE_SPEED_FACTOR  (5)

/**
 * Converts speed value to km/h
 * this is speed * 80 / 1024 rounded to nearest
 */
#define speed_to_kmh(speed) (((speed)*VEHICLE_SPEED_FACTOR+31) >> 6)

/**
 * Converts km/h value to speed
 * this is speed * 1024 / 80 = speed * 64 / 5
 */
#define kmh_to_speed(speed) (((speed) << 6) / VEHICLE_SPEED_FACTOR)

/*
 * Converts speed (yards per tick) into tiles per month
 */
// Done in world/simworld.h: speed_to_tiles_per_month

#endif
