/*
 * Fabrikfunktionen und Fabrikbau
 *
 * Hansjörg Malthaner
 *
 *
 * 25.03.00 Anpassung der Lagerkapazitäten: min. 5 normale Lieferungen
 *          sollten an Lager gehalten werden.
 */

#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simdebug.h"
#include "simio.h"
#include "simimg.h"
#include "simmem.h"
#include "simcolor.h"
#include "boden/grund.h"
#include "boden/boden.h"
#include "boden/fundament.h"
#include "simfab.h"
#include "simcity.h"
#include "simhalt.h"
#include "simskin.h"
#include "simtools.h"
#include "simware.h"
#include "simworld.h"
#include "besch/haus_besch.h"
#include "besch/ware_besch.h"
#include "player/simplay.h"

#include "simmesg.h"
#include "simintr.h"

#include "dings/wolke.h"
#include "dings/gebaeude.h"
#include "dings/field.h"
#include "dings/leitung2.h"

#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"

#include "besch/skin_besch.h"
#include "besch/fabrik_besch.h"
#include "bauer/hausbauer.h"
#include "bauer/warenbauer.h"
#include "bauer/fabrikbauer.h"

#include "gui/fabrik_info.h"

#include "utils/cbuffer_t.h"

#include "simwin.h"
#include "simgraph.h"

// Fabrik_t


static const int FAB_MAX_INPUT = 15000;


/**
 * Ordering based on relative distance to a fixed point `origin'.
 */
class RelativeDistanceOrdering
{
private:
	const koord m_origin;
public:
	RelativeDistanceOrdering(const koord& origin)
		: m_origin(origin)
	{ /* nothing */ }

	/**
	 * Returns true if `a' is closer to the origin than `b', otherwise false.
	 */
	bool operator()(const koord& a, const koord& b) const
	{
		return koord_distance(m_origin, a) < koord_distance(m_origin, b);
	}
};


fabrik_t *fabrik_t::get_fab(const karte_t *welt, const koord pos)
{
	const grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb) {
			return gb->get_fabrik();
		}
	}
	return NULL;
}


void fabrik_t::link_halt(halthandle_t halt)
{
	welt->access(pos.get_2d())->add_to_haltlist(halt);
}


void fabrik_t::unlink_halt(halthandle_t halt)
{
	planquadrat_t *plan=welt->access(pos.get_2d());
	if(plan) {
		plan->remove_from_haltlist(welt,halt);
	}
}


void fabrik_t::add_lieferziel(koord ziel)
{
	if(  !lieferziele.is_contained(ziel)  ) {
		lieferziele.insert_ordered( ziel, RelativeDistanceOrdering(pos.get_2d()) );
		// now tell factory too
		fabrik_t * fab = fabrik_t::get_fab(welt, ziel);
		if (fab) {
			fab->add_supplier(get_pos().get_2d());
		}
	}
}


void fabrik_t::rem_lieferziel(koord ziel)
{
	lieferziele.remove(ziel);
}

bool
fabrik_t::disconnect_consumer(koord pos) //Returns true if must be destroyed.
{
	rem_lieferziel(pos);
	if(lieferziele.get_count() < 1)
	{
		// If there are no consumers left, industry is orphaned.
		// Reconnect or close.

		// Attempt to reconnect. NOTE: This code may not work well if there are multiple supply types.
		
		for(sint16 i = welt->get_fab_list().get_count() - 1; i >= 0; i --)
		{
			fabrik_t* fab = welt->get_fab_list()[i];
			if(add_customer(fab)) 
			{
				//Only reconnect one.
				return false;
			}
		}
		return true;
	}
	return false;
}

bool
fabrik_t::disconnect_supplier(koord pos) //Returns true if must be destroyed.
{
	rem_supplier(pos);
	if(suppliers.get_count() < 1)
	{
		// If there are no suppliers left, industry is orphaned.
		// Reconnect or close.

		// Attempt to reconnect. NOTE: This code may not work well if there are multiple supply types.
		
		for(sint16 i = welt->get_fab_list().get_count() - 1; i >= 0; i --)
		{
			fabrik_t* fab = welt->get_fab_list()[i];
			if(add_supplier(fab))
			{
				//Only reconnect one.
				return false;
			}
		}
		return true;
	}
	return false;
}


fabrik_t::fabrik_t(karte_t* wl, loadsave_t* file)
{
	welt = wl;

	besitzer_p = NULL;
	power = 0;
	power_demand = 0;

	rdwr(file);

	if(  !welt->ist_in_kartengrenzen(pos.get_2d())  ) {
		dbg->error( "fabrik_t::baue()", "%s is not a valid position! (Will not be built!)", pos.get_str() );
	}
	else {
		baue(rotate);
		// now get rid of construction image
		for(  sint16 y=0;  y<besch->get_haus()->get_h(rotate);  y++  ) {
			for(  sint16 x=0;  x<besch->get_haus()->get_b(rotate);  x++  ) {
				gebaeude_t *gb = welt->lookup_kartenboden( pos.get_2d()+koord(x,y) )->find<gebaeude_t>();
				if(  gb  ) {
					gb->add_alter(10000);
				}
			}
		}
	}

	city = welt->get_city(pos.get_2d());
	if(city != NULL)
	{
		city->add_city_factory(this);
	}

	delta_sum = 0;
	delta_menge = 0;
	last_lieferziel_start = 0;
	total_input = total_output = 0;
	status = nothing;
	currently_producing = false;
	transformer_connected = false;
}


fabrik_t::fabrik_t(koord3d pos_, spieler_t* spieler, const fabrik_besch_t* fabesch) :
	welt(spieler->get_welt()),
	besch(fabesch),
	pos(pos_)
{
	this->pos.z = welt->max_hgt(pos.get_2d());

	besitzer_p = spieler;
	prodfaktor = 16;
	prodbase = besch->get_produktivitaet() + simrand(besch->get_bereich());
	prodbase = prodbase > 0 ? prodbase : 1;

	delta_sum = 0;
	delta_menge = 0;
	currently_producing = false;
	transformer_connected = false;
	power = 0;
	power_demand = 0;
	last_lieferziel_start = 0;
	total_input = total_output = 0;
	status = nothing;

	// create input information
	for(int i=0; i < fabesch->get_lieferanten(); i++) {
		const fabrik_lieferant_besch_t *lieferant = fabesch->get_lieferant(i);
		ware_production_t ware;
		ware.set_typ( lieferant->get_ware() );
		ware.abgabe_letzt = ware.abgabe_sum = 0;
		ware.max = lieferant->get_kapazitaet() << fabrik_t::precision_bits;
		ware.menge = 0;
		eingang.append(ware);
	}

	// create output information
	for (uint i = 0; i < fabesch->get_produkte(); i++) {
		const fabrik_produkt_besch_t *produkt = fabesch->get_produkt(i);
		ware_production_t ware;
		ware.set_typ( produkt->get_ware() );
		ware.abgabe_letzt = ware.abgabe_sum = 0;
		ware.max = produkt->get_kapazitaet() << fabrik_t::precision_bits;
		ware.menge = 0;
		if(  ware.max>0  &&  fabesch->get_lieferanten()==0  ) {
			// if source then start with full storage (thus AI will built immeadiately lines)
			ware.menge = ware.max-1;
		}
		ausgang.append(ware);
	}

	// Check to see whether this is within city limits, and add it to the city if it is.
	city = welt->get_city(pos.get_2d());
	if(city != NULL)
	{
		city->add_city_factory(this);
	}
}

