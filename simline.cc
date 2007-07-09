#include "dataobj/translator.h"
#include "simtypes.h"
#include "simline.h"
#include "simhalt.h"
#include "simplay.h"
#include "simvehikel.h"
#include "simconvoi.h"
#include "convoihandle_t.h"
#include "simworld.h"
#include "simlinemgmt.h"


uint8 simline_t::convoi_to_line_catgory[MAX_CONVOI_COST]={LINE_CAPACITY, LINE_TRANSPORTED_GOODS, LINE_REVENUE, LINE_OPERATIONS, LINE_PROFIT };

karte_t *simline_t::welt=NULL;



simline_t::simline_t(simlinemgmt_t* simlinemgmt, fahrplan_t* fpl)
{
	welt = simlinemgmt->get_welt();
	self = linehandle_t(this);
	init_financial_history();
	const int i = simlinemgmt->get_unique_line_id();
	memset(this->name, 0, 128);
	sprintf(this->name, "%s (%02d)", translator::translate("Line"), i);
	this->fpl = fpl;
	this->old_fpl = new fahrplan_t(fpl);
	this->id = i;
	this->state_color = COL_WHITE;
	type = simline_t::line;
	simlinemgmt->add_line(self);
DBG_MESSAGE("simline_t::simline_t(karte_t,simlinemgmt,fahrplan_t)","create with %d (unique %d)",self.get_id(),get_line_id());
}



simline_t::simline_t(karte_t* welt, loadsave_t* file)
{
	self = linehandle_t(this);
	init_financial_history();
	rdwr(file);
DBG_MESSAGE("simline_t::simline_t(karte_t,simlinemgmt,loadsave_t)","load line id=%d",id);
	this->welt = welt;
	this->old_fpl = new fahrplan_t(fpl);
}



simline_t::~simline_t()
{
	int count = count_convoys() - 1;
	for (int i = count; i>=0; i--)
	{
		DBG_DEBUG("simline_t::~simline_t()", "convoi '%d' removed", i);
		DBG_DEBUG("simline_t::~simline_t()", "convoi '%d'->fpl=%p", i, get_convoy(i)->gib_fahrplan());

		// Hajo: take care - this call will do "remove_convoi()"
		// on our list!
		get_convoy(i)->unset_line();
	}
	unregister_stops();

	DBG_DEBUG("simline_t::~simline_t()", "deleting fpl=%p and old_fpl=%p", fpl, old_fpl);

	delete (fpl);
	delete (old_fpl);

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
		if(cnv->gib_vehikel(0)) {
			// check, if needed to convert to tram line?
			if(cnv->gib_vehikel(0)->gib_besch()->get_waytype()==tram_wt) {
				type = simline_t::tramline;
			}
			// check, if needed to convert to monorail line?
			if(cnv->gib_vehikel(0)->gib_besch()->get_waytype()==monorail_wt) {
				type = simline_t::monorailline;
				// elevated monorail were saved with wrong coordinates for some versions.
				// We try to recover here
			}
		}
	}
	// only add convoy if not allready member of line
	line_managed_convoys.append_unique(cnv,16);

	// what goods can this line transport?
	for(int i=0;  i<cnv->gib_vehikel_anzahl();  i++  ) {
		const ware_besch_t *ware=cnv->gib_vehikel(i)->gib_fracht_typ();
		if(ware!=warenbauer_t::nichts) {
			goods_catg_index.append_unique( ware->gib_catg_index(), 1 );
		}
	}

	// will not hurt ...
	financial_history[0][LINE_CONVOIS] = count_convoys();
	if(state_color==COL_BLACK  &&  cnv->has_obsolete_vehicles()) {
		state_color = COL_DARK_BLUE;
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
	if (line_managed_convoys.empty()) {
		unregister_stops();
	}
}



void simline_t::rdwr(loadsave_t *file)
{
	// only create a new fahrplan if we are loading a savegame!
	if (file->is_loading()) {
		fpl = new fahrplan_t();
	}
	else {
		file->rdwr_enum(type, "\n");
	}

	file->rdwr_str(name, sizeof(name));
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
	for (int j = 0; j<MAX_LINE_COST; j++) {
		for (int k = MAX_MONTHS-1; k>=0; k--) {
			file->rdwr_longlong(financial_history[k][j], " ");
		}
	}
	// otherwise inintialized to zero if loading ...
	financial_history[0][LINE_CONVOIS] = count_convoys();
}



void simline_t::laden_abschliessen()
{
	register_stops(fpl);
	recalc_status();
}



void simline_t::register_stops(fahrplan_t * fpl)
{
DBG_DEBUG("simline_t::register_stops()", "%d fpl entries in schedule %p", fpl->maxi(),fpl);
	for (int i = 0; i<fpl->maxi(); i++) {
		const halthandle_t halt = haltestelle_t::gib_halt(welt, fpl->eintrag[i].pos.gib_2d());
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



void simline_t::unregister_stops(fahrplan_t * fpl)
{
	for (int i = 0; i<fpl->maxi(); i++) {
		halthandle_t halt = haltestelle_t::gib_halt(welt, fpl->eintrag[i].pos.gib_2d());
		if (halt.is_bound()) {
			halt->remove_line(self);
		}
	}
}



void simline_t::renew_stops()
{
	unregister_stops(this->old_fpl);
	register_stops(this->fpl);
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
	delete (old_fpl);
	this->old_fpl = new fahrplan_t(fpl);
}



void simline_t::init_financial_history()
{
	memset( financial_history, 0, sizeof(financial_history) );
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
	goods_catg_index.clear();
	for(unsigned i=0;  i<line_managed_convoys.get_count();  i++ ) {
		// what goods can this line transport?
		const convoihandle_t cnv = line_managed_convoys[i];
		for(int i=0;  i<cnv->gib_vehikel_anzahl();  i++  ) {
			const ware_besch_t *ware=cnv->gib_vehikel(i)->gib_fracht_typ();
			if(ware!=warenbauer_t::nichts) {
				goods_catg_index.append_unique( ware->gib_catg_index(), 1 );
			}
		}
	}
}
