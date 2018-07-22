/*
 * Roads for Simutrans
 *
 * Revised January 2001
 * Hj. Malthaner
 */

#include <stdio.h>

#include "strasse.h"
#include "../../simworld.h"
#include "../../dataobj/loadsave.h"
#include "../../descriptor/way_desc.h"
#include "../../bauer/wegbauer.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/environment.h"
#include "../../vehicle/simvehicle.h"
#include "../../vehicle/simroadtraffic.h"


const way_desc_t *strasse_t::default_strasse=NULL;

bool strasse_t::show_masked_ribi = false;
bool strasse_t::show_reservations = false;


void strasse_t::set_gehweg(bool janein)
{
	weg_t::set_gehweg(janein);
	if(janein  &&  get_desc()  &&  get_desc()->get_topspeed()>50) {
		set_max_speed(50);
	}
}



strasse_t::strasse_t(loadsave_t *file) : weg_t()
{
	rdwr(file);
	for(uint8 i=0; i<4; i++) {
		reserved_by[i] = NULL;
	}
}


strasse_t::strasse_t() : weg_t()
{
	set_gehweg(false);
	set_desc(default_strasse);
	ribi_mask_oneway =ribi_t::none;
	overtaking_mode = twoway_mode;
	street_flags = 0;
	prior_direction_setting = 0;
	for(uint8 type=0; type<MAX_WAY_STATISTICS; type++) {
		for(uint8 month=0; month<MAX_WAY_STAT_MONTHS; month++) {
			for(uint8 dir=0; dir<MAX_WAY_STAT_DIRECTIONS; dir++) {
				directional_statistics[month][type][dir] = 0;
			}
		}
	}
	for(uint8 i=0; i<4; i++) {
		reserved_by[i] = NULL;
	}
}



void strasse_t::rdwr(loadsave_t *file)
{
	xml_tag_t s( file, "strasse_t" );

	weg_t::rdwr(file);

	if(  file->get_OTRP_version() >= 15  ) {
		uint8 s = street_flags;
		file->rdwr_byte(s);
		street_flags = s;
	} else {
		street_flags = 0;
	}
	
	if(  file->get_OTRP_version() >= 14  ) {
		uint8 s = prior_direction_setting;
		file->rdwr_byte(s);
		prior_direction_setting = s;
		// directional statistics
		for(uint8 type=0; type<MAX_WAY_STATISTICS; type++) {
			for(uint8 month=0; month<MAX_WAY_STAT_MONTHS; month++) {
				for(uint8 dir=0; dir<MAX_WAY_STAT_DIRECTIONS; dir++) {
					sint16 w = directional_statistics[month][type][dir];
					file->rdwr_short(w);
					directional_statistics[month][type][dir] = w;
				}
			}
		}
	} else {
		prior_direction_setting = 0;
		for(uint8 type=0; type<MAX_WAY_STATISTICS; type++) {
			for(uint8 month=0; month<MAX_WAY_STAT_MONTHS; month++) {
				for(uint8 dir=0; dir<MAX_WAY_STAT_DIRECTIONS; dir++) {
					directional_statistics[month][type][dir] = 0;
				}
			}
		}
	}

	if(  (env_t::previous_OTRP_data  &&  file->get_version() >= 120006)  ||  file->get_OTRP_version() >= 14  ) {
		uint8 mask_oneway = get_ribi_mask_oneway();
		file->rdwr_byte(mask_oneway);
		set_ribi_mask_oneway(mask_oneway);
		sint8 ov = get_overtaking_mode();
		file->rdwr_byte(ov);
		overtaking_mode_t nov = (overtaking_mode_t)ov;
		set_overtaking_mode(nov);
	} else {
		set_ribi_mask_oneway(ribi_t::none);
		set_overtaking_mode(twoway_mode);
	}

	if(file->get_version()<89000) {
		bool gehweg;
		file->rdwr_bool(gehweg);
		set_gehweg(gehweg);
	}

	if(file->is_saving()) {
		const char *s = get_desc()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));

		const way_desc_t *desc = way_builder_t::get_desc(bname);
		int old_max_speed = get_max_speed();
		if(desc==NULL) {
			desc = way_builder_t::get_desc(translator::compatibility_name(bname));
			if(desc==NULL) {
				desc = default_strasse;
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("strasse_t::rdwr()", "Unknown street %s replaced by %s (old_max_speed %i)", bname, desc->get_name(), old_max_speed );
		}
		set_desc(desc);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
		if(desc->get_topspeed()>50  &&  hat_gehweg()) {
			set_max_speed(50);
		}
	}
}

