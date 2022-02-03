/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simtypes.h"
#include "simline.h"
#include "simhalt.h"
#include "world/simworld.h"

#include "utils/simstring.h"
#include "dataobj/schedule.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "gui/gui_theme.h"
#include "player/simplay.h"
#include "player/finance.h" // convert_money
#include "vehicle/vehicle.h"
#include "simconvoi.h"
#include "convoihandle.h"
#include "simlinemgmt.h"
#include "gui/simwin.h"
#include "gui/gui_frame.h"


uint8 convoi_to_line_catgory_[convoi_t::MAX_CONVOI_COST] = {
	LINE_CAPACITY, LINE_TRANSPORTED_GOODS, LINE_REVENUE, LINE_OPERATIONS, LINE_PROFIT, LINE_DISTANCE, LINE_MAXSPEED, LINE_WAYTOLL
};


uint8 simline_t::convoi_to_line_catgory(uint8 cnv_cost)
{
	assert(cnv_cost < convoi_t::MAX_CONVOI_COST);
	return convoi_to_line_catgory_[cnv_cost];
}


karte_ptr_t simline_t::welt;


simline_t::simline_t(player_t* player, linetype type)
{
	self = linehandle_t(this);
	char printname[128];
	sprintf(printname, "(%i) %s", self.get_id(), translator::translate("Line", welt->get_settings().get_name_language_id()));
	name = printname;

	init_financial_history();
	this->type = type;
	this->schedule = NULL;
	this->player = player;
	withdraw = false;
	state_color = SYSCOL_TEXT;
	create_schedule();
}


simline_t::simline_t(player_t* player, linetype type, loadsave_t *file)
{
	// id will be read and assigned during rdwr
	self = linehandle_t();
	this->type = type;
	this->schedule = NULL;
	this->player = player;
	withdraw = false;
	create_schedule();
	rdwr(file);
	// now self has the right id but the this-pointer is not assigned to the quickstone handle yet
	// do this explicitly
	// some savegames have line_id=0, resolve that in finish_rd
	if (self.get_id()!=0) {
		self = linehandle_t(this, self.get_id());
	}
}


simline_t::~simline_t()
{
	DBG_DEBUG("simline_t::~simline_t()", "deleting schedule=%p", schedule);

	assert(count_convoys()==0);
	unregister_stops();

	delete schedule;
	self.detach();
	DBG_MESSAGE("simline_t::~simline_t()", "line %d (%p) destroyed", self.get_id(), this);
}


simline_t::linetype simline_t::waytype_to_linetype(const waytype_t wt)
{
	switch (wt) {
		case road_wt: return simline_t::truckline;
		case track_wt: return simline_t::trainline;
		case water_wt: return simline_t::shipline;
		case monorail_wt: return simline_t::monorailline;
		case maglev_wt: return simline_t::maglevline;
		case tram_wt: return simline_t::tramline;
		case narrowgauge_wt: return simline_t::narrowgaugeline;
		case air_wt: return simline_t::airline;
		default: return simline_t::line;
	}
}


const char *simline_t::get_linetype_name(const simline_t::linetype lt)
{
	static const char *lt2name[MAX_LINE_TYPE] = {"All", "Truck", "Train", "Ship", "Air", "Monorail", "Tram", "Maglev", "Narrowgauge" };
	return translator::translate( lt2name[lt] );
}


waytype_t simline_t::linetype_to_waytype(const linetype lt)
{
	static const waytype_t wt2lt[MAX_LINE_TYPE] = { invalid_wt, road_wt, track_wt, water_wt, air_wt, monorail_wt, tram_wt, maglev_wt, narrowgauge_wt };
	return wt2lt[lt];
}


void simline_t::set_schedule(schedule_t* schedule)
{
	if (this->schedule) {
		unregister_stops();
		delete this->schedule;
	}
	this->schedule = schedule;
}


void simline_t::set_name(const char *new_name)
{
	name = new_name;

	/// Update window title if window is open
	gui_frame_t *line_info = win_get_magic((ptrdiff_t)self.get_rep());

	if (line_info) {
		line_info->set_name(name);
	}
}


void simline_t::create_schedule()
{
	switch(type) {
		case simline_t::truckline:       set_schedule(new truck_schedule_t()); break;
		case simline_t::trainline:       set_schedule(new train_schedule_t()); break;
		case simline_t::shipline:        set_schedule(new ship_schedule_t()); break;
		case simline_t::airline:         set_schedule(new airplane_schedule_t()); break;
		case simline_t::monorailline:    set_schedule(new monorail_schedule_t()); break;
		case simline_t::tramline:        set_schedule(new tram_schedule_t()); break;
		case simline_t::maglevline:      set_schedule(new maglev_schedule_t()); break;
		case simline_t::narrowgaugeline: set_schedule(new narrowgauge_schedule_t()); break;
		default:
			dbg->fatal( "simline_t::create_schedule()", "Cannot create default schedule!" );
	}
}