void fabrik_t::delete_all_fields()
{
	while(!fields.empty()) 
	{
		planquadrat_t *plan = welt->access( fields.back().location );
		// if destructor is called when world is destroyed, plan is already invalid
		if (plan) {
			grund_t *gr = plan->get_kartenboden();
			if (field_t* f = gr->find<field_t>()) {
				delete f; // implicitly removes the field from fields
				plan->boden_ersetzen( gr, new boden_t( welt, gr->get_pos(), hang_t::flach ) );
				plan->get_kartenboden()->calc_bild();
				continue;
			}
		}
		fields.remove_at(fields.get_count()-1);
	}
}

fabrik_t::~fabrik_t()
{
	delete_all_fields();

	if(city != NULL)
	{
		city->remove_city_factory(this);
	}

	welt->decrease_actual_industry_density(1.0 / (double)get_besch()->get_gewichtung());

	//Disconnect this factory from all chains.
	//@author: jamespetts
	uint32 number_of_customers = lieferziele.get_count();
	uint32 number_of_suppliers = suppliers.get_count();

	if (!welt->get_is_shutting_down())
	{
		const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
		for(weighted_vector_tpl<stadt_t*>::const_iterator j = staedte.begin(), end = staedte.end(); j != end; ++j) 
		{
			(*j)->remove_connected_industry(this);
		}
		
		char buf[192];
		uint16 jobs =  besch->get_pax_level();
		sprintf(buf, translator::translate("Industry:\n%s\nhas closed,\nwith the loss\nof %d jobs.\n%d upstream\nsuppliers and\n%d downstream\ncustomers\nare affected."), translator::translate(get_name()), jobs, number_of_suppliers, number_of_customers);
		welt->get_message()->add_message(buf, pos.get_2d(), message_t::industry, COL_DARK_RED, skinverwaltung_t::neujahrsymbol->get_bild_nr(0));
		for(sint32 i = number_of_customers - 1; i >= 0; i --)
		{
			fabrik_t* tmp = get_fab(welt, lieferziele[i]);
			if(tmp->disconnect_supplier(pos.get_2d()))
			{
				//Orphaned, must be deleted.
				grund_t *gr = 0;
				gr = welt->lookup(tmp->get_pos());
				gebaeude_t* gb = gr->find<gebaeude_t>();
				hausbauer_t::remove(welt,  welt->get_spieler(1), gb);
			}
		}

		for(sint32 i = number_of_suppliers - 1; i >= 0; i --)
		{
			fabrik_t* tmp = get_fab(welt, suppliers[i]);
			if(tmp->disconnect_consumer(pos.get_2d()))
			{
				//Orphaned, must be deleted.
				grund_t *gr = 0;
				gr = welt->lookup(tmp->get_pos());
				gebaeude_t* gb = gr->find<gebaeude_t>();
				hausbauer_t::remove(welt,  welt->get_spieler(1), gb);
			}
		}
	}
}


void fabrik_t::baue(sint32 rotate)
{
	this->rotate = rotate;
	pos = welt->lookup_kartenboden(pos.get_2d())->get_pos();
	hausbauer_t::baue(welt, besitzer_p, pos, rotate, besch->get_haus(), this);
	pos = welt->lookup_kartenboden(pos.get_2d())->get_pos();
	if(besch->get_field()) {
		// if there are fields
		if(!fields.empty()) {
			for( uint16 i=0;  i<fields.get_count();  i++  ) {
				const koord k = fields[i].location;
				grund_t *gr=welt->lookup_kartenboden(k);
				if(  gr->get_typ()==grund_t::boden  ) {
					// first make foundation below
					grund_t *gr2 = new fundament_t(welt, gr->get_pos(), gr->get_grund_hang());
					welt->access(k)->boden_ersetzen(gr, gr2);
					gr2->obj_add( new field_t( welt, gr2->get_pos(), besitzer_p, besch->get_field()->get_field_class( fields[i].field_class_index ), this ) );
				}
				else {
					// there was already a building at this position => do not restore!
					fields.remove_at(i);
					i--;
				}
			}
		}
		else {
			// we will start with a certain minimum number
			while(fields.get_count()<besch->get_field()->get_min_fields()  &&  add_random_field(0))
				;
		}
	}
}


/* field generation code
 * spawns a field for sure if probability==0 and zero for 1024
 * @author Kieron Green
 */
bool fabrik_t::add_random_field(uint16 probability)
{
	// has fields, and not yet too many?
	const field_besch_t *fb = besch->get_field();
	if(fb==NULL  ||  fb->get_max_fields() <= fields.get_count()) {
		return false;
	}
	// we are lucky and are allowed to generate a field
	if(simrand(10000)<probability) {
		return false;
	}

	// we start closest to the factory, and check for valid tiles as we move out
	uint8 radius = 1;

	// pick a coordinate to use - create a list of valid locations and choose a random one
	slist_tpl<grund_t *> build_locations;
	do {
		for(sint32 xoff = -radius; xoff < radius + get_besch()->get_haus()->get_groesse().x ; xoff++) {
			for(sint32 yoff =-radius ; yoff < radius + get_besch()->get_haus()->get_groesse().y; yoff++) {
				// if we can build on this tile then add it to the list
				grund_t *gr = welt->lookup_kartenboden(pos.get_2d()+koord(xoff,yoff));
				if (gr != NULL &&
						gr->get_typ()        == grund_t::boden &&
						gr->get_hoehe()      == pos.z &&
						gr->get_grund_hang() == hang_t::flach &&
						gr->ist_natur() &&
						(gr->find<leitung_t>() || gr->kann_alle_obj_entfernen(NULL) == NULL)) {
					// only on same height => climate will match!
					build_locations.append(gr);
					assert(gr->find<field_t>() == NULL);
				}
				// skip inside of rectange (already checked earlier)
				if(radius > 1 && yoff == -radius && (xoff > -radius && xoff < radius + get_besch()->get_haus()->get_groesse().x - 1)) {
					yoff = radius + get_besch()->get_haus()->get_groesse().y - 2;
				}
			}
		}
		if(build_locations.get_count() == 0) {
			radius++;
		}
	} while (radius < 10 && build_locations.get_count() == 0);
	// built on one of the positions
	if(build_locations.get_count() > 0) {
		grund_t *gr = build_locations.at(simrand(build_locations.get_count()));
		leitung_t* lt = gr->find<leitung_t>();
		if(lt) {
			gr->obj_remove(lt);
		}
		gr->obj_loesche_alle(NULL);
		// first make foundation below
		const koord k = gr->get_pos().get_2d();
		field_data_t new_field(k);
		assert(!fields.is_contained(new_field));
		// Knightly : fetch a random field class besch based on spawn weights
		const weighted_vector_tpl<uint16> &field_class_indices = fb->get_field_class_indices();
		new_field.field_class_index = field_class_indices.at_weight( simrand( field_class_indices.get_sum_weight() ) );
		const field_class_besch_t *const field_class = fb->get_field_class( new_field.field_class_index );
		fields.append(new_field);
		grund_t *gr2 = new fundament_t(welt, gr->get_pos(), gr->get_grund_hang());
		welt->access(k)->boden_ersetzen(gr, gr2);
		gr2->obj_add( new field_t( welt, gr2->get_pos(), besitzer_p, field_class, this ) );
		// Knightly : adjust production base and storage capacities
		prodbase += field_class->get_field_production();
		prodbase = prodbase > 0 ? prodbase : 1;
		const uint32 ware_types = eingang.get_count() + ausgang.get_count();
		if(  ware_types>0  &&  field_class->get_storage_capacity()>0  ) {
			const sint32 share = ( (uint32)(field_class->get_storage_capacity()) << fabrik_t::precision_bits ) / ware_types;
			for(  uint32 i=0  ;  i<eingang.get_count()  ;  ++i  ) {
				eingang[i].max += share;
			}
			for(  uint32 i=0  ;  i<ausgang.get_count()  ;  ++i  ) {
				ausgang[i].max += share;
			}
		}
		if(lt) {
			gr2->obj_add( lt );
		}
		gr2->calc_bild();
		return true;
	}
	return false;
}