void strasse_t::update_ribi_mask_oneway(ribi_t::ribi mask, ribi_t::ribi allow)
{
	// assertion. @mask and @allow must be single or none.
	if(!(ribi_t::is_single(mask)||(mask==ribi_t::none))) dbg->error( "weg_t::update_ribi_mask_oneway()", "mask is not single or none.");
	if(!(ribi_t::is_single(allow)||(allow==ribi_t::none))) dbg->error( "weg_t::update_ribi_mask_oneway()", "allow is not single or none.");

	if(  mask==ribi_t::none  ) {
		if(  ribi_t::is_twoway(get_ribi_unmasked())  ) {
			// auto complete
			ribi_mask_oneway |= (get_ribi_unmasked()-allow);
		}
	} else {
		ribi_mask_oneway |= mask;
	}
	// remove backward ribi
	if(  allow==ribi_t::none  ) {
		if(  ribi_t::is_twoway(get_ribi_unmasked())  ) {
			// auto complete
			ribi_mask_oneway &= ~(get_ribi_unmasked()-mask);
		}
	} else {
		ribi_mask_oneway &= ~allow;
	}
}


ribi_t::ribi strasse_t::get_ribi() const {
	ribi_t::ribi ribi = get_ribi_unmasked();
	ribi_t::ribi ribi_maske = get_ribi_maske();
	if(  get_waytype()==road_wt  &&  overtaking_mode<=oneway_mode  ) {
		return (ribi_t::ribi)((ribi & ~ribi_maske) & ~ribi_mask_oneway);
	} else {
		return (ribi_t::ribi)(ribi & ~ribi_maske);
	}
}

void strasse_t::rotate90() {
	weg_t::rotate90();
	ribi_mask_oneway = ribi_t::rotate90( ribi_mask_oneway );
}

void strasse_t::init_statistics() {
	weg_t::init_statistics();
	for(uint8 type=0; type<MAX_WAY_STATISTICS; type++) {
		for(uint8 month=0; month<MAX_WAY_STAT_MONTHS; month++) {
			for(uint8 dir=0; dir<MAX_WAY_STAT_DIRECTIONS; dir++) {
				directional_statistics[month][type][dir] = 0;
			}
		}
	}
}

void strasse_t::book(int amount, way_statistics type, ribi_t::ribi dir) {
	weg_t::book(amount, type);
	if(  (dir&(ribi_t::north))!=0  ||  (dir&(ribi_t::south))!=0  ) {
		// north-south traffic
		directional_statistics[0][type][0] += amount;
	} else {
		// east-west traffic
		directional_statistics[0][type][1] += amount;
	}
}

void strasse_t::new_month() {
	weg_t::new_month();
	for(uint8 type=0; type<MAX_WAY_STATISTICS; type++) {
		for(uint8 dir=0; dir<MAX_WAY_STAT_DIRECTIONS; dir++) {
			for(uint8 month=MAX_WAY_STAT_MONTHS-1; month>0; month--) {
				directional_statistics[month][type][dir] = directional_statistics[month-1][type][dir];
			}
			directional_statistics[0][type][dir] = 0;
		}
	}
}

ribi_t::ribi strasse_t::get_prior_direction() const {
	if(  directional_statistics[1][1][0]>directional_statistics[1][1][1]  ) {
		return ribi_t::northsouth;
	} else if(  directional_statistics[1][1][0]==directional_statistics[1][1][1]  ) {
		if(  directional_statistics[0][1][0]>directional_statistics[0][1][1]  ) {
			return ribi_t::northsouth;
		} else if(  directional_statistics[0][1][0]<directional_statistics[0][1][1]  ) {
			return ribi_t::eastwest;
		} else {
			return ribi_t::all;
		}
	} else {
		return ribi_t::eastwest;
	}
}

uint8 calc_reservation_flag(ribi_t::ribi dir_in, ribi_t::ribi dir_out) {
	if(  dir_in==ribi_t::north  &&  dir_out==ribi_t::east  ) { return 13; }
	else if(  dir_in==ribi_t::north  &&  dir_out==ribi_t::south  ) { return 5; }
	else if(  dir_in==ribi_t::north  &&  dir_out==ribi_t::west   ) { return 1; }
	else if(  dir_in==ribi_t::east   &&  dir_out==ribi_t::north  ) { return 2; }
	else if(  dir_in==ribi_t::east   &&  dir_out==ribi_t::south  ) { return 7; }
	else if(  dir_in==ribi_t::east   &&  dir_out==ribi_t::west   ) { return 3; }
	else if(  dir_in==ribi_t::south  &&  dir_out==ribi_t::north  ) { return 10;}
	else if(  dir_in==ribi_t::south  &&  dir_out==ribi_t::east   ) { return 8; }
	else if(  dir_in==ribi_t::south  &&  dir_out==ribi_t::west   ) { return 11;}
	else if(  dir_in==ribi_t::west   &&  dir_out==ribi_t::north  ) { return 14;}
	else if(  dir_in==ribi_t::west   &&  dir_out==ribi_t::east   ) { return 12;}
	else if(  dir_in==ribi_t::west   &&  dir_out==ribi_t::south  ) { return 4; }
	else { return 0; }
}