void simline_t::add_convoy(convoihandle_t cnv)
{
	if (line_managed_convoys.empty()  &&  self.is_bound()) {
		// first convoi -> ok, now we can announce this connection to the stations
		// unbound self can happen during loading if this line had line_id=0
		register_stops(schedule);
	}

	// first convoi may change line type
	if (type == trainline  &&  line_managed_convoys.empty() &&  cnv.is_bound()) {
		// check, if needed to convert to tram/monorail line
		if (vehicle_t const* const v = cnv->front()) {
			switch (v->get_desc()->get_waytype()) {
				case tram_wt:     type = simline_t::tramline;     break;
				// elevated monorail were saved with wrong coordinates for some versions.
				// We try to recover here
				case monorail_wt: type = simline_t::monorailline; break;
				default:          break;
			}
		}
	}
	// only add convoy if not already member of line
	line_managed_convoys.append_unique(cnv);

	// what goods can this line transport?
	bool update_schedules = false;
	if(  cnv->get_state()!=convoi_t::INITIAL  ) {
		FOR(minivec_tpl<uint8>, const catg_index, cnv->get_goods_catg_index()) {
			if(  !goods_catg_index.is_contained( catg_index )  ) {
				goods_catg_index.append( catg_index, 1 );
				update_schedules = true;
			}
		}
	}

	// will not hurt ...
	financial_history[0][LINE_CONVOIS] = count_convoys();
	recalc_status();

	// do we need to tell the world about our new schedule?
	if(  update_schedules  ) {
		welt->set_schedule_counter();
	}
}


void simline_t::remove_convoy(convoihandle_t cnv)
{
	if(line_managed_convoys.is_contained(cnv)) {
		line_managed_convoys.remove(cnv);
		recalc_catg_index();
		financial_history[0][LINE_CONVOIS] = count_convoys();
		recalc_status();
	}
	if(line_managed_convoys.empty()) {
		unregister_stops();
	}
}


// invalid line id prior to 110.0
#define INVALID_LINE_ID_OLD ((uint16)(-1))
// invalid line id from 110.0 on
#define INVALID_LINE_ID ((uint16)(0))

void simline_t::rdwr_linehandle_t(loadsave_t *file, linehandle_t &line)
{
	uint16 id;
	if (file->is_saving()) {
		id = line.is_bound() ? line.get_id() :
			 (file->is_version_less(110, 0)  ? INVALID_LINE_ID_OLD : INVALID_LINE_ID);
	}
	else {
		// to avoid undefined errors during loading
		id = 0;
	}

	if(file->is_version_less(88, 3)) {
		sint32 dummy=id;
		file->rdwr_long(dummy);
		id = (uint16)dummy;
	}
	else {
		file->rdwr_short(id);
	}
	if (file->is_loading()) {
		// invalid line_id's: 0 and 65535
		if (id == INVALID_LINE_ID_OLD) {
			id = 0;
		}
		line.set_id(id);
	}
}


void simline_t::rdwr(loadsave_t *file)
{
	xml_tag_t s( file, "simline_t" );

	assert(schedule);

	file->rdwr_str(name);

	rdwr_linehandle_t(file, self);

	schedule->rdwr(file);

	//financial history
	if(  file->is_version_less(102, 3)  ) {
		for (int j = 0; j<6; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][LINE_DISTANCE] = 0;
			financial_history[k][LINE_MAXSPEED] = 0;
			financial_history[k][LINE_WAYTOLL] = 0;
		}
	}
	else if(  file->is_version_less(111, 1)  ) {
		for (int j = 0; j<7; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][LINE_MAXSPEED] = 0;
			financial_history[k][LINE_WAYTOLL] = 0;
		}
	}
	else if(  file->is_version_less(112, 8)  ) {
		for (int j = 0; j<8; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][LINE_WAYTOLL] = 0;
		}
	}
	else {
		for (int j = 0; j<MAX_LINE_COST; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
	}

	if(file->is_version_atleast(102, 2)) {
		file->rdwr_bool(withdraw);
	}

	// otherwise initialized to zero if loading ...
	financial_history[0][LINE_CONVOIS] = count_convoys();
}



void simline_t::finish_rd()
{
	if(  !self.is_bound()  ) {
		// get correct handle
		self = player->simlinemgmt.get_line_with_id_zero();
		assert( self.get_rep() == this );
		DBG_MESSAGE("simline_t::finish_rd", "assigned id=%d to line %s", self.get_id(), get_name());
	}
	if (!line_managed_convoys.empty()) {
		register_stops(schedule);
	}
	recalc_status();
}



void simline_t::register_stops(schedule_t * schedule)
{
DBG_DEBUG("simline_t::register_stops()", "%d schedule entries in schedule %p", schedule->get_count(),schedule);
	FOR(minivec_tpl<schedule_entry_t>, const& i, schedule->entries) {
		halthandle_t const halt = haltestelle_t::get_halt(i.pos, player);
		if(halt.is_bound()) {
//DBG_DEBUG("simline_t::register_stops()", "halt not null");
			halt->add_line(self);
		}
		else {
DBG_DEBUG("simline_t::register_stops()", "halt null");
		}
	}
}



void simline_t::unregister_stops()
{
	unregister_stops(schedule);
}


