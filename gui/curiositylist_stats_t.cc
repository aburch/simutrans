/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "curiositylist_stats_t.h"

#include "../simtypes.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../simskin.h"

#include "../obj/gebaeude.h"

#include "../descriptor/building_desc.h"
#include "../descriptor/skin_desc.h"

#include "../dataobj/translator.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"


#include "../simcity.h"

curiositylist::sort_mode_t curiositylist_stats_t::sortby = curiositylist::by_name;
bool curiositylist_stats_t::sortreverse = false;
bool curiositylist_stats_t::filter_own_network = false;
static karte_ptr_t welt;

bool curiositylist_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	const curiositylist_stats_t* ca = dynamic_cast<const curiositylist_stats_t*>(aa);
	const curiositylist_stats_t* cb = dynamic_cast<const curiositylist_stats_t*>(bb);
	const gebaeude_t* a = ca->attraction;
	const gebaeude_t* b = cb->attraction;

	int cmp = 0;
	switch (sortby) {
		default: NOT_REACHED
		case curiositylist::by_name:
		{
			const char* a_name = translator::translate(a->get_tile()->get_desc()->get_name());
			const char* b_name = translator::translate(b->get_tile()->get_desc()->get_name());
			cmp = STRICMP(a_name, b_name);
			break;
		}

		case curiositylist::by_paxlevel:
			cmp = a->get_adjusted_visitor_demand() - b->get_adjusted_visitor_demand();
			break;

		case curiositylist::by_pax_arrived:
		{
			int a_arrive = a->get_passengers_succeeded_commuting() == 65535 ? a->get_passengers_succeeded_visiting() : a->get_passengers_succeeded_visiting() + a->get_passengers_succeeded_commuting();
			int b_arrive = b->get_passengers_succeeded_commuting() == 65535 ? b->get_passengers_succeeded_visiting() : b->get_passengers_succeeded_visiting() + b->get_passengers_succeeded_commuting();
			cmp = a_arrive - b_arrive;
			break;
		}
		/*
		case curiositylist::by_city:
		{
			const char* a_name = a->get_stadt() ? a->get_stadt()->get_name() : '\0';
			const char* b_name = b->get_stadt() ? b->get_stadt()->get_name() : '\0';
			cmp = STRICMP(a_name, b_name);
			a->get_stadt()->get_zufallspunkt();
			break;
		}*/

		case curiositylist::by_region:
			cmp = welt->get_region(a->get_pos().get_2d()) - welt->get_region(b->get_pos().get_2d());
			break;
	}
	return sortreverse ? cmp > 0 : cmp < 0;
}



curiositylist_stats_t::curiositylist_stats_t(gebaeude_t *att)
{
	attraction = att;
	// pos button
	set_table_layout(8, 1);
	button_t *b = new_component<button_t>();
	b->set_typ(button_t::posbutton_automatic);
	b->set_targetpos(attraction->get_pos().get_2d());

	add_table(2, 1);
	{
		add_component(&img_enabled[0]);
		img_enabled[0].set_image(skinverwaltung_t::passengers->get_image_id(0), true);
		add_component(&img_enabled[1]);
		img_enabled[1].set_image(skinverwaltung_t::mail->get_image_id(0), true);

		img_enabled[0].set_rigid(true);
		img_enabled[1].set_rigid(true);
	}
	end_table();

	// name
	lb_name.set_text(attraction->get_tile()->get_desc()->get_name());
	lb_name.set_min_size(scr_size(D_LABEL_WIDTH*3/2, D_LABEL_HEIGHT));
	add_component(&lb_name);

	// Pakset must have pax evaluation symbols to show overcrowding symbol
	if (skinverwaltung_t::pax_evaluation_icons) {
		img_enabled[2].set_image(skinverwaltung_t::pax_evaluation_icons->get_image_id(1), true);
		img_enabled[2].set_rigid(true);
	}
	else {
		img_enabled[2].set_image(IMG_EMPTY);
	}
	add_component(&img_enabled[2]);

	gui_label_buf_t *l = new_component<gui_label_buf_t>();
	l->buf().printf("(%4d/%4d) ",
		attraction->get_passengers_succeeded_commuting() == 65535 ? attraction->get_passengers_succeeded_visiting() : attraction->get_passengers_succeeded_visiting() + attraction->get_passengers_succeeded_commuting(),
		attraction->get_adjusted_visitor_demand());
	l->update();


	// city attraction images
	add_component(&img_enabled[3]);
	img_enabled[3].set_image(skinverwaltung_t::intown->get_image_id(0), true);
	img_enabled[3].set_rigid(false);

	img_enabled[3].set_visible(attraction->get_tile()->get_desc()->get_extra() != 0);

	// city & region name
	gui_label_buf_t *lb_region = new_component<gui_label_buf_t>();
	if (attraction->get_stadt() != NULL) {
		lb_region->buf().append(attraction->get_stadt()->get_name());
	}
	if (!welt->get_settings().regions.empty()) {
		lb_region->buf().printf(" (%s)", translator::translate(welt->get_region_name(attraction->get_pos().get_2d()).c_str()));
	}
	lb_region->update();

	new_component<gui_fill_t>();
}