void fabrik_t::remove_field_at(koord pos)
{
	field_data_t field(pos);
	assert(fields.is_contained( field ));
	field = fields[ fields.index_of(field) ];
	const field_class_besch_t *const field_class = besch->get_field()->get_field_class( field.field_class_index );
	// Knightly : revert the field's effect on production base and storage capacities
	prodbase -= field_class->get_field_production();
	const uint32 ware_types = eingang.get_count() + ausgang.get_count();
	if(  ware_types>0  &&  field_class->get_storage_capacity()>0  ) {
		const sint32 share = ( (uint32)(field_class->get_storage_capacity()) << fabrik_t::precision_bits ) / ware_types;
		for(  uint32 i=0  ;  i<eingang.get_count()  ;  ++i  ) {
			eingang[i].max -= share;
		}
		for(  uint32 i=0  ;  i<ausgang.get_count()  ;  ++i  ) {
			ausgang[i].max -= share;
		}
	}
	fields.remove(field);
}


bool fabrik_t::ist_bauplatz(karte_t *welt, koord pos, koord groesse,bool wasser,climate_bits cl)
{
	if(pos.x > 0 && pos.y > 0 &&
		pos.x+groesse.x < welt->get_groesse_x() && pos.y+groesse.y < welt->get_groesse_y() &&
		( wasser  ||  welt->ist_platz_frei(pos, groesse.x, groesse.y, NULL, cl) )&&
		!ist_da_eine(welt,pos-koord(5,5),pos+groesse+koord(3,3))) {

		// check for water (no shore in sight!)
		if(wasser) {
			for(int y=0;y<groesse.y;y++) {
				for(int x=0;x<groesse.x;x++) {
					const grund_t *gr=welt->lookup_kartenboden(pos+koord(x,y));
					if(!gr->ist_wasser()  ||  gr->get_grund_hang()!=hang_t::flach) {
						return false;
					}
				}
			}
		}

		return true;
	}
	return false;
}


vector_tpl<fabrik_t *> &fabrik_t::sind_da_welche(karte_t *welt, koord min_pos, koord max_pos)
{
	static vector_tpl <fabrik_t*> fablist(16);
	fablist.clear();

	for(int y=min_pos.y; y<=max_pos.y; y++) {
		for(int x=min_pos.x; x<=max_pos.x; x++) {
			fabrik_t *fab=get_fab(welt,koord(x,y));
			if(fab) {
				if (fablist.append_unique(fab)) {
//DBG_MESSAGE("fabrik_t::sind_da_welche()","appended factory %s at (%i,%i)",gr->first_obj()->get_fabrik()->get_besch()->get_name(),x,y);
				}
			}
		}
	}
	return fablist;
}


bool fabrik_t::ist_da_eine(karte_t *welt, koord min_pos, koord max_pos )
{
	for(int y=min_pos.y; y<=max_pos.y; y++) {
		for(int x=min_pos.x; x<=max_pos.x; x++) {
			if(get_fab(welt,koord(x,y))) {
				return true;
			}
		}
	}
	return false;
}


void fabrik_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fabrik_t" );
	sint32 i;
	sint32 spieler_n;
	sint32 eingang_count;
	sint32 ausgang_count;
	sint32 anz_lieferziele;

	if(file->is_saving()) {
		eingang_count = eingang.get_count();
		ausgang_count = ausgang.get_count();
		anz_lieferziele = lieferziele.get_count();
		const char *s = besch->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
DBG_DEBUG("fabrik_t::rdwr()","loading factory '%s'",s);
		besch = fabrikbauer_t::get_fabesch(s);
		if(besch==NULL) {
			besch = fabrikbauer_t::get_fabesch(translator::compatibility_name(s));
			if (!besch) {
				dbg->fatal("fabrik_t::rdwr()", "no besch for %s", s);
			}
		}
		// set ware arrays ...
		eingang.clear();
		eingang.resize(besch->get_lieferanten());
		ausgang.clear();
		ausgang.resize(besch->get_produkte());
	}
	pos.rdwr(file);

	file->rdwr_byte(rotate);

	// now rebuilt information for received goods
	file->rdwr_long(eingang_count);
	for(i=0; i<eingang_count; i++) {
		ware_production_t dummy;
		const char *typ = NULL;

		if(file->is_saving()) {
			typ = eingang[i].get_typ()->get_name();
			dummy.menge = eingang[i].menge << (old_precision_bits - precision_bits);
			dummy.max   = eingang[i].max   << (old_precision_bits - precision_bits);
		}

		file->rdwr_str(typ);
		file->rdwr_long(dummy.menge);
		file->rdwr_long(dummy.max);
		if(file->is_loading()) {
			dummy.set_typ( warenbauer_t::get_info(typ) );
			guarded_free(const_cast<char *>(typ));

			// Hajo: repair files that have 'insane' values
			dummy.menge >>= (old_precision_bits-precision_bits);
			dummy.max >>= (old_precision_bits-precision_bits);
			if(dummy.menge < 0) {
				dummy.menge = 0;
			}
			if(dummy.menge > (FAB_MAX_INPUT << precision_bits)) {
				dummy.menge = (FAB_MAX_INPUT << precision_bits);
			}
			eingang.append(dummy);
		}
	}

	// now rebuilt information for produced goods
	file->rdwr_long(ausgang_count);
	for(i=0; i<ausgang_count; i++) {
		ware_production_t dummy;
		const char *typ = NULL;

		if(file->is_saving()) {
			typ = ausgang[i].get_typ()->get_name();
			dummy.menge = ausgang[i].menge << (old_precision_bits - precision_bits);
			dummy.max   = ausgang[i].max   << (old_precision_bits - precision_bits);
			dummy.abgabe_sum   = ausgang[i].abgabe_sum;
			dummy.abgabe_letzt = ausgang[i].abgabe_letzt;
		}
		file->rdwr_str(typ);
		file->rdwr_long(dummy.menge);
		file->rdwr_long(dummy.max);
		file->rdwr_long(dummy.abgabe_sum);
		file->rdwr_long(dummy.abgabe_letzt);

		if(file->is_loading()) {
			dummy.set_typ( warenbauer_t::get_info(typ));
			guarded_free(const_cast<char *>(typ));
			dummy.menge >>= (old_precision_bits-precision_bits);
			dummy.max >>= (old_precision_bits-precision_bits);
			ausgang.append(dummy);
		}
	}
	// restore other information
	spieler_n = welt->sp2num(besitzer_p);
	file->rdwr_long(spieler_n);
	file->rdwr_long(prodbase);
	file->rdwr_long(prodfaktor);

	// information on fields ...
	if(file->get_version()>99016) {
		file->rdwr_long(power);
	}

	// owner stuff
	if(file->is_loading()) {
		// take care of old files
		if(prodfaktor==1  ||  prodfaktor>16) {
			prodfaktor = 16;
		}
		if(file->get_version() < 86001) {
			koord k=besch->get_haus()->get_groesse();
			DBG_DEBUG("fabrik_t::rdwr()","correction of production by %i",k.x*k.y);
			// since we step from 86.01 per factory, not per tile!
			prodbase *= k.x*k.y*2;
		}
		// Hajo: restore factory owner
		// Due to a omission in Volkers changes, there might be savegames
		// in which factories were saved without an owner. In this case
		// set the owner to the default of player 1
		if(spieler_n == -1) {
			// Use default
			besitzer_p = welt->get_spieler(1);
		}
		else {
			// Restore owner pointer
			besitzer_p = welt->get_spieler(spieler_n);
		}
	}

	file->rdwr_long(anz_lieferziele);

	// connect/save consumer
	if(file->is_loading()) {
		koord k;
		for(int i=0; i<anz_lieferziele; i++) {
			k.rdwr(file);
			add_lieferziel(k);
		}
	}
	else {
		for(int i=0; i<anz_lieferziele; i++) {
			koord k = lieferziele[i];
			k.rdwr(file);
		}
	}

	// information on fields ...
	if(file->get_version()>99009) {
		if(file->is_saving()) {
			uint16 nr=fields.get_count();
			file->rdwr_short(nr);
			if(  file->get_version()>102002  && file->get_experimental_version() != 7 ) {
				// each field stores location and a field class index
				for(  uint16 i=0  ;  i<nr  ;  ++i  ) {
					koord k = fields[i].location;
					k.rdwr(file);
					uint16 idx = fields[i].field_class_index;
					file->rdwr_short(idx);
				}
			}
			else {
				// each field only stores location
				for(  uint16 i=0  ;  i<nr  ;  ++i  ) {
					koord k = fields[i].location;
					k.rdwr(file);
				}
			}
		}
		else {
			uint16 nr=0;
			koord k;
			uint16 idx;
			file->rdwr_short(nr);
			fields.resize(nr);
			if(  file->get_version()>102002  && file->get_experimental_version() != 7 ) {
				// each field stores location and a field class index
				for(  uint16 i=0  ;  i<nr  ;  ++i  ) {
					k.rdwr(file);
					file->rdwr_short(idx);
					if(  idx>=besch->get_field()->get_field_class_count()  ) {
						// set class index to 0 if it is out of range
						idx = 0;
					}
					fields.append( field_data_t(k, idx) );
				}
			}
			else {
				// each field only stores location
				for(  uint16 i=0  ;  i<nr  ;  ++i  ) {
					k.rdwr(file);
					fields.append( field_data_t(k, 0) );
				}
			}
		}
	}

	// restore city pointer here
	if(  file->get_version()>=99014  ) {
		sint32 nr = arbeiterziele.get_count();
		file->rdwr_long(nr);
		for( int i=0;  i<nr;  i++  ) {
			sint32 city_index = -1;
			if(file->is_saving()) {
				city_index = welt->get_staedte().index_of( arbeiterziele.at(i) );
			}
			file->rdwr_long(city_index);
			if(file->is_loading()) {
				// will also update factory information
				welt->get_staedte()[city_index]->add_factory_arbeiterziel( this );
			}
		}
	}
	else if(file->is_loading()) {
		// will be handled by the city after reloading
		arbeiterziele.clear();
	}
}