void simline_t::unregister_stops(schedule_t * schedule)
{
	FOR(minivec_tpl<schedule_entry_t>, const& i, schedule->entries) {
		halthandle_t const halt = haltestelle_t::get_halt(i.pos, player);
		if(halt.is_bound()) {
			halt->remove_line(self);
		}
	}
}


void simline_t::renew_stops()
{
	if (!line_managed_convoys.empty()) {
		register_stops( schedule );
		DBG_DEBUG("simline_t::renew_stops()", "Line id=%d, name='%s'", self.get_id(), name.c_str());
	}
}


void simline_t::check_freight()
{
	FOR(vector_tpl<convoihandle_t>, const i, line_managed_convoys) {
		i->check_freight();
	}
}


void simline_t::new_month()
{
	recalc_status();
	// then calculate maxspeed
	sint64 line_max_speed = 0, line_max_speed_count = 0;
	FOR(vector_tpl<convoihandle_t>, const i, line_managed_convoys) {
		if (!i->in_depot()) {
			// since convoi stepped first, our history is in month 1 ...
			line_max_speed += i->get_finance_history(1, convoi_t::CONVOI_MAXSPEED);
			line_max_speed_count ++;
		}
	}
	// to avoid div by zero
	if(  line_max_speed_count  ) {
		line_max_speed /= line_max_speed_count;
	}
	financial_history[0][LINE_MAXSPEED] = line_max_speed;
	// now roll history
	for (int j = 0; j<MAX_LINE_COST; j++) {
		for (int k = MAX_MONTHS-1; k>0; k--) {
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}
	financial_history[0][LINE_CONVOIS] = count_convoys();
}


void simline_t::init_financial_history()
{
	MEMZERO(financial_history);
}



/*
 * the current state saved as color
 * Meanings are BLACK (ok), WHITE (no convois), YELLOW (no vehicle moved), RED (last month income minus), BLUE (at least one convoi vehicle is obsolete)
 */
void simline_t::recalc_status()
{
	if(financial_history[0][LINE_CONVOIS]==0) {
		// no convois assigned to this line
		state_color = SYSCOL_EMPTY;
		withdraw = false;
	}
	else if(financial_history[0][LINE_PROFIT]<0) {
		// ok, not performing best
		state_color = MONEY_MINUS;
	}
	else if((financial_history[0][LINE_OPERATIONS]|financial_history[1][LINE_OPERATIONS])==0) {
		// nothing moved
		state_color = SYSCOL_TEXT_UNUSED;
	}
	else if(welt->use_timeline()) {
		// convois has obsolete vehicles?
		bool has_obsolete = false;
		FOR(vector_tpl<convoihandle_t>, const i, line_managed_convoys) {
			has_obsolete = i->has_obsolete_vehicles();
			if (has_obsolete) break;
		}
		// now we have to set it
		state_color = has_obsolete ? SYSCOL_OBSOLETE : SYSCOL_TEXT;
	}
	else {
		// normal state
		state_color = SYSCOL_TEXT;
	}
}



// recalc what good this line is moving
void simline_t::recalc_catg_index()
{
	// first copy old
	minivec_tpl<uint8> old_goods_catg_index(goods_catg_index.get_count());
	FOR(minivec_tpl<uint8>, const i, goods_catg_index) {
		old_goods_catg_index.append(i);
	}
	goods_catg_index.clear();
	withdraw = !line_managed_convoys.empty();
	// then recreate current
	FOR(vector_tpl<convoihandle_t>, const i, line_managed_convoys) {
		// what goods can this line transport?
		convoi_t const& cnv = *i;
		withdraw &= cnv.get_withdraw();

		FOR(minivec_tpl<uint8>, const catg_index, cnv.get_goods_catg_index()) {
			goods_catg_index.append_unique( catg_index );
		}
	}
	// if different => schedule need recalculation
	if(  goods_catg_index.get_count()!=old_goods_catg_index.get_count()  ) {
		// surely changed
		welt->set_schedule_counter();
	}
	else {
		// maybe changed => must test all entries
		FOR(minivec_tpl<uint8>, const i, goods_catg_index) {
			if (!old_goods_catg_index.is_contained(i)) {
				// different => recalc
				welt->set_schedule_counter();
				break;
			}
		}
	}
}



void simline_t::set_withdraw( bool yes_no )
{
	withdraw = yes_no && !line_managed_convoys.empty();
	// convois in depots will be immediately destroyed, thus we go backwards
	for (size_t i = line_managed_convoys.get_count(); i-- != 0;) {
		line_managed_convoys[i]->set_no_load(yes_no); // must be first, since set withdraw might destroy convoi if in depot!
		line_managed_convoys[i]->set_withdraw(yes_no);
	}
}


sint64 simline_t::get_stat_converted(int month, int cost_type) const
{
	sint64 value = financial_history[month][cost_type];
	switch(cost_type) {
		case LINE_REVENUE:
		case LINE_OPERATIONS:
		case LINE_PROFIT:
		case LINE_WAYTOLL:
			value = convert_money(value);
			break;
		default: ;
	}
	return value;
}
