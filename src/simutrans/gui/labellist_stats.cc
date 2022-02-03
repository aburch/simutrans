/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "labellist_stats.h"

#include "components/gui_button.h"
#include "../world/simworld.h"
#include "../player/simplay.h"
#include "../obj/label.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer.h"

#include "../dataobj/environment.h"


labellist::sort_mode_t labellist_stats_t::sortby = labellist::by_name;
bool labellist_stats_t::sortreverse = false;
bool labellist_stats_t::filter = false;

static karte_ptr_t welt;

bool labellist_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	const labellist_stats_t* a = dynamic_cast<const labellist_stats_t*>(aa);
	const labellist_stats_t* b = dynamic_cast<const labellist_stats_t*>(bb);

	int cmp = 0;
	switch (sortby) {
		default: NOT_REACHED
		case labellist::by_name:
		{
			cmp = 0;
			break;
		}
		case labellist::by_koord:
			cmp = a->label_pos.x - b->label_pos.x;
			if(cmp==0) {
				cmp = a->label_pos.y - b->label_pos.y;
			}
			break;
		case labellist::by_player:
		{
			if(!filter) {
				const label_t* a_l = a->get_label();
				const label_t* b_l = b->get_label();
				if(a_l && b_l) {
					cmp = a_l->get_owner()->get_player_nr() - b_l->get_owner()->get_player_nr();
				}
			}
			break;
		}
	}
	if(cmp==0) {
		const char* a_name = a->get_text();
		const char* b_name = b->get_text();

		cmp = strcmp(a_name, b_name);
	}
	return sortreverse ? cmp > 0 : cmp < 0;
}


labellist_stats_t::labellist_stats_t(koord label_pos)
{
	this->label_pos = label_pos;

	set_table_layout(2,1);
	button_t *b = new_component<button_t>();
	b->set_typ(button_t::posbutton_automatic);
	b->set_targetpos(label_pos);

	add_component(&label);
	label.buf().printf("(%s) %s", label_pos.get_str(), get_text());
	label.update();

	if (const label_t *lb = get_label()) {
		label.set_color(PLAYER_FLAG | color_idx_to_rgb(lb->get_owner()->get_player_color1()+env_t::gui_player_color_dark));
	}
}


const label_t* labellist_stats_t::get_label() const
{
	if (grund_t *gr = welt->lookup_kartenboden(label_pos)) {
		return gr->find<label_t>();
	}
	return NULL;
}


void labellist_stats_t::map_rotate90( sint16 y_size )
{
	label_pos.rotate90(y_size);
}


bool labellist_stats_t::is_valid() const
{
	return get_label() != NULL;
}


const char* labellist_stats_t::get_text() const
{
	if (grund_t *gr = welt->lookup_kartenboden(label_pos)) {
		return gr->get_text();
	}
	return "";
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 */
bool labellist_stats_t::infowin_event(const event_t * ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);

	if(  !swallowed  &&  IS_LEFTRELEASE(ev)  ) {
		if (grund_t *gr = welt->lookup_kartenboden(label_pos)) {
			if (label_t *lb = gr->find<label_t>()) {
				lb->show_info();
			}
		}
		swallowed = true;
	}
	return swallowed;
}


/**
 * Draw the component
 */
void labellist_stats_t::draw(scr_coord offset)
{
	label.buf().printf("(%s) %s", label_pos.get_str(), get_text());
	gui_aligned_container_t::draw(offset);
}
