/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <cmath>

#include "../simdebug.h"
#include "../gui/simwin.h"
#include "../simtypes.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../simtool.h"
#include "../display/simimg.h"

#include "../utils/cbuffer_t.h"
#include "../gui/messagebox.h"
#include "../descriptor/building_desc.h"
#include "../boden/grund.h"
#include "../obj/gebaeude.h"
#include "../player/simplay.h"
#include "../simdepot.h"
#include "loadsave.h"
#include "translator.h"

#include "schedule.h"

#include "../tpl/slist_tpl.h"


schedule_entry_t schedule_t::dummy_entry(koord3d::invalid, 0, 0, 0);


// copy all entries from schedule src to this and adjusts current_stop
void schedule_t::copy_from(const schedule_t *src)
{
	// make sure, we can access both
	if(  src==NULL  ) {
		dbg->fatal("schedule_t::copy_to()","cannot copy from NULL");
		return;
	}
	entries.clear();
	FOR(minivec_tpl<schedule_entry_t>, const& i, src->entries) {
		entries.append(i);
	}
	set_current_stop( src->get_current_stop() );

	editing_finished = src->is_editing_finished();
	flags = src->get_flags();
	max_speed = src->get_max_speed();
}



bool schedule_t::is_stop_allowed(const grund_t *gr) const
{
	// first: check, if we can go here
	waytype_t const my_waytype = get_waytype();
	bool ok = gr->hat_weg(my_waytype);
	if(  !ok  ) {
		if(  my_waytype==air_wt  ) {
			// everywhere is ok but not on stops (we have to load at airports only ...)
			ok = !gr->get_halt().is_bound();
		}
		else if(  my_waytype==water_wt  &&  gr->get_typ()==grund_t::wasser  ) {
			ok = true;
		}
		else if(  my_waytype==tram_wt  ) {
			// tram rails are track internally
			ok = gr->hat_weg(track_wt);
		}
	}

	if(  ok  ) {
		// ok, we can go here; but we must also check, that we are not entering a foreign depot
		depot_t *dp = gr->get_depot();
		ok &= (dp==NULL  ||  (int)dp->get_tile()->get_desc()->get_extra()==my_waytype);
	}

	return ok;
}



/* returns a valid halthandle if there is a next halt in the schedule;
 * it may however not be allowed to load there, if the owner mismatches!
 */
halthandle_t schedule_t::get_next_halt( player_t *player, halthandle_t halt ) const
{
	if(  entries.get_count()>1  ) {
		for(  uint i=1;  i < entries.get_count();  i++  ) {
			halthandle_t h = haltestelle_t::get_halt( entries[ (current_stop+i) % entries.get_count() ].pos, player );
			if(  h.is_bound()  &&  h != halt  ) {
				return h;
			}
		}
	}
	return halthandle_t();
}


/* returns a valid halthandle if there is a previous halt in the schedule;
 * it may however not be allowed to load there, if the owner mismatches!
 */
halthandle_t schedule_t::get_prev_halt( player_t *player ) const
{
	if(  entries.get_count()>1  ) {
		for(  uint i=1;  i < entries.get_count()-1u;  i++  ) {
			halthandle_t h = haltestelle_t::get_halt( entries[ (current_stop+entries.get_count()-i) % entries.get_count() ].pos, player );
			if(  h.is_bound()  ) {
				return h;
			}
		}
	}
	return halthandle_t();
}


bool schedule_t::insert(const grund_t* gr, uint8 minimum_loading, uint16 waiting_time_shift, uint8 coupling_point )
{
	// stored in minivec, so we have to avoid adding too many
	if(  entries.get_count()>=254  ) {
		create_win( new news_img("Maximum 254 stops\nin a schedule!\n"), w_time_delete, magic_none);
		return false;
	}

	if(  is_stop_allowed(gr)  ) {
		entries.insert_at(current_stop, schedule_entry_t(gr->get_pos(), minimum_loading, waiting_time_shift, coupling_point));
		current_stop ++;
		make_current_stop_valid();
		return true;
	}
	else {
		// too many stops or wrong kind of stop
		create_win( new news_img(get_error_msg()), w_time_delete, magic_none);
		return false;
	}
}