/*
 * let the chimney smoke, if there is something to produce
 * @author Hj. Malthaner
 */
void fabrik_t::smoke() const
{
	const rauch_besch_t *rada = besch->get_rauch();
	if(rada) {
		const koord size = besch->get_haus()->get_groesse(0)-koord(1,1);
		const uint8 rot = rotate%besch->get_haus()->get_all_layouts();
		koord ro = rada->get_pos_off(size,rot);
		grund_t *gr=welt->lookup_kartenboden(pos.get_2d()+ro);
		// to get same random order on different compilers
		const sint8 offsetx =  ((rada->get_xy_off(rot).x+simrand(7)-3)*TILE_STEPS)/16;
		const sint8 offsety =  ((rada->get_xy_off(rot).y+simrand(7)-3)*TILE_STEPS)/16;
		wolke_t *smoke =  new wolke_t(welt, gr->get_pos(), offsetx, offsety, rada->get_bilder() );
		gr->obj_add(smoke);
		welt->sync_add( smoke );
	}
}


/*
 * calculates the produktion per delta_t; scaled to PRODUCTION_DELTA_T
 * @author Hj. Malthaner - original
 */
uint32 fabrik_t::produktion(const uint32 produkt, const long delta_t) const
{
	// default prodfaktor = 16 => shift 4, default time = 1024 => shift 10, rest precion
	const uint32 max = prodbase * prodfaktor;
	uint32 menge = ((max >> (18-10+4-fabrik_t::precision_bits)) * delta_t / PRODUCTION_DELTA_T);

	if (ausgang.get_count() > produkt) {
		// wenn das lager voller wird, produziert eine Fabrik weniger pro step

		const uint32 maxi = ausgang[produkt].max;
		const uint32 actu = ausgang[produkt].menge;

		if(actu<maxi) {
			if(menge>(0x7FFFFFFFu/maxi)) {
				// avoid overflow
				menge = (((maxi-actu)>>5)*(menge>>5))/(maxi>>10);
			}
			else {
				// and that is the simple formula
				menge = (menge*(maxi-actu)) / maxi;
			}
		}
		else {
			// overfull? Reduce to maximum
			menge = 0;
		}
	}

	return menge;
}


sint32 fabrik_t::input_vorrat_an(const ware_besch_t *typ)
{
	sint32 menge = -1;

	for (uint32 index = 0; index < eingang.get_count(); index++) {
		if (typ == eingang[index].get_typ()) {
			menge = eingang[index].menge >> precision_bits;
			break;
		}
	}

	return menge;
}


sint32 fabrik_t::vorrat_an(const ware_besch_t *typ)
{
	sint32 menge = -1;

	for (uint32 index = 0; index < ausgang.get_count(); index++) {
		if (typ == ausgang[index].get_typ()) {
			menge = ausgang[index].menge >> precision_bits;
			break;
		}
	}

	return menge;
}


sint32 fabrik_t::hole_ab(const ware_besch_t *typ, sint32 menge)
{
	for (uint32 index = 0; index < ausgang.get_count(); index++) {
		if (ausgang[index].get_typ() == typ) {
			if (ausgang[index].menge >> precision_bits >= menge) {
				ausgang[index].menge -= menge << precision_bits;
			} else {
				menge = ausgang[index].menge >> precision_bits;
				ausgang[index].menge = 0;
			}

			ausgang[index].abgabe_sum += menge;

			return menge;
		}
	}

	// ware "typ" wird hier nicht produziert
	return -1;
}


sint32 fabrik_t::liefere_an(const ware_besch_t *typ, sint32 menge)
{
	for (uint32 index = 0; index < eingang.get_count(); index++) {
		if (eingang[index].get_typ() == typ) {
			// Hajo: avoid overflow
			if (eingang[index].menge < (FAB_MAX_INPUT - menge) << precision_bits) {
				eingang[index].menge += menge << precision_bits;
			}

			// sollte maximale lagerkapazitaet pruefen
			return menge;
		}
	}

	// ware "typ" wird hier nicht verbraucht
	return -1;
}


sint32 fabrik_t::verbraucht(const ware_besch_t *typ)
{
	for(uint32 index = 0; index < eingang.get_count(); index ++) {
		if (eingang[index].get_typ() == typ) {
			// sollte maximale lagerkapazitaet pruefen
			return eingang[index].menge > eingang[index].max;
		}
	}
	return -1;  // wird hier nicht verbraucht
}


