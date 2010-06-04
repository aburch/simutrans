#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "simtypes.h"
#include "simline.h"
#include "simhalt.h"
#include "player/simplay.h"
#include "vehicle/simvehikel.h"
#include "simconvoi.h"
#include "convoihandle_t.h"
#include "simworld.h"
#include "simlinemgmt.h"


uint8 simline_t::convoi_to_line_catgory[MAX_CONVOI_COST] = {
	LINE_CAPACITY, LINE_TRANSPORTED_GOODS, LINE_REVENUE, LINE_OPERATIONS, LINE_PROFIT, LINE_DISTANCE
};

karte_t *simline_t::welt=NULL;



simline_t::simline_t(karte_t* welt, spieler_t* sp)
{
	self = linehandle_t(this);
	sprintf( name, "(%i) %s", self.get_id(), translator::translate("Line") );
	init_financial_history();
	this->id = INVALID_LINE_ID;
	this->welt = welt;
	this->old_fpl = NULL;
	this->fpl = NULL;
	this->sp = sp;
	withdraw = false;
	state_color = COL_YELLOW;
}



void simline_t::set_line_id(uint32 id)
{
	this->id = id;
	sprintf( name, "(%i) %s", id, translator::translate("Line") );
}



simline_t::~simline_t()
{
	DBG_DEBUG("simline_t::~simline_t()", "deleting fpl=%p and old_fpl=%p", fpl, old_fpl);

	assert(count_convoys()==0);
	unregister_stops();

	delete fpl;
	delete old_fpl;
	self.detach();

	DBG_MESSAGE("simline_t::~simline_t()", "line %d (%p) destroyed", id, this);
}