bool schedule_t::append(const grund_t* gr, uint8 minimum_loading, uint16 waiting_time_shift, uint8 coupling_point)
{
	// stored in minivec, so we have to avoid adding too many
	if(entries.get_count()>=254) {
		create_win( new news_img("Maximum 254 stops\nin a schedule!\n"), w_time_delete, magic_none);
		return false;
	}

	if(is_stop_allowed(gr)) {
		entries.append(schedule_entry_t(gr->get_pos(), minimum_loading, waiting_time_shift, coupling_point), 4);
		return true;
	}
	else {
		DBG_MESSAGE("schedule_t::append()","forbidden stop at %i,%i,%i",gr->get_pos().x, gr->get_pos().x, gr->get_pos().z );
		// error
		create_win( new news_img(get_error_msg()), w_time_delete, magic_none);
		return false;
	}
}



// cleanup a schedule
void schedule_t::cleanup()
{
	if(  entries.empty()  ) {
		return; // nothing to check
	}
	make_current_stop_valid();
}



bool schedule_t::remove()
{
	bool ok = entries.remove_at(current_stop);
	make_current_stop_valid();
	return ok;
}



void schedule_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fahrplan_t" );

	make_current_stop_valid();

	uint8 size = entries.get_count();
	if(  file->is_version_less(101, 1)  ) {
		uint32 dummy=current_stop;
		file->rdwr_long(dummy);
		current_stop = (uint8)dummy;

		sint32 maxi=size;
		file->rdwr_long(maxi);
		DBG_MESSAGE("schedule_t::rdwr()","read schedule %p with %i entries",this,maxi);
		if(file->is_version_less(86, 10)) {
			// old array had different maxi-counter
			maxi ++;
		}
		size = (uint8)max(0,maxi);
	}
	else {
		file->rdwr_byte(current_stop);
		file->rdwr_byte(size);
	}
	entries.resize(size);
	
	if(  file->get_OTRP_version()>=24  ) {
		file->rdwr_byte(flags);
	}
	else if(  file->get_OTRP_version()>=23  ) {
		bool dummy;
		file->rdwr_bool(dummy); // temporary
		set_temporary(dummy);
		file->rdwr_bool(dummy); // same_dep_time
		set_same_dep_time(dummy);
	} else {
		flags = NONE;
	}
	
	if(  file->get_OTRP_version()>=25  ) {
		file->rdwr_short(max_speed);
	} else {
		max_speed = 0;
	}

	if(file->is_version_less(99, 12)) {
		for(  uint8 i=0; i<size; i++  ) {
			koord3d pos;
			uint32 dummy;
			pos.rdwr(file);
			file->rdwr_long(dummy);
			entries.append(schedule_entry_t(pos, (uint8)dummy, 0, 0));
		}
	}
	else {
		// loading/saving new version
		for(  uint8 i=0;  i<size;  i++  ) {
			if(entries.get_count()<=i) {
				entries.append( schedule_entry_t() );
				entries[i] .waiting_time_shift = 0;
			}
			entries[i].pos.rdwr(file);
			file->rdwr_byte(entries[i].minimum_loading);
			if(file->get_OTRP_version()>=26) {
				file->rdwr_short(entries[i].waiting_time_shift);
			}
			else if(file->is_version_atleast(99, 18)) {
				sint8 n = 0;
				// Conversion for standard compatible writing.
				if(  entries[i].waiting_time_shift>0  ) {
					n = 16-max(int(log2(entries[i].waiting_time_shift)), 9);
				}
				file->rdwr_byte(n);
				if(  file->is_loading()  ) {
					if(  n==0  ) {
						entries[i].waiting_time_shift = 0;
					} else {
						entries[i].waiting_time_shift = ( 1<<(16-n) );
					}
				}
			}
			if(file->get_OTRP_version()>=22) {
				uint8 flags = entries[i].get_stop_flags();
				file->rdwr_byte(flags);
				entries[i].set_stop_flags(flags);
			} else {
				entries[i].set_stop_flags(0);
			}
			if(file->get_OTRP_version()>=25) {
				// prepare for configurable departure slots
				file->rdwr_short(entries[i].spacing);
				file->rdwr_short(entries[i].delay_tolerance);
				uint16 dummy = 1;
				file->rdwr_short(dummy); // num of departure slots
				file->rdwr_short(entries[i].spacing_shift);
			}
			else if(file->get_OTRP_version()>=23) {
				file->rdwr_short(entries[i].spacing);
				file->rdwr_short(entries[i].spacing_shift);
				file->rdwr_short(entries[i].delay_tolerance);
				// v23 can violate spacing must be larger than 0 limitation.
				if(  file->is_loading()  &&  entries[i].spacing<1  ) {
					entries[i].spacing = 1;
				}
			} else {
				entries[i].spacing = 1;
				entries[i].spacing_shift = entries[i].delay_tolerance = 0;
			}
		}
	}
	if(file->is_loading()) {
		editing_finished = true;
	}
	if(current_stop>=entries.get_count()  ) {
		if (!entries.empty()) {
			dbg->error("schedule_t::rdwr()","current_stop %i >count %i => current_stop = 0", current_stop, entries.get_count() );
		}
		current_stop = 0;
	}
}


