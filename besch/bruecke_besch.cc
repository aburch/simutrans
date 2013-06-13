#include "../simdebug.h"

#include "bruecke_besch.h"
#include "grund_besch.h"
#include "../utils/checksum.h"


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Richtigen Index für einfaches Brückenstück bestimmen
 */
bruecke_besch_t::img_t bruecke_besch_t::get_simple(ribi_t::ribi ribi)
{
	return (ribi & ribi_t::nordsued) ? NS_Segment : OW_Segment;
}


// dito for pillars
bruecke_besch_t::img_t bruecke_besch_t::get_pillar(ribi_t::ribi ribi)
{
	return (ribi & ribi_t::nordsued) ? NS_Pillar : OW_Pillar;
}


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Richtigen Index für klassischen Hangstart ück bestimmen
 */
bruecke_besch_t::img_t bruecke_besch_t::get_start(hang_t::typ slope) const
{
	// if double heights enabled and besch has 2 height images present then use these
	if(  grund_besch_t::double_grounds  &&  get_child<bildliste_besch_t>(1 + offset)->get_bild(N_Start2) != NULL  ) {
		switch(  slope  ) {
			case hang_t::nord: return N_Start;
			case hang_t::sued: return S_Start;
			case hang_t::ost:  return O_Start;
			case hang_t::west: return W_Start;
			case hang_t::nord * 2: return N_Start2;
			case hang_t::sued * 2: return S_Start2;
			case hang_t::ost  * 2: return O_Start2;
			case hang_t::west * 2: return W_Start2;
		}
	}
	else {
		switch(  slope  ) {
			case hang_t::nord: case hang_t::nord * 2: return N_Start;
			case hang_t::sued: case hang_t::sued * 2: return S_Start;
			case hang_t::ost:  case hang_t::ost  * 2: return O_Start;
			case hang_t::west: case hang_t::west * 2: return W_Start;
		}
	}
	return (img_t) - 1;
}


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Richtigen Index für Rampenstart ück bestimmen
 */
bruecke_besch_t::img_t bruecke_besch_t::get_rampe(hang_t::typ slope)
{
	switch(  slope  ) {
		case hang_t::nord: return S_Rampe;
		case hang_t::sued: return N_Rampe;
		case hang_t::ost:  return W_Rampe;
		case hang_t::west: return O_Rampe;
		case hang_t::nord * 2: return S_Rampe;
		case hang_t::sued * 2: return N_Rampe;
		case hang_t::ost  * 2: return W_Rampe;
		case hang_t::west * 2: return O_Rampe;
		default: return (img_t) - 1;
	}
 }


/*
 *  Author:
 *      Kieron Green
 *
 *  Description:
 *      returns image index for appropriate ramp or start image given ground and way slopes
 */
bruecke_besch_t::img_t bruecke_besch_t::get_end(hang_t::typ test_slope, hang_t::typ ground_slope, hang_t::typ way_slope) const
{
	img_t end_image;
	if(  test_slope == hang_t::flach  ) {
		end_image = get_rampe( way_slope );
	}
	else {
		end_image = get_start( ground_slope );
	}
	return end_image;
}


void bruecke_besch_t::calc_checksum(checksum_t *chk) const
{
	chk->input(topspeed);
	chk->input(preis);
	chk->input(maintenance);
	chk->input(wegtyp);
	chk->input(pillars_every);
	chk->input(pillars_asymmetric);
	chk->input(max_length);
	chk->input(max_height);
	chk->input(intro_date);
	chk->input(obsolete_date);
}