void simline_t::add_convoy(convoihandle_t cnv)
{
	if (line_managed_convoys.empty()) {
		// first convoi -> ok, now we can announce this connection to the stations
		register_stops(fpl);
	}

	// first convoi may change line type
	if (type == trainline  &&  line_managed_convoys.empty() &&  cnv.is_bound()) {
		if(cnv->get_vehikel(0)) {
			// check, if needed to convert to tram line?
			if(cnv->get_vehikel(0)->get_besch()->get_waytype()==tram_wt) {
				type = simline_t::tramline;
			}
			// check, if needed to convert to monorail line?
			if(cnv->get_vehikel(0)->get_besch()->get_waytype()==monorail_wt) {
				type = simline_t::monorailline;
				// elevated monorail were saved with wrong coordinates for some versions.
				// We try to recover here
			}
		}
	}
	// only add convoy if not already member of line
	line_managed_convoys.append_unique(cnv);

	// what goods can this line transport?
	bool update_schedules = false;
	if(  cnv->get_state()!=convoi_t::INITIAL  ) {
		const minivec_tpl<uint8> &convoys_goods = cnv->get_goods_catg_index();
		for(  uint8 i = 0;  i < convoys_goods.get_count();  i++  ) {
			const uint8 catg_index = convoys_goods[i];
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



void simline_t::rdwr(loadsave_t *file)
{
	xml_tag_t s( file, "simline_t" );

	assert(fpl);

	file->rdwr_str(name, lengthof(name));
	if(file->get_version()<88003) {
		sint32 dummy=id;
		file->rdwr_long(dummy, " ");
		id = (uint16)dummy;
	}
	else {
		file->rdwr_short(id, " ");
	}
	fpl->rdwr(file);

	//financial history
	if(  file->get_version()<=102002  ) {
		for (int j = 0; j<6; j++) {
			for (int k = MAX_MONTHS-1; k>=0; k--) {
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
		for (int k = MAX_MONTHS-1; k>=0; k--) {
			financial_history[k][LINE_DISTANCE] = 0;
		}
	}
	else {
		for (int j = 0; j<MAX_LINE_COST; j++) {
			for (int k = MAX_MONTHS-1; k>=0; k--) {
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
	}

	if(file->get_version()>=102002) {
		file->rdwr_bool( withdraw, "" );
	}

	// otherwise inintialized to zero if loading ...
	financial_history[0][LINE_CONVOIS] = count_convoys();
}



void simline_t::laden_abschliessen()
{
	register_stops(fpl);
	recalc_status();
}



void simline_t::register_stops(schedule_t * fpl)
{
DBG_DEBUG("simline_t::register_stops()", "%d fpl entries in schedule %p", fpl->get_count(),fpl);
	for (int i = 0; i<fpl->get_count(); i++) {
		const halthandle_t halt = haltestelle_t::get_halt( welt, fpl->eintrag[i].pos, sp );
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
	for (int i = 0; i<fpl->get_count(); i++) {
		halthandle_t halt = haltestelle_t::get_halt( welt, fpl->eintrag[i].pos, sp );
		if(halt.is_bound()) {
			halt->remove_line(self);
		}
	}
}



void simline_t::renew_stops()
{
	if(  old_fpl  ) {
		unregister_stops( old_fpl );
	}
	register_stops( fpl );
	DBG_DEBUG("simline_t::renew_stops()", "Line id=%d, name='%s'", id, name);
}



void simline_t::new_month()
{
	recalc_status();
	for (int j = 0; j<MAX_LINE_COST; j++) {
		for (int k = MAX_MONTHS-1; k>0; k--) {
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}
	financial_history[0][LINE_CONVOIS] = count_convoys();
}



/*
 * called from line_management_gui.cc to prepare line for a change of its schedule
 */
void simline_t::prepare_for_update()
{
	DBG_DEBUG("simline_t::prepare_for_update()", "line %d (%p)", id, this);
	delete old_fpl;
	old_fpl = fpl->copy();
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
	} else if((financial_history[0][LINE_OPERATIONS]|financial_history[1][LINE_OPERATIONS])==0) {
		// nothing moved
		state_color = COL_YELLOW;
	}
	else if(welt->use_timeline()) {
		// convois has obsolete vehicles?
		bool has_obsolete = false;
		for(unsigned i=0;  !has_obsolete  &&  i<line_managed_convoys.get_count();  i++ ) {
			has_obsolete = line_managed_convoys[i]->has_obsolete_vehicles();
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
	for(  uint i=0;  i<goods_catg_index.get_count();  i++  ) {
		old_goods_catg_index.append( goods_catg_index[i] );
	}
	goods_catg_index.clear();
	withdraw = line_managed_convoys.get_count()>0;
	// then recreate current
	for(unsigned i=0;  i<line_managed_convoys.get_count();  i++ ) {
		// what goods can this line transport?
		// const convoihandle_t cnv = line_managed_convoys[i];
		const convoi_t *cnv = line_managed_convoys[i].get_rep();
		withdraw &= cnv->get_withdraw();

		const minivec_tpl<uint8> &convoys_goods = cnv->get_goods_catg_index();
		for(  uint8 i = 0;  i < convoys_goods.get_count();  i++  ) {
			const uint8 catg_index = convoys_goods[i];
			goods_catg_index.append_unique( catg_index, 1 );
		}
	}
	// if different => schedule need recalculation
	if(  goods_catg_index.get_count()!=old_goods_catg_index.get_count()  ) {
		// surely changed
		welt->set_schedule_counter();
	}
	else {
		// maybe changed => must test all entries
		for(  uint i=0;  i<goods_catg_index.get_count();  i++  ) {
			if(  !old_goods_catg_index.is_contained(goods_catg_index[i])  ) {
				// different => recalc
				welt->set_schedule_counter();
				break;
			}
		}
	}
}



void simline_t::set_withdraw( bool yes_no )
{
	withdraw = yes_no  &&  (line_managed_convoys.get_count()>0);
	// convois in depots will be immeadiately destroyed, thus we go backwards
	for( sint32 i=line_managed_convoys.get_count()-1;  i>=0;  i--  ) {
		line_managed_convoys[i]->set_no_load(yes_no);	// must be first, since set withdraw might destroy convoi if in depot!
		line_managed_convoys[i]->set_withdraw(yes_no);
	}
}