bool strasse_t::is_reserved_by_others(vehicle_base_t* r, bool is_overtaking, koord3d pos_prev, koord3d pos_next) {
	ribi_t::ribi dir_in = ribi_type(get_pos(), pos_prev);
	ribi_t::ribi dir_out = ribi_type(get_pos(), pos_next);
	uint8 reservation_flag = calc_reservation_flag(dir_in, dir_out);
	if(  reservation_flag==0  ) {
		// failed to calculate reservation flag.
		return false;
	}
	if(  (welt->get_settings().is_drive_left()  &&  !is_overtaking)  ||  (!welt->get_settings().is_drive_left()  &&  is_overtaking)  ) {
		reservation_flag = (~reservation_flag)&0x0F;
	}
	// check whether the tile is reserved by others.
	for(uint8 i=0; i<4; i++) {
		if(  (reservation_flag&(1<<i))!=0  &&  reserved_by[i]  &&  reserved_by[i]!=r  ) {
			return true;
		}
	}
	return false;
}

bool strasse_t::reserve(vehicle_base_t* r, bool is_overtaking, koord3d pos_prev, koord3d pos_next) {
	ribi_t::ribi dir_in = ribi_type(get_pos(), pos_prev);
	ribi_t::ribi dir_out = ribi_type(get_pos(), pos_next);
	uint8 reservation_flag = calc_reservation_flag(dir_in, dir_out);
	if(  reservation_flag==0  ) {
		// failed to calculate reservation flag.
		// allow entering this tile.
		return true;
	}
	if(  (welt->get_settings().is_drive_left()  &&  !is_overtaking)  ||  (!welt->get_settings().is_drive_left()  &&  is_overtaking)  ) {
		reservation_flag = (~reservation_flag)&0x0F;
	}
	// check whether the tile is reserved by others.
	for(uint8 i=0; i<4; i++) {
		if(  (reservation_flag&(1<<i))!=0  &&  reserved_by[i]  &&  reserved_by[i]!=r  ) {
			return false;
		}
	}
	// now we can reserve the tile
	for(uint8 i=0; i<4; i++) {
		if(  (reservation_flag&(1<<i))!=0  ) {
			reserved_by[i] = r;
		}
	}
	if(strasse_t::show_reservations) {
		set_flag( obj_t::dirty );
	}
	return true;
}

bool strasse_t::unreserve(vehicle_base_t* r) {
	bool deleted = false;
	for(uint8 i=0; i<4; i++) {
		if(  r==reserved_by[i]  ) {
			reserved_by[i] = NULL;
			deleted = true;
		}
	}
	if(  deleted  ) {
		if(strasse_t::show_reservations) {
			set_flag( obj_t::dirty );
		}
	}
	return deleted;
}

void strasse_t::unreserve_all() {
	for(uint8 i=0; i<4; i++) {
		if(  reserved_by[i]  ) {
			if(  road_vehicle_t* r = obj_cast<road_vehicle_t>(reserved_by[i])  ) {
				r->unreserve_all_tiles();
			}
			else if(  private_car_t* p = obj_cast<private_car_t>(reserved_by[i])  ) {
				p->unreserve_all_tiles();
			}
			reserved_by[i] = NULL;
		}
	}
	if(strasse_t::show_reservations) {
		set_flag( obj_t::dirty );
	}
}

FLAGGED_PIXVAL strasse_t::get_outline_colour() const {
	bool reserved = false;
	if(  show_reservations  ) {
		for(uint8 i=0; i<4; i++) {
			if(  reserved_by[i]  ) {
				reserved = true;
			}
		}
	}
	if(  reserved  ) {
		return TRANSPARENT75_FLAG | OUTLINE_FLAG | color_idx_to_rgb(COL_RED);
	}else if(  show_masked_ribi  &&  get_avoid_cityroad()  ) {
		return TRANSPARENT75_FLAG | OUTLINE_FLAG | color_idx_to_rgb(COL_GREEN);
	}else {
		return 0;
	}
}
