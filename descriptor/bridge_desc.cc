/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"

#include "bridge_desc.h"
#include "ground_desc.h"
#include "../network/checksum.h"


/**
 * Returns image index of a straight piece (excluding start pieces)
 */
bridge_desc_t::img_t bridge_desc_t::get_straight(ribi_t::ribi ribi, uint8 height) const
{
	if(  height>1 && get_background(NS_Segment2, 0)!=IMG_EMPTY  ) {
		return (ribi & ribi_t::northsouth) ? NS_Segment2 : OW_Segment2;
	}
	else {
		return (ribi & ribi_t::northsouth) ? NS_Segment : OW_Segment;
	}
}


// ditto for pillars
bridge_desc_t::img_t bridge_desc_t::get_pillar(ribi_t::ribi ribi)
{
	return (ribi & ribi_t::northsouth) ? NS_Pillar : OW_Pillar;
}


/**
 * Returns image index of a straight bridge-start piece (on slope)
 */
bridge_desc_t::img_t bridge_desc_t::get_start(slope_t::type slope) const
{
	// if double heights enabled and desc has 2 height images present then use these
	if(  ground_desc_t::double_grounds  &&  get_background(N_Start2, 0) != IMG_EMPTY  ) {
		switch(  slope  ) {
			case slope_t::north: return N_Start;
			case slope_t::south: return S_Start;
			case slope_t::east:  return O_Start;
			case slope_t::west: return W_Start;
			case slope_t::north * 2: return N_Start2;
			case slope_t::south * 2: return S_Start2;
			case slope_t::east  * 2: return O_Start2;
			case slope_t::west * 2: return W_Start2;
		}
	}
	else {
		switch(  slope  ) {
			case slope_t::north: case slope_t::north * 2: return N_Start;
			case slope_t::south: case slope_t::south * 2: return S_Start;
			case slope_t::east:  case slope_t::east  * 2: return O_Start;
			case slope_t::west: case slope_t::west * 2: return W_Start;
		}
	}
	return (img_t) - 1;
}


/**
 * Returns image index of a ramp piece
 */
bridge_desc_t::img_t bridge_desc_t::get_ramp(slope_t::type slope) const
{
	if(  ground_desc_t::double_grounds  &&  has_double_ramp()  ) {
		switch(  slope  ) {
			case slope_t::north: return S_Ramp;
			case slope_t::south: return N_Ramp;
			case slope_t::east:  return W_Ramp;
			case slope_t::west: return O_Ramp;
			case slope_t::north * 2: return S_Ramp2;
			case slope_t::south * 2: return N_Ramp2;
			case slope_t::east  * 2: return W_Ramp2;
			case slope_t::west * 2: return O_Ramp2;
		}
	}
	else {
		switch(  slope  ) {
			case slope_t::north: case slope_t::north * 2: return S_Ramp;
			case slope_t::south: case slope_t::south * 2: return N_Ramp;
			case slope_t::east:  case slope_t::east  * 2: return W_Ramp;
			case slope_t::west: case slope_t::west * 2: return O_Ramp;
		}
	}
	return (img_t) - 1;
}


/**
 * returns image index for appropriate ramp or start image given ground and way slopes
 */
bridge_desc_t::img_t bridge_desc_t::get_end(slope_t::type test_slope, slope_t::type ground_slope, slope_t::type way_slope) const
{
	img_t end_image;
	if(  test_slope == slope_t::flat  ) {
		end_image = get_ramp( way_slope );
	}
	else {
		end_image = get_start( ground_slope );
	}
	return end_image;
}


/**
 * returns whether desc has double height images for ramps
 */
bool bridge_desc_t::has_double_ramp() const
{
	return (get_background(bridge_desc_t::N_Ramp2, 0)!=IMG_EMPTY || get_foreground(bridge_desc_t::N_Ramp2, 0)!=IMG_EMPTY);
}

bool bridge_desc_t::has_double_start() const
{
	return (get_background(bridge_desc_t::N_Start2, 0) != IMG_EMPTY  ||  get_foreground(bridge_desc_t::N_Start2, 0) != IMG_EMPTY);
}


void bridge_desc_t::calc_checksum(checksum_t *chk) const
{
	obj_desc_transport_infrastructure_t::calc_checksum(chk);
	chk->input(pillars_every);
	chk->input(pillars_asymmetric);
	chk->input(max_length);
	chk->input(max_height);
}
