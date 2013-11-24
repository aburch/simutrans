
#include "simtypes.h"
#include "simline.h"
#include "simhalt.h"
#include "simworld.h"

#include "utils/simstring.h"
#include "dataobj/fahrplan.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "player/simplay.h"
#include "vehicle/simvehikel.h"
#include "simconvoi.h"
#include "convoihandle_t.h"
#include "simlinemgmt.h"

uint8 convoi_to_line_catgory_[convoi_t::MAX_CONVOI_COST] = {
	LINE_CAPACITY, LINE_TRANSPORTED_GOODS, LINE_REVENUE, LINE_OPERATIONS, LINE_PROFIT, LINE_DISTANCE, LINE_MAXSPEED, LINE_WAYTOLL
};

uint8 simline_t::convoi_to_line_catgory(uint8 cnv_cost)
{
	assert(cnv_cost < convoi_t::MAX_CONVOI_COST);
	return convoi_to_line_catgory_[cnv_cost];
}


karte_ptr_t simline_t::welt;


simline_t::simline_t(spieler_t* sp, linetype type)
{
	self = linehandle_t(this);
	char printname[128];
	sprintf(printname, "(%i) %s", self.get_id(), translator::translate("Line", welt->get_settings().get_name_language_id()));
	name = printname;

	init_financial_history();
	this->type = type;
	this->fpl = NULL;
	this->sp = sp;
	withdraw = false;
	state_color = COL_WHITE;
	create_schedule();
}


simline_t::simline_t(spieler_t* sp, linetype type, loadsave_t *file)
{
	// id will be read and assigned during rdwr
	self = linehandle_t();
	this->type = type;
	this->fpl = NULL;
	this->sp = sp;
	withdraw = false;
	create_schedule();
	rdwr(file);
	// now self has the right id but the this-pointer is not assigned to the quickstone handle yet
	// do this explicitly
	// some savegames have line_id=0, resolve that in laden_abschliessen
	if (self.get_id()!=0) {
		self = linehandle_t(this, self.get_id());
	}
}


simline_t::~simline_t()
{
	DBG_DEBUG("simline_t::~simline_t()", "deleting fpl=%p", fpl);

	assert(count_convoys()==0);
	unregister_stops();

	delete fpl;
	self.detach();
	DBG_MESSAGE("simline_t::~simline_t()", "line %d (%p) destroyed", self.get_id(), this);
}


void simline_t::set_schedule(schedule_t* fpl)
{
	if (this->fpl) {
		unregister_stops();
		delete this->fpl;
	}
	this->fpl = fpl;
}


void simline_t::create_schedule()
{
	switch(type) {
		case simline_t::truckline:       set_schedule(new autofahrplan_t()); break;
		case simline_t::trainline:       set_schedule(new zugfahrplan_t()); break;
		case simline_t::shipline:        set_schedule(new schifffahrplan_t()); break;
		case simline_t::airline:         set_schedule(new airfahrplan_t()); break;
		case simline_t::monorailline:    set_schedule(new monorailfahrplan_t()); break;
		case simline_t::tramline:        set_schedule(new tramfahrplan_t()); break;
		case simline_t::maglevline:      set_schedule(new maglevfahrplan_t()); break;
		case simline_t::narrowgaugeline: set_schedule(new narrowgaugefahrplan_t()); break;
		default:
			dbg->fatal( "simline_t::create_schedule()", "Cannot create default schedule!" );
	}
}


void simline_t::add_convoy(convoihandle_t cnv)
{
	if (line_managed_convoys.empty()  &&  self.is_bound()) {
		// first convoi -> ok, now we can announce this connection to the stations
		// unbound self can happen during loading if this line had line_id=0
		register_stops(fpl);
	}

	// first convoi may change line type
	if (type == trainline  &&  line_managed_convoys.empty() &&  cnv.is_bound()) {
		// check, if needed to convert to tram/monorail line
		if (vehikel_t const* const v = cnv->front()) {
			switch (v->get_besch()->get_waytype()) {
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
		id = line.is_bound() ? line.get_id(): (file->get_version() < 110000  ? INVALID_LINE_ID_OLD : INVALID_LINE_ID);
	}
	else {
		// to avoid undefined errors during loading
		id = 0;
	}

	if(file->get_version()<88003) {
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

	assert(fpl);

	file->rdwr_str(name);

	rdwr_linehandle_t(file, self);

	fpl->rdwr(file);

	//financial history
	if(  file->get_version()<=102002  ) {
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
	else if(  file->get_version()<111001  ) {
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
	else if(  file->get_version()<112008  ) {
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

	if(file->get_version()>=102002) {
		file->rdwr_bool(withdraw);
	}

	// otherwise inintialized to zero if loading ...
	financial_history[0][LINE_CONVOIS] = count_convoys();
}



void simline_t::laden_abschliessen()
{
	if(  !self.is_bound()  ) {
		// get correct handle
		self = sp->simlinemgmt.get_line_with_id_zero();
		assert( self.get_rep() == this );
		DBG_MESSAGE("simline_t::laden_abschliessen", "assigned id=%d to line %s", self.get_id(), get_name());
	}
	if (!line_managed_convoys.empty()) {
		register_stops(fpl);
	}
	recalc_status();
}



void simline_t::register_stops(schedule_t * fpl)
{
DBG_DEBUG("simline_t::register_stops()", "%d fpl entries in schedule %p", fpl->get_count(),fpl);
	FOR(minivec_tpl<linieneintrag_t>, const& i, fpl->eintrag) {
		halthandle_t const halt = haltestelle_t::get_halt(i.pos, sp);
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
	unregister_stops(fpl);
}


void simline_t::unregister_stops(schedule_t * fpl)
{
	FOR(minivec_tpl<linieneintrag_t>, const& i, fpl->eintrag) {
		halthandle_t const halt = haltestelle_t::get_halt(i.pos, sp);
		if(halt.is_bound()) {
			halt->remove_line(self);
		}
	}
}


void simline_t::renew_stops()
{
	if (!line_managed_convoys.empty()) {
		register_stops( fpl );
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
		// noconvois assigned to this line
		state_color = COL_WHITE;
		withdraw = false;
	}
	else if(financial_history[0][LINE_PROFIT]<0) {
		// ok, not performing best
		state_color = COL_RED;
	}
	else if((financial_history[0][LINE_OPERATIONS]|financial_history[1][LINE_OPERATIONS])==0) {
		// nothing moved
		state_color = COL_YELLOW;
	}
	else if(welt->use_timeline()) {
		// convois has obsolete vehicles?
		bool has_obsolete = false;
		FOR(vector_tpl<convoihandle_t>, const i, line_managed_convoys) {
			has_obsolete = i->has_obsolete_vehicles();
			if (has_obsolete) break;
		}
		// now we have to set it
		state_color = has_obsolete ? COL_DARK_BLUE : COL_BLACK;
	}
	else {
		// normal state
		state_color = COL_BLACK;
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
	// convois in depots will be immeadiately destroyed, thus we go backwards
	for (size_t i = line_managed_convoys.get_count(); i-- != 0;) {
		line_managed_convoys[i]->set_no_load(yes_no);	// must be first, since set withdraw might destroy convoi if in depot!
		line_managed_convoys[i]->set_withdraw(yes_no);
	}
}