const char* curiositylist_stats_t::get_text() const
{
	const unsigned char *name = (const unsigned char *)ltrim( translator::translate(attraction->get_tile()->get_desc()->get_name()) );
	static char short_name[256];
	char* dst = short_name;
	int    cr = 0;
	for( int j=0;  j<255  &&  cr<10;  j++  ) {
		if(name[j]>0  &&  name[j]<=' ') {
			cr++;
			if(  name[j]<32  ) {
				break;
			}
			if (dst != short_name && dst[-1] != ' ') {
				*dst++ = ' ';
			}
		}
		else {
			*dst++ = name[j];
		}
	}
	*dst = '\0';
	// now we have a short name ...
	return short_name;
}


bool curiositylist_stats_t::is_valid() const
{
	return world()->get_attractions().is_contained(attraction);
}


/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 */
bool curiositylist_stats_t::infowin_event(const event_t * ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);
	if (!swallowed  &&  IS_LEFTRELEASE(ev)) {
		attraction->show_info();
		swallowed = true;
	}
	return swallowed;
}


/**
 * Draw the component
 */
void curiositylist_stats_t::draw(scr_coord offset)
{
		// is connected?
		uint8 mail = 0;
		uint8 pax  = 0;
		uint8 pax_crowded = 0;
		const planquadrat_t *plan = welt->access(attraction->get_pos().get_2d());
		const nearby_halt_t *halt_list = plan->get_haltlist();
		for(  unsigned h=0;  h < plan->get_haltlist_count();  h++ ) {
			halthandle_t halt = halt_list[h].halt;
			if (halt->get_pax_enabled()) {
				if (halt->has_available_network(welt->get_active_player(), goods_manager_t::INDEX_PAS)) {
					pax |= own_network;
					if (halt->get_pax_unhappy() > 40) {
						pax_crowded |= own_network;
					}
				}
				else{
					pax |= someones_network;
					if (halt->get_pax_unhappy() > 40) {
						pax_crowded |= someones_network;
					}
				}
			}
			if (halt->get_mail_enabled()) {
				if (halt->has_available_network(welt->get_active_player(), goods_manager_t::INDEX_MAIL)) {
					mail |= own_network;
				}
				else {
					mail |= someones_network;
				}
			}
		}

		img_enabled[0].set_visible(pax & own_network || pax & someones_network);
		if (pax & someones_network) {
			img_enabled[0].set_transparent(TRANSPARENT25_FLAG);
		}

		img_enabled[1].set_visible(mail & own_network || mail & someones_network);
		if (mail & someones_network) {
			img_enabled[1].set_transparent(TRANSPARENT25_FLAG);
		}

		img_enabled[2].set_visible(pax_crowded);
		if (pax_crowded & someones_network) {
			img_enabled[2].set_transparent(TRANSPARENT25_FLAG);
		}

		gui_aligned_container_t::draw(offset);
}
