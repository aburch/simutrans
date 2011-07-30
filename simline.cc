#include "utils/simstring.h"
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

uint8 simline_t::convoi_to_line_catgory[MAX_CONVOI_COST]={LINE_CAPACITY, LINE_TRANSPORTED_GOODS, LINE_AVERAGE_SPEED, LINE_COMFORT, LINE_REVENUE, LINE_OPERATIONS, LINE_PROFIT, LINE_DISTANCE, LINE_REFUNDS };

karte_t *simline_t::welt=NULL;

simline_t::simline_t(karte_t* welt, spieler_t* sp, linetype type)
{
	self = linehandle_t(this);
	char printname[128];
	sprintf(printname, "(%i) %s", self.get_id(), translator::translate("Line", welt->get_settings().get_name_language_id()));
	name = printname;

	init_financial_history();
	this->type = type;
	this->welt = welt;
	this->fpl = NULL;
	this->sp = sp;
	withdraw = false;
	state_color = COL_YELLOW;

	for(uint8 i = 0; i < MAX_LINE_COST; i ++)
	{	
		rolling_average[i] = 0;
		rolling_average_count[i] = 0;
	}
	start_reversed = false;
	livery_scheme_index = 0;

	create_schedule();

	average_journey_times = new koordhashtable_tpl<id_pair, average_tpl<uint16> >;
}


