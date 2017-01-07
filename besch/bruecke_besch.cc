#include "../simdebug.h"

#include "bruecke_besch.h"
#include "grund_besch.h"
#include "../network/checksum.h"


/*
 *  Author:
 *      Volker Meyer
 *
 *  Description:
 *      Richtigen Index für einfaches Brückenstück bestimmen
 */
bruecke_besch_t::img_t bruecke_besch_t::get_simple(ribi_t::ribi ribi, uint8 height) const
{
	if(  height>1 && get_background(NS_Segment2, 0)!=IMG_EMPTY  ) {
		return (ribi & ribi_t::northsouth) ? NS_Segment2 : OW_Segment2;
	}
	else {
		return (ribi & ribi_t::northsouth) ? NS_Segment : OW_Segment;
	}
}


// dito for pillars
bruecke_besch_t::img_t bruecke_besch_t::get_pillar(ribi_t::ribi ribi)
{
	return (ribi & ribi_t::northsouth) ? NS_Pillar : OW_Pillar;
}


/*
 *  Author:
 *      Volker Meyer
 *
 *  Description:
 *      Richtigen Index für klassischen Hangstart ück bestimmen
 */
bruecke_besch_t::img_t bruecke_besch_t::get_start(slope_t::type slope) const
{
	// if double heights enabled and desc has 2 height images present then use these
	if(  grund_besch_t::double_grounds  &&  get_background(N_Start2, 0) != IMG_EMPTY  ) {
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


/*
 *  Author:
 *      Volker Meyer
 *
 *  Description:
 *      Richtigen Index für Rampenstart ück bestimmen
 */
bruecke_besch_t::img_t bruecke_besch_t::get_rampe(slope_t::type slope) const
{
	if(  grund_besch_t::double_grounds  &&  has_double_ramp()  ) {
		switch(  slope  ) {
			case slope_t::north: return S_Rampe;
			case slope_t::south: return N_Rampe;
			case slope_t::east:  return W_Rampe;
			case slope_t::west: return O_Rampe;
			case slope_t::north * 2: return S_Rampe2;
			case slope_t::south * 2: return N_Rampe2;
			case slope_t::east  * 2: return W_Rampe2;
			case slope_t::west * 2: return O_Rampe2;
		}
	}
	else {
		switch(  slope  ) {
			case slope_t::north: case slope_t::north * 2: return S_Rampe;
			case slope_t::south: case slope_t::south * 2: return N_Rampe;
			case slope_t::east:  case slope_t::east  * 2: return W_Rampe;
			case slope_t::west: case slope_t::west * 2: return O_Rampe;
		}
	}
	return (img_t) - 1;
 }


/*
 *  Author:
 *      Kieron Green
 *
 *  Description:
 *      returns image index for appropriate ramp or start image given ground and way slopes
 */
bruecke_besch_t::img_t bruecke_besch_t::get_end(slope_t::type test_slope, slope_t::type ground_slope, slope_t::type way_slope) const
{
	img_t end_image;
	if(  test_slope == slope_t::flat  ) {
		end_image = get_rampe( way_slope );
	}
	else {
		end_image = get_start( ground_slope );
	}
	return end_image;
}


/*
 *  Author:
 *      Kieron Green
 *
 *  Description:
 *      returns whether desc has double height images for ramps
 */
bool bruecke_besch_t::has_double_ramp() const
{
	return (get_background(bruecke_besch_t::N_Rampe2, 0)!=IMG_EMPTY || get_foreground(bruecke_besch_t::N_Rampe2, 0)!=IMG_EMPTY);
}

bool bruecke_besch_t::has_double_start() const
{
	return (get_background(bruecke_besch_t::N_Start2, 0) != IMG_EMPTY  ||  get_foreground(bruecke_besch_t::N_Start2, 0) != IMG_EMPTY);
}


void bruecke_besch_t::calc_checksum(checksum_t *chk) const
{
	obj_desc_transport_infrastructure_t::calc_checksum(chk);
	chk->input(pillars_every);
	chk->input(pillars_asymmetric);
	chk->input(max_length);
	chk->input(max_height);
}