void fabrik_t::step(long delta_t)
{
	if(delta_t==0) {
		return;
	}

	// produce nothing/consumes nothing ...
	if(  eingang.empty()  &&  ausgang.empty()  ) {
		// power station? => produce power
		if(  besch->is_electricity_producer()  ) {
			currently_producing = true;
			power = prodbase * PRODUCTION_DELTA_T * 4;
		}

		// produced => trigger smoke
		delta_menge = 1 << fabrik_t::precision_bits;
	}
	else {
		// not a producer => then consume electricity ...
		if(  !besch->is_electricity_producer()  ) {
			// one may be thinking of linking this to actual production only
			prodfaktor = 16 + (  16 * power * besch->get_inverse_electricity_proportion() / (prodbase * PRODUCTION_DELTA_T + 1)  );
		}

		const uint32 ecount = eingang.get_count();
		uint32 index = 0;
		uint32 produkt = 0;

		currently_producing = false;	// needed for electricity
		power_demand = 0;

		if(  ausgang.empty()  ) {
			// consumer only ...
			uint32 menge = produktion(produkt, delta_t);

			if(  besch->is_electricity_producer()  ) {
				// power station => start with no production
				power = 0;
			}

			// finally consume stock
			for(  index = 0;  index < ecount;  index++  ) {

				const uint32 vb = besch->get_lieferant(index)->get_verbrauch();
				const uint32 v = max(1,(menge*vb) >> 8);

				if(  (uint32)eingang[index].menge > v + 1  ) {
					eingang[index].menge -= v;
					currently_producing = true;
					if(  besch->is_electricity_producer()  ) {
						// power station => produce power
						power += prodbase * PRODUCTION_DELTA_T * 4;
					}
					// to find out, if storage changed
					delta_menge += v;
				}
				else {
					if(  besch->is_electricity_producer()  ) {
						// power station => produce power
						power += prodbase*  PRODUCTION_DELTA_T * 4 * eingang[index].menge / (v + 1);
					}
					delta_menge += eingang[index].menge;
					eingang[index].menge = 0;
				}
			}
		}
		else {
			// ok, calulate maximum allowed consumption
			uint32 min_menge = 0x7FFFFFFF;
			uint32 consumed_menge = 0;
			for(index = 0; index < ecount; index ++) {
				// verbrauch fuer eine Einheit des Produktes (in 1/256)
				const uint32 vb = besch->get_lieferant(index)->get_verbrauch();
				const uint32 n = eingang[index].menge * 256 / vb;

				if(n<min_menge) {
					min_menge = n;    // finde geringsten vorrat
				}
			}

			// produces something
			for (produkt = 0; produkt < ausgang.get_count(); produkt++) {
				uint32 menge;

				if(ecount>0) {
					// calculate production
					const uint32 p_menge = produktion(produkt, delta_t);
					menge = p_menge < min_menge ? p_menge : min_menge;  // production smaller than possible due to consumption
					if(menge>consumed_menge) {
						consumed_menge = menge;
					}
				}
				else {
					// source producer
					menge = produktion(produkt, delta_t);
				}

				if (menge>0) {
					const uint32 pb = besch->get_produkt(produkt)->get_faktor();
					// ensure some minimum production
					const uint32 p = max(1,(menge*pb) >> 8);

					// produce
					if (ausgang[produkt].menge < ausgang[produkt].max) {
						// to find out, if storage changed
						delta_menge += p;
						ausgang[produkt].menge += p;
						// if less than 3/4 filled we neary always consume power
						currently_producing |= (ausgang[produkt].menge*4 < ausgang[produkt].max*3);
					}
					else {
						ausgang[produkt].menge = ausgang[produkt].max - 1;
					}
				}
			}

			// and finally consume stock
			for(index = 0; index<ecount; index ++) {

				const uint32 vb = besch->get_lieferant(index)->get_verbrauch();
				const uint32 v = (consumed_menge*vb) >> 8;

				if ((uint32)eingang[index].menge > v+1) {
					eingang[index].menge -= v;
				}
				else {
					eingang[index].menge = 0;
				}
			}

		}

		if(  currently_producing  ) {
			// requires full power even if runs out of raw material next cycle
			power_demand = PRODUCTION_DELTA_T * (uint32)( (float)prodbase * besch->get_electricity_proportion() );
		}

		// not a power station => then consume all electricity ...
		if(  !besch->is_electricity_producer()  ) {
			power = 0;
		}
	}

	delta_sum += delta_t;
	if(  delta_sum > PRODUCTION_DELTA_T  ) {
		delta_sum = delta_sum % PRODUCTION_DELTA_T;

		// distribute, if there are more than 10 waiting ...
		for(  uint32 produkt = 0;  produkt < ausgang.get_count();  produkt++  ) {
			// either more than ten or nearly full (if there are less than ten output)
			if(  ausgang[produkt].menge > (10 << precision_bits)  ||  ausgang[produkt].menge*2 > ausgang[produkt].max  ) {

				verteile_waren(produkt);
				INT_CHECK("simfab 636");
			}
		}

		recalc_factory_status();

		// rescale delta_menge here: all products should be produced at least once
		// (if consumer only: all supplements should be consumed once)
		const uint32 min_change = ausgang.empty() ? eingang.get_count() : ausgang.get_count();

		if((delta_menge>>fabrik_t::precision_bits) > min_change) {

			// we produced some real quantity => smoke
			smoke();

			if(besch->get_field()  &&  fields.get_count()<besch->get_field()->get_max_fields()) {
				// spawn new field with given probablitily
				add_random_field(besch->get_field()->get_probability());
			}

			INT_CHECK("simfab 558");
			// reset for next cycle
			delta_menge = 0;
		}

		// to distribute to all target equally, we use this counter, for the factory, to try first
		last_lieferziel_start ++;
	}

}


class distribute_ware_t
{
public:
	halthandle_t halt;
	sint32	space_left;
	ware_t	ware;
	distribute_ware_t( halthandle_t h, sint32 l, ware_t w )
	{
		halt = h;
		space_left = l;
		ware = w;
	}
	distribute_ware_t() {}
};


/**
 * Die erzeugten waren auf die Haltestellen verteilen
 * "The produced were on the stops distribute" (Babelfish)
 * @author Hj. Malthaner
 */