void schedule_t::rotate90( sint16 y_size )
{
	// now we have to rotate all entries ...
	FOR(minivec_tpl<schedule_entry_t>, & i, entries) {
		i.pos.rotate90(y_size);
	}
}


/**
 * compare this schedule (schedule) with another, passed in schedule
 */
bool schedule_t::matches(karte_t *welt, const schedule_t *schedule)
{
	if(  schedule == NULL  ) {
		return false;
	}
	// same pointer => equal!
	if(  this==schedule  ) {
		return true;
	}
	// no match for empty schedules
	if(  schedule->entries.empty()  ||  entries.empty()  ) {
		return false;
	}
	// now we have to check all entries ...
	// we need to do this that complicated, because the last stop may make the difference
	uint16 f1=0, f2=0;
	while(  f1+f2<entries.get_count()+schedule->entries.get_count()  ) {
		if(f1<entries.get_count()  &&  f2<schedule->entries.get_count()  &&  schedule->entries[(uint8)f2].pos == entries[(uint8)f1].pos) {
			// minimum_loading/waiting ignored: identical
			f1++;
			f2++;
		}
		else {
			bool ok = false;
			if(  f1<entries.get_count()  ) {
				grund_t *gr1 = welt->lookup(entries[(uint8)f1].pos);
				if(  gr1  &&  gr1->get_depot()  ) {
					// skip depot
					f1++;
					ok = true;
				}
			}
			if(  f2<schedule->entries.get_count()  ) {
				grund_t *gr2 = welt->lookup(schedule->entries[(uint8)f2].pos);
				if(  gr2  &&  gr2->get_depot()  ) {
					ok = true;
					f2++;
				}
			}
			// no depot but different => do not match!
			if(  !ok  ) {
				/* in principle we could also check for same halt; but this is dangerous,
				 * since a rebuilding of a single square might change that
				 */
				return false;
			}
		}
	}
	return f1==entries.get_count()  &&  f2==schedule->entries.get_count();
}



/**
 * Ordering based on halt id
 */
class HaltIdOrdering
{
public:
	bool operator()(const halthandle_t& a, const halthandle_t& b) const { return a.get_id() < b.get_id(); }
};


/**
 * compare this schedule (schedule) with another, ignoring order and exact positions and waypoints
 */
bool schedule_t::similar( const schedule_t *schedule, const player_t *player )
{
	if(  schedule == NULL  ) {
		return false;
	}
	// same pointer => equal!
	if(  this == schedule  ) {
		return true;
	}
	// unequal count => not equal
	const uint8 min_count = min( schedule->entries.get_count(), entries.get_count() );
	if(  min_count == 0  ) {
		return false;
	}
	// now we have to check all entries: So we add all stops to a vector we will iterate over
	vector_tpl<halthandle_t> halts;
	for(  uint8 idx = 0;  idx < this->entries.get_count();  idx++  ) {
		koord3d p = this->entries[idx].pos;
		halthandle_t halt = haltestelle_t::get_halt( p, player );
		if(  halt.is_bound()  ) {
			halts.insert_unique_ordered( halt, HaltIdOrdering() );
		}
	}
	vector_tpl<halthandle_t> other_halts;
	for(  uint8 idx = 0;  idx < schedule->entries.get_count();  idx++  ) {
		koord3d p = schedule->entries[idx].pos;
		halthandle_t halt = haltestelle_t::get_halt( p, player );
		if(  halt.is_bound()  ) {
			other_halts.insert_unique_ordered( halt, HaltIdOrdering() );
		}
	}
	// now compare them
	if(  other_halts.get_count() != halts.get_count()  ) {
		return false;
	}
	// number of unique halt similar => compare them now
	for(  uint32 idx = 0;  idx < halts.get_count();  idx++  ) {
		if(  halts[idx] != other_halts[idx]  ) {
			return false;
		}
	}
	return true;
}



