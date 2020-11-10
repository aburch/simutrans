/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simunits.h"

const float32e8_t kmh2ms((uint32) 10, (uint32) 36);
const float32e8_t ms2kmh((uint32) 36, (uint32) 10);

/**
 * Conversion between simutrans speed and m/s
 */

// scale to convert between simutrans speed and m/s
const float32e8_t simspeed2ms((uint32) 10 * VEHICLE_SPEED_FACTOR, (uint32) 36 * 64);
const float32e8_t ms2simspeed((uint32) 36 * 64, (uint32) 10 * VEHICLE_SPEED_FACTOR);

/**
 * Conversion between simutrans steps and meters
 */
// scale to convert between simutrans steps and meters
//const float32e8_t yards2m((uint32) 10 * VEHICLE_SPEED_FACTOR, (uint32) 36 * 1024 * DT_TIME_FACTOR);
//const float32e8_t m2yards((uint32) 36 * 1024 * DT_TIME_FACTOR, (uint32) 10 * VEHICLE_SPEED_FACTOR);
const float32e8_t steps2yards((uint32)1 << YARDS_PER_VEHICLE_STEP_SHIFT);

const sint32 SPEED_MIN = kmh_to_speed(KMH_MIN);
const float32e8_t V_MIN = kmh2ms * KMH_MIN;