simline_t::simline_t(karte_t* welt, spieler_t* sp, linetype type, loadsave_t *file)
{
	// id will be read and assigned during rdwr
	self = linehandle_t();
	this->type = type;
	this->welt = welt;
	this->fpl = NULL;
	this->sp = sp;
	create_schedule();
	average_journey_times = new koordhashtable_tpl<id_pair, average_tpl<uint16> >;
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

	delete average_journey_times;
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


void simline_t::add_convoy(convoihandle_t cnv, bool from_loading)
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
		/*
		// already on the road => need to add them
		for(  uint8 i=0;  i<cnv->get_vehikel_anzahl();  i++  ) {
			// Only consider vehicles that really transport something
			// this helps against routing errors through passenger
			// trains pulling only freight wagons
			if(  cnv->get_vehikel(i)->get_fracht_max() == 0  ) {
				continue;
			}
			const ware_besch_t *ware=cnv->get_vehikel(i)->get_fracht_typ();
			if(  ware!=warenbauer_t::nichts  &&  !goods_catg_index.is_contained(ware->get_catg_index())  ) {
				goods_catg_index.append( ware->get_catg_index(), 1 );
				update_schedules = true;
			}
		}
		*/

		// Added by : Knightly
		const minivec_tpl<uint8> &categories = cnv->get_goods_catg_index();
		const uint8 catg_count = categories.get_count();
		for (uint8 i = 0; i < catg_count; i++)
		{
			if (!goods_catg_index.is_contained(categories[i]))
			{
				goods_catg_index.append(categories[i], 1);
				update_schedules = true;
			}
		}
	}

	// will not hurt ...
	financial_history[0][LINE_CONVOIS] = count_convoys();
	recalc_status();

	// do we need to tell the world about our new schedule?
	if(  update_schedules  ) {

		// Added by : Knightly
		haltestelle_t::refresh_routing(fpl, goods_catg_index, sp);
	}

	// if the schedule is flagged as bidirectional, set the initial convoy direction
	if( fpl->is_bidirectional() && !from_loading ) {
		cnv->set_reverse_schedule(start_reversed);
		start_reversed = !start_reversed;
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

	if(  file->is_loading()  ) {
		char name[1024];
		file->rdwr_str(name, lengthof(name));
		this->name = name;
	}
	else {
		char name[1024];
		tstrncpy( name, this->name.c_str(), lengthof(name) );
		file->rdwr_str(name, lengthof(name));
	}

	rdwr_linehandle_t(file, self);

	fpl->rdwr(file);

	//financial history

	if(file->get_version() < 102002 || (file->get_version() < 103000 && file->get_experimental_version() < 7))
	{
		for (int j = 0; j<LINE_DISTANCE; j++) 
		{
			for (int k = MAX_MONTHS-1; k>=0; k--) 
			{
				if(((j == LINE_AVERAGE_SPEED || j == LINE_COMFORT) && file->get_experimental_version() <= 1) || (j == LINE_REFUNDS && file->get_experimental_version() < 8))
				{
					// Versions of Experimental saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values. Likewise, versions of Experimental < 8
					// did not store refund information.
					if(file->is_loading())
					{
						financial_history[k][j] = 0;
					}
					continue;
				}
				file->rdwr_longlong(financial_history[k][j]);

			}
		}
		for (int k = MAX_MONTHS-1; k>=0; k--) 
		{
			financial_history[k][LINE_DISTANCE] = 0;
		}
	}
	else 
	{
		for (int j = 0; j<MAX_LINE_COST; j++) 
		{
			for (int k = MAX_MONTHS-1; k>=0; k--)
			{
				if(((j == LINE_AVERAGE_SPEED || j == LINE_COMFORT) && file->get_experimental_version() <= 1) || (j == LINE_REFUNDS && file->get_experimental_version() < 8))
				{
					// Versions of Experimental saves with 1 and below
					// did not have settings for average speed or comfort.
					// Thus, this value must be skipped properly to
					// assign the values. Likewise, versions of Experimental < 8
					// did not store refund information.
					if(file->is_loading())
					{
						financial_history[k][j] = 0;
					}
					continue;
				}
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
	}

	if(file->get_version()>=102002) {
		file->rdwr_bool(withdraw);
	}

	if(file->get_experimental_version() >= 9) 
	{
		file->rdwr_bool( start_reversed);
	}

	// otherwise inintialized to zero if loading ...
	financial_history[0][LINE_CONVOIS] = count_convoys();

	if(file->get_experimental_version() >= 2)
	{
		const uint8 counter = file->get_version() < 103000 ? LINE_DISTANCE : MAX_LINE_COST;
		for(uint8 i = 0; i < counter; i ++)
		{	
			file->rdwr_long(rolling_average[i]);
			file->rdwr_short(rolling_average_count[i]);
		}	
	}

	if(file->get_experimental_version() >= 9 && file->get_version() >= 110006)
	{
		file->rdwr_short(livery_scheme_index);
	}
	else
	{
		livery_scheme_index = 0;
	}

	if(file->get_experimental_version() >= 10)
	{
		if(file->is_saving())
		{
			uint32 count = average_journey_times->get_count();
			file->rdwr_long(count);

			koordhashtable_iterator_tpl<id_pair, average_tpl<uint16> > iter(average_journey_times);
			while(iter.next())
			{
				id_pair idp = iter.get_current_key();
				file->rdwr_short(idp.x);
				file->rdwr_short(idp.y);
				file->rdwr_short(iter.access_current_value().count);
				file->rdwr_short(iter.access_current_value().total);
			}
		}
		else
		{
			uint32 count = 0;
			file->rdwr_long(count);
			for(uint32 i = 0; i < count; i ++)
			{
				id_pair idp;
				file->rdwr_short(idp.x);
				file->rdwr_short(idp.y);
				
				uint16 count;
				uint16 total;
				file->rdwr_short(count);
				file->rdwr_short(total);

				average_tpl<uint16> average;
				average.count = count;
				average.total = total;

				average_journey_times->put(idp, average);
			}
		}
	}
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

int simline_t::get_replacing_convoys_count() const {
	int count=0;
	for (int i=0; i<line_managed_convoys.get_count(); ++i) {
		if (line_managed_convoys[i]->get_replace()) {
			count++;
		}
	}
	return count;
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
	if (!line_managed_convoys.empty()) 
	{
		register_stops( fpl );
	
		// Added by Knightly
		haltestelle_t::refresh_routing(fpl, goods_catg_index, sp);
		
		DBG_DEBUG("simline_t::renew_stops()", "Line id=%d, name='%s'", self.get_id(), name.c_str());
	}
}

void simline_t::set_schedule(schedule_t* fpl)
{
	if (this->fpl) 
	{
		haltestelle_t::refresh_routing(fpl, goods_catg_index, sp);
		unregister_stops();
		delete this->fpl;
	}
	this->fpl = fpl;
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

	for(uint8 i = 0; i < MAX_LINE_COST; i ++)
	{	
		rolling_average[i] = 0;
		rolling_average_count[i] = 0;
	}
}


void simline_t::init_financial_history()
{
	MEMZERO(financial_history);
}



/*
 * the current state saved as color
 * Meanings are BLACK (ok), WHITE (no convois), YELLOW (no vehicle moved), RED (last month income minus), DARK PURPLE (some vehicles overcrowded), BLUE (at least one convoi vehicle is obsolete)
 */
void simline_t::recalc_status()
{
	// normal state
	// Moved from an else statement at bottom
	// to ensure that this value is always initialised.
	state_color = COL_BLACK;

	if(financial_history[0][LINE_CONVOIS]==0) 
	{
		// no convoys assigned to this line
		state_color = COL_WHITE;
		withdraw = false;
	}
	else if(financial_history[0][LINE_PROFIT]<0) 
	{
		// Loss-making
		state_color = COL_RED;
	} 

	else if((financial_history[0][LINE_OPERATIONS]|financial_history[1][LINE_OPERATIONS])==0) 
	{
		// Stuck or static
		state_color = COL_YELLOW;
	}
	else if(has_overcrowded())
	{
		// Overcrowded
		state_color = COL_DARK_PURPLE;
	}
	else if(welt->use_timeline()) 
	{
		// Has obsolete vehicles.
		bool has_obsolete = false;
		for(unsigned i=0;  !has_obsolete  &&  i<line_managed_convoys.get_count();  i++ ) 
		{
			has_obsolete = line_managed_convoys[i]->has_obsolete_vehicles();
		}
		// now we have to set it
		state_color = has_obsolete ? COL_DARK_BLUE : COL_BLACK;
	}
}

bool simline_t::has_overcrowded() const
{
	ITERATE(line_managed_convoys,i)
	{
		if(line_managed_convoys[i]->get_overcrowded() > 0)
		{
			return true;
		}
	}
	return false;
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
	withdraw = !line_managed_convoys.empty();
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
	
	// Modified by	: Knightly
	// Purpose		: Determine removed and added categories and refresh only those categories.
	//				  Avoids refreshing unchanged categories
	minivec_tpl<uint8> differences(goods_catg_index.get_count() + old_goods_catg_index.get_count());

	// removed categories : present in old category list but not in new category list
	for (uint8 i = 0; i < old_goods_catg_index.get_count(); i++)
	{
		if ( ! goods_catg_index.is_contained( old_goods_catg_index[i] ) )
		{
			differences.append( old_goods_catg_index[i] );
		}
	}

	// added categories : present in new category list but not in old category list
	for (uint8 i = 0; i < goods_catg_index.get_count(); i++)
	{
		if ( ! old_goods_catg_index.is_contained( goods_catg_index[i] ) )
		{
			differences.append( goods_catg_index[i] );
		}
	}

	// refresh only those categories which are either removed or added to the category list
	haltestelle_t::refresh_routing(fpl, differences, sp);
}



void simline_t::set_withdraw( bool yes_no )
{
	withdraw = yes_no && !line_managed_convoys.empty();
	// convois in depots will be immeadiately destroyed, thus we go backwards
	for( sint32 i=line_managed_convoys.get_count()-1;  i>=0;  i--  ) {
		line_managed_convoys[i]->set_no_load(yes_no);	// must be first, since set withdraw might destroy convoi if in depot!
		line_managed_convoys[i]->set_withdraw(yes_no);
	}
}

void simline_t::propogate_livery_scheme()
{
	ITERATE(line_managed_convoys, i)
	{
		line_managed_convoys[i]->set_livery_scheme_index(livery_scheme_index);
		line_managed_convoys[i]->apply_livery_scheme();
	}
}