void fabrik_t::verteile_waren(const uint32 produkt)
{	
	// wohin liefern ?
	if (lieferziele.empty()) 
	{
		return;
	}

	// not connected?
	const planquadrat_t *plan = welt->lookup(pos.get_2d());
	if(plan==NULL) 
	{
		dbg->fatal("fabrik_t::verteile_waren", "%s has not distibution target", get_name() );
	}
	if(plan->get_haltlist_count()==0) 
	{
		return;
	}

	slist_tpl<distribute_ware_t> dist_list;
	bool still_overflow = true;

	/* prissi: distribute goods to factory
	 * that has not an overflowing input storage
	 * also prevent stops from overflowing, if possible
	 */
	sint32 menge = ausgang[produkt].menge >> precision_bits;

	// ok, first send everything away
	const halthandle_t *haltlist = plan->get_haltlist();

	for(unsigned i = 0; i < plan->get_haltlist_count(); i++) 
	{
		halthandle_t halt = haltlist[i];

		// Über alle Ziele iterieren
		// "Iterate over all goals" (Babelfish)
		for(uint32 n = 0; n < lieferziele.get_count(); n++) 
		{
			// prissi: this way, the halt, that is tried first, will change. 
			// As a result, if all destinations are empty, it will be spread evenly.
			const koord lieferziel = lieferziele[(n + last_lieferziel_start) % lieferziele.get_count()];

			fabrik_t * ziel_fab = get_fab(welt, lieferziel);
			int vorrat;

			if (ziel_fab && (vorrat = ziel_fab->verbraucht(ausgang[produkt].get_typ())) >= 0) 
			{
				//ware_t ware(ausgang[produkt].get_typ());
				ware_t ware(ausgang[produkt].get_typ(), halt);
				ware.menge = menge;
				ware.set_zielpos( lieferziel );
				ware.arrival_time = welt->get_zeit_ms();

				unsigned w;
				// find the index in the target factory
				for (w = 0; w < ziel_fab->get_eingang().get_count() && ziel_fab->get_eingang()[w].get_typ() != ware.get_besch(); w++) 
				{
					// emtpy
				}

				// Station can only store up to a maximum amount of goods per square
				const sint32 halt_left = (sint32)halt->get_capacity(2) - (sint32)halt->get_ware_summe(ware.get_besch());
				// ok, still enough space
				const uint16 current_journey_time = halt->find_route(ware);
				if(current_journey_time < 65535)
				{
					// if only overflown factories found => deliver to first
					// else deliver to non-overflown factory
					bool overflown = (ziel_fab->get_eingang()[w].menge >= ziel_fab->get_eingang()[w].max);

					if(!welt->get_einstellungen()->get_just_in_time()) 
					{

						// distribution also to overflowing factories
						if(still_overflow  &&  !overflown) 
						{
							// not overflowing factory found
							still_overflow = false;
							dist_list.clear();
						}
						if(still_overflow  ||  !overflown) 
						{
							dist_list.insert( distribute_ware_t( halt, halt_left, ware ) );
						}
					}
					else 
					{
						// only distribute to no-overflowed factories
						if(!overflown) 
						{
							dist_list.insert( distribute_ware_t( halt, halt_left, ware ) );
						}
					}
				}
			}
		}
	}

	// Auswertung der Ergebnisse
	// "Evaluation of the results" (Babelfish)
	if (!dist_list.empty()) 
	{
		slist_iterator_tpl<distribute_ware_t> iter (dist_list);

		ware_t best_ware       = dist_list.front().ware;
		halthandle_t best_halt = dist_list.front().halt;
		sint32 best_amount        = 999999;
		sint32 capacity_left = -1;

		while(iter.next()) 
		{
			halthandle_t halt = iter.get_current().halt;
			const ware_t& ware = iter.get_current().ware;

			const sint32 amount = (sint32)halt->get_ware_fuer_zielpos(ausgang[produkt].get_typ(),ware.get_zielpos());
			if(amount < best_amount) 
			{
				best_ware = ware;
				best_halt = halt;
				best_amount = amount;
				capacity_left = iter.get_current().space_left;
			}
//DBG_MESSAGE("verteile_waren()","best_amount %i %s",best_amount,translator::translate(ware.get_name()));
		}

		menge = max( 10, min( menge, 9+capacity_left ) );
		best_ware.menge = menge;
		if(capacity_left<0) 
		{
			// find, what is most waiting here from us
			ware_t most_waiting(ausgang[produkt].get_typ());
			most_waiting.menge = 0;
			for(uint32 n=0; n<lieferziele.get_count(); n++) 
			{
				const uint32 amount = (sint32)best_halt->get_ware_fuer_zielpos( ausgang[produkt].get_typ(), lieferziele[n] );
				if(amount > most_waiting.menge) 
				{
					most_waiting.set_zielpos( lieferziele[n] );
					most_waiting.menge = amount;
					most_waiting.arrival_time = welt->get_zeit_ms();
				}
			}


			//  we will reroute some goods
			if(best_amount==0  &&  most_waiting.menge>0) 
			{
				// remove something from the most waiting goods
				if(best_halt->recall_ware( most_waiting, min(most_waiting.menge/2,1-capacity_left) ) ) 
				{
					best_ware.menge += most_waiting.menge;
				}
				else {
					// overcrowded with other stuff (not from us)
					return;
				}
			}
			else {
				// overflowed with our own ware and we have still nearly full stock
				if(  ausgang[produkt].menge>= 0.75 * ausgang[produkt].max  ) 
				{
					/* Station too full, notify player */
						best_halt->bescheid_station_voll();
				}
				return;
			}
		}
		ausgang[produkt].menge -= menge << precision_bits;
		best_halt->starte_mit_route(best_ware);
		best_halt->recalc_status();
	}

}


void fabrik_t::neuer_monat()
{
	for (uint32 index = 0; index < ausgang.get_count(); index++) {
		ausgang[index].abgabe_letzt = ausgang[index].abgabe_sum;
		ausgang[index].abgabe_sum = 0;
	}

	// This needs to be re-checked regularly, as cities grow.
	stadt_t* c = welt->get_city(pos.get_2d());

	if(city == NULL && c != NULL)
	{
		city = c;
		city->add_city_factory(this);
	}
	if(city != NULL && c != city && c != NULL)
	{
		city = c;
		city->add_city_factory(this);
	}

	// Check to see whether factory is obsolete.
	// If it is, give it a chance of being closed down.
	// @author: jamespetts

	if(welt->use_timeline() && besch->get_haus()->get_retire_year_month() < welt->get_timeline_year_month())
	{
		const uint32 difference =  welt->get_timeline_year_month() - besch->get_haus()->get_retire_year_month();
		const uint32 max_difference = welt->get_einstellungen()->get_factory_max_years_obsolete() * 12;
		bool closedown = false;
		if(difference > max_difference)
		{
			closedown = true;
		}
		
		else
		{
			float proportion = (float)difference / (float)max_difference;
			proportion *= 2.5F; //Set to percentage value, but take into account fact will be frequently checked (would otherwise be * 100.0F - large change to take into account frequency of checking)
			const float chance = (float)(simrand(10000) / 100.0F);
			if(chance <= proportion)
			{
				closedown = true;
			}
		}

		if(closedown)
		{
			grund_t *gr = 0;
			gr = welt->lookup(pos);
			gebaeude_t* gb = gr->find<gebaeude_t>();
			char buf[192];
			
			const int upgrades_count = besch->get_upgrades_count();
			if(upgrades_count > 0)
			{
				// This factory has some upgrades: consider upgrading.
				minivec_tpl<const fabrik_besch_t*> upgrade_list(upgrades_count);
				const double max_density = welt->get_target_industry_density() * 1.5;
				const double adjusted_density = welt->get_actual_industry_density() - (1 / besch->get_gewichtung());
				for(uint16 i = 0; i < upgrades_count; i ++)
				{
					// Check whether any upgrades are suitable.
					// Currently, they must be of identical size and have
					// identical outputs and inputs, as the upgrade mechanism
					// is very simple. In future, it might be possible to write
					// more sophisticated upgrading code to enable industries
					// that are not identical in such a way to be upgraded.
					// Thus, non-suitable upgrades are allowed to be specified
					// in the .dat files for future compatibility.

					const fabrik_besch_t* fab = besch->get_upgrades(i);
					if(	fab != NULL && fab->is_electricity_producer() == besch->is_electricity_producer() &&
						fab->get_haus()->get_b() == besch->get_haus()->get_b() &&
						fab->get_haus()->get_h() == besch->get_haus()->get_h() &&
						fab->get_haus()->get_groesse() == besch->get_haus()->get_groesse() &&
						fab->get_lieferanten() == besch->get_lieferanten() &&
						fab->get_produkte() ==  besch->get_produkte() &&
						fab->get_haus()->get_intro_year_month() <= welt->get_timeline_year_month() &&
						fab->get_haus()->get_retire_year_month() >= welt->get_timeline_year_month() &&
						adjusted_density < (max_density + (1 / fab->get_gewichtung())))
					{
						upgrade_list.append_unique(fab);
					}
				}
				
				const uint8 list_count = upgrade_list.get_count();
				if(list_count > 0)
				{
					double total_density = 0;
					ITERATE(upgrade_list, j)
					{
						total_density += (1 / upgrade_list[j]->get_gewichtung());
					}
					const double average_density = total_density / list_count;
					const double probability_floating = 1 / ((1 - ((adjusted_density + average_density) / max_density)) * upgrade_list.get_count());
					const uint32 chance = simrand(probability_floating);
					if(chance < list_count)
					{
						// All the conditions are met: upgrade.
						const fabrik_besch_t* new_type = upgrade_list[chance];
						float proportion = new_type->get_field() ? (float)new_type->get_field()->get_max_fields() / (float)besch->get_field()->get_max_fields() : 0.0;
						const uint16 adjusted_number_of_fields = proportion ? fields.get_count() * proportion : 0;
						delete_all_fields();
						const char* old_name = get_name();
						besch = new_type;
						const char* new_name = get_name();
						gb->calc_bild();
						// Base production is randomised, so is an instance value. Must re-set from the type.
						prodbase = besch->get_produktivitaet() + simrand(besch->get_bereich());
						// Re-add the fields
						for(uint16 i = 0; i < adjusted_number_of_fields; i ++)
						{
							add_random_field(0);
						}
						sprintf(buf, translator::translate("Industry:\n%s\nhas been upgraded\nto industry:\n%s."), translator::translate(old_name), translator::translate(new_name));
						welt->get_message()->add_message(buf, pos.get_2d(), message_t::industry, CITY_KI, skinverwaltung_t::neujahrsymbol->get_bild_nr(0));
						return;
					}
				}
			}

			hausbauer_t::remove(welt, welt->get_spieler(1), gb);
		}
	}
	// NOTE: No code should come after this part, as the closing down code may cause this object to be deleted.
}