void schedule_t::add_return_way()
{
	if(  entries.get_count()<127  &&  entries.get_count()>1  ) {
		for(  uint8 maxi=entries.get_count()-2;  maxi>0;  maxi--  ) {
			entries.append(entries[maxi]);
		}
	}
}



void schedule_t::sprintf_schedule( cbuffer_t &buf ) const
{
	uint32 s = current_stop + (flags<<8) + (max_speed<<16);
	buf.printf("%u|%d|", s, (int)get_type());
	FOR(minivec_tpl<schedule_entry_t>, const& i, entries) {
		buf.printf("%s,%i,%i,%i,%i,%i,%i|", i.pos.get_str(), (int)i.minimum_loading, (int)i.waiting_time_shift, i.get_stop_flags(), i.spacing, i.spacing_shift, i.delay_tolerance);
	}
}


bool schedule_t::sscanf_schedule( const char *ptr )
{
	const char *p = ptr;
	// first: clear current schedule
	while (!entries.empty()) {
		remove();
	}
	if ( p == NULL  ||  *p == 0) {
		// empty string
		return false;
	}
	//  first get current_stop pointer
	uint32 s = atoi( p );
	current_stop = s & 0xff;
	flags = ((s&0xff00) >> 8);
	max_speed = (s>>16);
	while(  *p  &&  *p!='|'  ) {
		p++;
	}
	if(  *p!='|'  ) {
		dbg->error( "schedule_t::sscanf_schedule()","incomplete entry termination!" );
		return false;
	}
	p++;
	//  then schedule type
	int type = atoi( p );
	//  .. check for correct type
	if(  type != (int)get_type()) {
		dbg->error( "schedule_t::sscanf_schedule()","schedule has wrong type (%d)! should have been %d.", type, get_type() );
		return false;
	}
	while(  *p  &&  *p!='|'  ) {
		p++;
	}
	if(  *p!='|'  ) {
		dbg->error( "schedule_t::sscanf_schedule()","incomplete entry termination!" );
		return false;
	}
	p++;
	// now scan the entries
	while(  *p>0  ) {
		sint16 values[9];
		for(  sint8 i=0;  i<9;  i++  ) {
			values[i] = atoi( p );
			while(  *p  &&  (*p!=','  &&  *p!='|')  ) {
				p++;
			}
			if(  i<8  &&  *p!=','  ) {
				dbg->error( "schedule_t::sscanf_schedule()","incomplete string!" );
				return false;
			}
			if(  i==8  &&  *p!='|'  ) {
				dbg->error( "schedule_t::sscanf_schedule()","incomplete entry termination!" );
				return false;
			}
			p++;
		}
		// ok, now we have a complete entry
		schedule_entry_t entry = schedule_entry_t(koord3d(values[0], values[1], values[2]), values[3], values[4], values[5]);
		entry.set_spacing(values[6], values[7], values[8]);
		entries.append(entry);
	}
	return true;
}

