
#include "../simconst.h"
#include "../simtypes.h"
#include "../simdebug.h"

#include "../simplay.h"
#include "../simfab.h"
#include "../simcity.h"
#include "../simworld.h"

#include "../dataobj/tabfile.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"

#include "../vehicle/simvehikel.h"

#include "../utils/simstring.h"


#include "scenario.h"



scenario_t::scenario_t(karte_t *w)
{
	welt = w;
	what_scenario = 0;
	city = NULL;
	target_factory = NULL;
	scenario_name = NULL;
}



void scenario_t::init( const char *filename, karte_t *w )
{
	welt = w;
	what_scenario = 0;
	city = NULL;
	target_factory = NULL;
	if(scenario_name) {
		free( scenario_name );
	}
	scenario_name = NULL;

	tabfile_t scenario;

	char path[1024];
	sprintf( path, "%s.tab", filename );
	if (!scenario.open(filename)) {
		dbg->error("scenario_t::scenario_t()", "Can't read %s", path );
		return;
	}

	tabfileobj_t contents;
	scenario.read(contents);

	scenario_name = strdup( contents.get( "savegame" ) );
	what_scenario = contents.get_int( "type", 0 );
	factor = contents.get_int( "factor", 1 );

	// may have additional info like city ...
	const char *cityname = contents.get( "cityname" );
	city = NULL;
	if(*cityname) {
		// find a city with this name ...
		const weighted_vector_tpl<stadt_t*> staedte = welt->gib_staedte();
		for(  int i=0;  staedte.get_count();  i++  ) {
			if(  strcmp( staedte[i]->gib_name(), cityname )==0  ) {
				city = staedte[i];
			}
		}
	}

	// ... or factory
	int *pos = contents.get_ints( "factorypos" );
	if(*pos==2  &&  welt) {
		fabrik_t *f = fabrik_t::gib_fab( welt, koord( pos[1], pos[2] ) );
		target_factory = f;
	}
}



void scenario_t::rdwr(loadsave_t *file)
{
	uint32 city_nr = 0xFFFFFFFFu;
	koord fabpos = koord::invalid;

	if(  file->is_saving()  ) {
		if(city) {
			city_nr = welt->gib_staedte().index_of( city );
		}
		if(  target_factory  ) {
			fabpos = target_factory->gib_pos().gib_2d();
		}
	}

	file->rdwr_short( what_scenario, "" );
	file->rdwr_long( city_nr, "" );
	file->rdwr_longlong( factor, "" );
	fabpos.rdwr( file );

	if(  file->is_loading()  ) {
		if(  city_nr < welt->gib_staedte().get_count()  ) {
			city = welt->gib_staedte()[city_nr];
		}
		target_factory = fabrik_t::gib_fab( welt, fabpos );
	}
}



/* recursive lookup of a factory tree:
 * count ratio of needed versus producing factories
 */
void scenario_t::get_factory_producing( fabrik_t *fab, int &producing, int &existing )
{
	int own_producing=0, own_existing=0;

	// now check for all input
	for(  uint ware_nr=0;  ware_nr<fab->gib_eingang().get_count();  ware_nr++  ) {
		if(fab->gib_eingang()[ware_nr].menge > 512) {
			producing ++;
			own_producing ++;
		}
		existing ++;
		own_existing ++;
	}

	if(fab->gib_eingang().get_count()>0) {
		// now check for all output (of not source ... )
		for(  uint ware_nr=0;  ware_nr<fab->gib_ausgang().get_count();  ware_nr++  ) {
			if(fab->gib_ausgang()[ware_nr].menge > 512) {
				producing ++;
				own_producing ++;
			}
			existing ++;
			own_existing ++;
		}
	}

	// now all delivering factories
	const vector_tpl <koord> & sources = fab->get_suppliers();
	for( unsigned q=0;  q<sources.get_count();  q++  ) {
		fabrik_t *qfab = fabrik_t::gib_fab(welt,sources[q]);
		if(  own_producing==own_existing  ) {
			// fully supplied => counts as 100% ...
			int i=0, cnt=0;
			get_factory_producing( qfab, i, cnt );
			producing += cnt;
			existing += cnt;
		}
		else {
			// try something else ...
			get_factory_producing( qfab, producing, existing );
		}
	}
}



// return percentage completed
int scenario_t::completed(int player_nr)
{
	switch(  what_scenario  ) {

		case CONNECT_CITY_WORKER:
			// check, if there are connections to all factories from all over the town
			return 0;

		case CONNECT_FACTORY_PAX:
			// check, if there is complete coverage of all connected towns to this factory
			return 0;

		case CONNECT_FACTORY_GOODS:
		{
			// true, if this factory can produce (i.e. more than one unit per good in each input)
			int prod=0, avail=0;
			get_factory_producing( target_factory, prod, avail );
			return (prod*100)/avail;
		}

		case DOUBLE_INCOME:
		{
			int pts = (int)( welt->gib_spieler(player_nr)->get_finance_history_month(0,COST_CASH)/factor );
			return min( 100, pts );
		}

		case BUILT_HEADQUARTER_AND_10_TRAINS:
		{
			spieler_t *sp = welt->gib_spieler(player_nr);
			int pts = 0;
			for (vector_tpl<convoihandle_t>::const_iterator i = welt->convois_begin(), end = welt->convois_end(); i != end; ++i) {
				convoihandle_t cnv = *i;
				if(  cnv->gib_besitzer() == sp  &&  cnv->gib_jahresgewinn()>0  &&  cnv->gib_vehikel_anzahl()>0  &&  cnv->gib_vehikel(0)->gib_waytype()==track_wt) {
					pts ++;
				}
			}
			pts = (int)( (pts*90l)/factor );
			pts += (sp->get_headquarter_pos() != koord::invalid) ? 10 : 0;
			return pts;
		}

		case TRANSPORT_1000_PAX:
			return min( 100, (welt->gib_spieler(player_nr)->get_finance_history_month(0,COST_TRANSPORTED_PAS)*(sint64)100)/(sint64)factor );

	}
	return 0;
}




const char *scenario_t::get_description()
{
	static char description[512];
	switch(  what_scenario  ) {

		case CONNECT_CITY_WORKER:
		case CONNECT_FACTORY_PAX:
		default:
			*description = 0;
			break;

		case CONNECT_FACTORY_GOODS:
			if(target_factory!=NULL) {
				sprintf( description, translator::translate("Supply %s at (%i,%i)"), target_factory->gib_name(), target_factory->gib_pos().x, target_factory->gib_pos().y );
			}
			else {
				tstrncpy( description, translator::translate("Connect factory"), 511 );
			}
			break;

		case DOUBLE_INCOME:
			{
				char money[64];
				money_to_string( money, (double)factor );
				sprintf( description, translator::translate("Account above %s"), money );
			}
			break;

		case BUILT_HEADQUARTER_AND_10_TRAINS:
			sprintf( description, translator::translate("Headquarter and %li trains"), factor );
			break;

		case TRANSPORT_1000_PAX:
			sprintf( description, translator::translate("Transport %li passengers"), (long)factor );
			break;
	}
	return description;
}