// static !
unsigned fabrik_t::status_to_color[5] = {COL_RED, COL_ORANGE, COL_GREEN, COL_YELLOW, COL_WHITE };

#define FL_WARE_NULL           1
#define FL_WARE_ALLENULL       2
#define FL_WARE_LIMIT          4
#define FL_WARE_ALLELIMIT      8
#define FL_WARE_UEBER75        16
#define FL_WARE_ALLEUEBER75    32
#define FL_WARE_FEHLT_WAS      64


/* returns the status of the current factory, as well as output */
void fabrik_t::recalc_factory_status()
{
	unsigned long warenlager;
	char status_ein;
	char status_aus;

	int haltcount=welt->lookup(pos.get_2d())->get_haltlist_count();

	// set bits for input
	warenlager = 0;
	status_ein = FL_WARE_ALLELIMIT;
	for (uint j = 0; j < eingang.get_count(); j++) {
		if (eingang[j].menge >= eingang[j].max) {
			status_ein |= FL_WARE_LIMIT;
		}
		else {
			status_ein &= ~FL_WARE_ALLELIMIT;
		}
		warenlager += eingang[j].menge;
		if(  (eingang[j].menge>>fabrik_t::precision_bits)==0  ) {
			status_ein |= FL_WARE_FEHLT_WAS;
		}

	}
	warenlager >>= fabrik_t::precision_bits;
	if(warenlager==0) {
		status_ein |= FL_WARE_ALLENULL;
	}
	total_input = warenlager;

	// one ware missing, but producing
	if(status_ein&FL_WARE_FEHLT_WAS  &&  ausgang.get_count()>0  &&  haltcount>0) {
		status = bad;
		return;
	}

	// set bits for output
	warenlager = 0;
	status_aus = FL_WARE_ALLEUEBER75|FL_WARE_ALLENULL;
	for (uint j = 0;j < ausgang.get_count(); j++) {
		if (ausgang[j].menge > 0) {

			status_aus &= ~FL_WARE_ALLENULL;
			if (ausgang[j].menge >= 0.75 * ausgang[j].max) {
				status_aus |= FL_WARE_UEBER75;
			}
			else {
				status_aus &= ~FL_WARE_ALLEUEBER75;
			}
			warenlager += ausgang[j].menge;
			status_aus &= ~FL_WARE_ALLENULL;
		}
		else {
			// menge = 0
			status_aus &= ~FL_WARE_ALLEUEBER75;
		}
	}
	warenlager >>= fabrik_t::precision_bits;
	total_output = warenlager;

	// now calculate status bar
	if(eingang.get_count()==0) {
		// does not consume anything, should just produce

		if(ausgang.get_count()==0) {
			// does also not produce anything
			status = nothing;
		}
		else if(status_aus&FL_WARE_ALLEUEBER75  ||  status_aus&FL_WARE_UEBER75) {
			status = inactive;	// not connected?
			if(haltcount>0) {
				if(status_aus&FL_WARE_ALLEUEBER75) {
					status = bad;	// connect => needs better service
				}
				else {
					status = medium;	// connect => needs better service for at least one product
				}
			}
		}
		else {
			status = good;
		}
	}
	else if(ausgang.get_count()==0) {
		// nothing to produce

		if(status_ein&FL_WARE_ALLELIMIT) {
			// we assume not served
			status = bad;
		}
		else if(status_ein&FL_WARE_LIMIT) {
			// served, but still one at limit
			status = medium;
		}
		else if(status_ein&FL_WARE_ALLENULL) {
			status = inactive;	// assume not served
			if(haltcount>0) {
				// there is a halt => needs better service
				status = bad;
			}
		}
		else {
			status = good;
		}
	}
	else {
		// produces and consumes
		if((status_ein&FL_WARE_ALLELIMIT)!=0  &&  (status_aus&FL_WARE_ALLEUEBER75)!=0) {
			status = bad;
		}
		else if((status_ein&FL_WARE_ALLELIMIT)!=0  ||  (status_aus&FL_WARE_ALLEUEBER75)!=0) {
			status = medium;
		}
		else if((status_ein&FL_WARE_ALLENULL)!=0  &&  (status_aus&FL_WARE_ALLENULL)!=0) {
			// not producing
			status = inactive;
		}
		else if(haltcount>0  &&  ((status_ein&FL_WARE_ALLENULL)!=0  ||  (status_aus&FL_WARE_ALLENULL)!=0)) {
			// not producing but out of supply
			status = medium;
		}
		else {
			status = good;
		}
	}
}


void fabrik_t::zeige_info() const
{
	gebaeude_t *gb = welt->lookup(pos)->find<gebaeude_t>();
	create_win(new fabrik_info_t(this, gb), w_info, (long)gb );
}