void construct_schedule_entry_attributes(cbuffer_t& buf, schedule_entry_t const& entry) {
	uint8 cnt = 1;
	char str[10];
	str[0] = '[';
	const uint8 flag = entry.get_stop_flags();
	if(  flag&schedule_entry_t::WAIT_FOR_COUPLING  ) {
		str[cnt] = 'W';
		cnt++;
	}
	if(  flag&schedule_entry_t::TRY_COUPLING  ) {
		str[cnt] = 'C';
		cnt++;
	}
	if(  flag&schedule_entry_t::NO_LOAD  ) {
		str[cnt] = 'L';
		cnt++;
	}
	if(  flag&schedule_entry_t::NO_UNLOAD  ) {
		str[cnt] = 'U';
		cnt++;
	}
	if(  flag&schedule_entry_t::UNLOAD_ALL  ) {
		str[cnt] = 'A';
		cnt++;
	}
	// there are at least one attributes.
	if(  cnt>1  ) {
		str[cnt] = ']';
		str[cnt+1] = ' ';
		str[cnt+2] = '\0';
		buf.append(str);
	}
}

void schedule_t::gimme_stop_name(cbuffer_t& buf, karte_t* welt, player_t const* const player_, schedule_entry_t const& entry, int const max_chars)
{
	const char *p;
	halthandle_t halt = haltestelle_t::get_halt(entry.pos, player_);
	if(halt.is_bound()) {
		construct_schedule_entry_attributes(buf, entry);
		if(  max_chars <= 0  ) {
			if(  entry.get_wait_for_time()  ) {
				buf.printf("%dT ", entry.spacing);
			}
			else if (  entry.minimum_loading != 0  ) {
				buf.printf("%d%% ", entry.minimum_loading);
			}
		}
		p = halt->get_name();
	}
	else {
		const grund_t* gr = welt->lookup(entry.pos);
		if(gr==NULL) {
			p = translator::translate("Invalid coordinate");
		}
		else if(gr->get_depot() != NULL) {
			p = translator::translate("Depot");
		}
		else {
			p = translator::translate("Wegpunkt");
		}
	}
	// finally append
	if(max_chars > 0  &&  strlen(p)>(unsigned)max_chars) {
		buf.printf("%.*s...", max_chars - 3, p);
	}
	else {
		buf.append(p);
	}
	// position (when no length restriction)
	if (max_chars <= 0) {
		buf.printf(" (%s)", entry.pos.get_str());
	}
}

schedule_entry_t const& schedule_t::get_next_entry() const {
	if(  entries.empty()  ) {
		return dummy_entry;
	} else {
		return entries[(current_stop+1)%entries.get_count()];
	}
}

void schedule_t::set_spacing_for_all(uint16 v) {
	for(uint8 i=0; i<entries.get_count(); i++) {
		entries[i].spacing = v;
	}
}

void schedule_t::set_spacing_shift_for_all(uint16 v) {
	for(uint8 i=0; i<entries.get_count(); i++) {
		entries[i].spacing_shift = v;
	}
}

void schedule_t::set_delay_tolerance_for_all(uint16 v) {
	for(uint8 i=0; i<entries.get_count(); i++) {
		entries[i].delay_tolerance = v;
	}
}

schedule_entry_t* schedule_t::access_corresponding_entry(schedule_t* other, uint8 n) {
	if(  n >= other->get_count()  ) {
		// out of range;
		dbg->error("schedule_t::access_corresponding_entry", "index %d is out of range %d", n, other->get_count());
		return NULL;
	}
	uint8 o_idx = n;
	// count the number of depot entries
	for(uint8 i=0; i<n; i++) {
		grund_t* gr = world()->lookup(other->entries[i].pos);
		if(  gr  &&  gr->get_depot()  ) {
			// this entry is a depot entry
			o_idx -= 1;
		}
	}
	uint8 h = 0;
	uint8 k = 0;
	while(  h<get_count()  ) {
		grund_t* gr = world()->lookup(entries[h].pos);
		if(  !gr  ||  !gr->get_depot()  ) {
			// this entry is not a depot entry
			if(  k==o_idx  ) {
				return &entries[h];
			}
			k++;
		}
		h++;
	}
	dbg->error("schedule_t::access_corresponding_entry", "corresponding entry not found.");
	return NULL;
}

uint8 schedule_t::get_current_stop_exluding_depot() const {
	uint8 idx = current_stop;
	// count the number of depot entries
	for(uint8 i=0; i<current_stop; i++) {
		grund_t* gr = world()->lookup(entries[i].pos);
		if(  gr  &&  gr->get_depot()  ) {
			// this entry is a depot entry
			idx -= 1;
		}
	}
	return idx;
}