void fabrik_t::info(cbuffer_t& buf) const
{
	buf.clear();
	buf.append( translator::translate("Durchsatz") );
	buf.append( get_current_production(), 0 );
	buf.append( translator::translate("units/day") );
	buf.append( "\n" );
	if(get_besch()->is_electricity_producer())
	{
		buf.append(translator::translate("Electrical output: "));
	}
	else
	{
		buf.append(translator::translate("Electrical demand: "));
	}

	const float electricity_proportion = get_besch()->is_electricity_producer() ? 4 : get_besch()->get_electricity_proportion();
	const uint32 p = (prodbase * PRODUCTION_DELTA_T) * electricity_proportion;
	buf.append(p>>POWER_TO_MW);
	buf.append(" MW");

	if(city != NULL)
	{
		buf.append("\n\n");
		buf.append(translator::translate("City"));
		buf.append(": ");
		buf.append(city->get_name());
	}
	buf.append("\n");

	if (!lieferziele.empty()) {
		buf.append("\n");
		buf.append(translator::translate("Abnehmer"));
		buf.append(":\n");

		for(uint32 i=0; i<lieferziele.get_count(); i++) {
			const koord lieferziel = lieferziele[i];

			fabrik_t *fab = get_fab( welt, lieferziel );
			if(fab) {
				buf.append("   ");
				buf.append(translator::translate(fab->get_name()));
				buf.append(" (");
				buf.append(lieferziel.x);
				buf.append(", ");
				buf.append(lieferziel.y);
				buf.append(")\n");
			}
		}
	}

	if (!suppliers.empty()) {
		buf.append("\n");
		buf.append(translator::translate("Suppliers"));
		buf.append(":\n");

		for(uint32 i=0; i<suppliers.get_count(); i++) {
			const koord supplier = suppliers[i];

			fabrik_t *fab = get_fab( welt, supplier );
			if(fab) {
				buf.append("   ");
				buf.append(translator::translate(fab->get_name()));
				buf.append(" (");
				buf.append(supplier.x);
				buf.append(", ");
				buf.append(supplier.y);
				buf.append(")\n");
			}
		}
	}

	if (!arbeiterziele.empty()) {
		slist_iterator_tpl<stadt_t *> iter (arbeiterziele);

		buf.append("\n");
		buf.append(translator::translate("Arbeiter aus:"));
		buf.append("\n");

		while(iter.next()) {
			stadt_t *stadt = iter.get_current();

			buf.append("   ");
			buf.append(stadt->get_name());
			buf.append("\n");

		}
		// give a passenger level for orientation
		int passagier_rate = besch->get_pax_level();
		buf.append("\n");
		buf.append(translator::translate("Passagierrate"));
		buf.append(": ");
		buf.append(passagier_rate);
		buf.append("\n");

		buf.append(translator::translate("Postrate"));
		buf.append(": ");
		buf.append(passagier_rate);
		buf.append("\n");
	}

	if (!ausgang.empty()) {

		buf.append("\n");
		buf.append(translator::translate("Produktion"));
		buf.append(":\n");

		for (uint32 index = 0; index < ausgang.get_count(); index++) {
			const ware_besch_t * type = ausgang[index].get_typ();

			buf.append(" - ");
			buf.append(translator::translate(type->get_name()));
			buf.append(" ");
			buf.append(ausgang[index].menge / (double)(1<<precision_bits), 0 );
			buf.append("/");
			buf.append(ausgang[index].max >> fabrik_t::precision_bits,0);
			buf.append(translator::translate(type->get_mass()));

			if(type->get_catg() != 0) {
				buf.append(", ");
				buf.append(translator::translate(type->get_catg_name()));
			}

			buf.append(", ");
			buf.append((besch->get_produkt(index)->get_faktor()*100l)/256.0,0);
			buf.append("%\n");
		}
	}

	if (!eingang.empty()) {

		buf.append("\n");
		buf.append(translator::translate("Verbrauch"));
		buf.append(":\n");

		for (uint32 index = 0; index < eingang.get_count(); index++) {

			buf.append(" - ");
			buf.append(translator::translate(eingang[index].get_typ()->get_name()));
			buf.append(" ");
			buf.append(eingang[index].menge / (double)(1<<precision_bits), 0 );
			buf.append("/");
			buf.append(eingang[index].max >> precision_bits,0);
			buf.append(translator::translate(eingang[index].get_typ()->get_mass()));
			buf.append(", ");
			buf.append((besch->get_lieferant(index)->get_verbrauch()*100l)/256.0,0);
			buf.append("%\n");
		}
	}

	const planquadrat_t *plan = welt->lookup(get_pos().get_2d());
	if(plan  &&  plan->get_haltlist_count()>0) {
		buf.append("\n");
		buf.append(translator::translate("Connected stops"));
		buf.append("\n");
		for(  uint i=0;  i<plan->get_haltlist_count();  i++  ) {
			buf.append(" - ");
			buf.append(plan->get_haltlist()[i]->get_name());
			buf.append("\n");
		}
	}
}


void fabrik_t::laden_abschliessen()
{
	if(welt->get_einstellungen()->is_crossconnect_factories()) {
		add_all_suppliers();
	}
	else {
		for(uint32 i=0; i<lieferziele.get_count(); i++) {
			fabrik_t * fab2 = fabrik_t::get_fab(welt, lieferziele[i]);
			if (fab2) {
				fab2->add_supplier(pos.get_2d());
			}
			else {
				// remove this ...
				dbg->warning( "fabrik_t::laden_abschliessen()", "No factory at expected position %s!", lieferziele[i].get_str() );
				lieferziele.remove_at(i);
				i--;
			}
		}
	}
}


void fabrik_t::rotate90( const sint16 y_size )
{
	if(  this!=get_fab( welt, pos.get_2d() )  ){
		// was not rotated, because tile (0,0) is missing
		pos.rotate90( y_size );
		dbg->warning( "fabrik_t::rotate90()","no tile zero form %s at (%s)", get_name(), pos.get_str() );
	}
	rotate = (rotate+3)%besch->get_haus()->get_all_layouts();

	for( uint32 i=0;  i<lieferziele.get_count();  i++  ) {
		lieferziele[i].rotate90( y_size );
		// on larger factories the target position changed too
		fabrik_t *fab = get_fab( welt, lieferziele[i] );
		if(  fab  ) {
			lieferziele[i] = fab->get_pos().get_2d();
		}
	}
	for( uint32 i=0;  i<suppliers.get_count();  i++  ) {
		suppliers[i].rotate90( y_size );
		fabrik_t *fab = get_fab( welt, suppliers[i] );
		if(  fab  ) {
			suppliers[i] = fab->get_pos().get_2d();
		}
	}
	for( uint32 i=0;  i<fields.get_count();  i++  ) {
		fields[i].location.rotate90( y_size );
	}
}


void fabrik_t::add_supplier(koord ziel)
{
	suppliers.insert_unique_ordered( ziel, RelativeDistanceOrdering(pos.get_2d()) );
}


void fabrik_t::rem_supplier(koord pos)
{
	suppliers.remove(pos);
}


/** crossconnect everything possible */
void fabrik_t::add_all_suppliers()
{

	for(int i=0; i < besch->get_lieferanten(); i++) {
		const fabrik_lieferant_besch_t *lieferant = besch->get_lieferant(i);
		const ware_besch_t *ware = lieferant->get_ware();

		//const slist_tpl<fabrik_t *> & list = welt->get_fab_list();
		const vector_tpl<fabrik_t*> & list = welt->get_fab_list();
		//slist_iterator_tpl <fabrik_t *> iter (list);

		//while( iter.next() ) {
		for(sint16 i = list.get_count() - 1; i >= 0; i --)
		{
			//fabrik_t * fab = iter.get_current();
			fabrik_t * fab = list[i];

			// connect to an existing one, if this is an producer
			if(fab!=this  &&  fab->vorrat_an(ware) > -1) {
				// add us to this factory
				fab->add_lieferziel(pos.get_2d());
				// and vice versa
				add_supplier(fab->get_pos().get_2d());
			}
		}
	}
}


/* adds a new supplier to this factory
 * fails if no matching goods are there
 */
bool fabrik_t::add_supplier(fabrik_t* fab)
{
	for(int i=0; i < besch->get_lieferanten(); i++) {
		const fabrik_lieferant_besch_t *lieferant = besch->get_lieferant(i);
		const ware_besch_t *ware = lieferant->get_ware();

			// connect to an existing one, if this is an producer
			if(fab!=this  &&  fab->vorrat_an(ware) > -1) { //"inventory to" (Google)
				// add us to this factory
				fab->add_lieferziel(pos.get_2d());
				return true;
			}
	}
	return false;
}

/* adds a new customer to this factory
 * fails if no matching goods are accepted
 */

bool fabrik_t::add_customer(fabrik_t* fab)
{
	for(int i=0; i < fab->get_besch()->get_lieferanten(); i++) {
		const fabrik_lieferant_besch_t *lieferant = fab->get_besch()->get_lieferant(i);
		const ware_besch_t *ware = lieferant->get_ware();

			// connect to an existing one, if it is a consumer
			if(fab!=this  &&  vorrat_an(ware) > -1) { //"inventory to" (Google)
				// add this factory
				add_lieferziel(fab->pos.get_2d());
				return true;
			}
	}
	return false;
}

void fabrik_t::get_tile_list( vector_tpl<koord> &tile_list ) const
{
	tile_list.clear();

	koord pos_2d = pos.get_2d();
	koord size = this->get_besch()->get_haus()->get_groesse(this->get_rotate());
	koord test;
	// Which tiles belong to the fab?
	for( test.x = 0; test.x < size.x; test.x++ ) {
		for( test.y = 0; test.y < size.y; test.y++ ) {
			if( fabrik_t::get_fab( welt, pos_2d+test ) == this ) {
				tile_list.append( pos_2d+test );
			}
		}
	}
}
