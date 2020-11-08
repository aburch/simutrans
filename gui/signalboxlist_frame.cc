// ****************** List of all signalboxes ************************


#include "signalboxlist_frame.h"
#include "gui_theme.h"
#include "../simsignalbox.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "../descriptor/skin_desc.h"
#include "../descriptor/building_desc.h"
#include "../utils/simstring.h"

enum sort_mode_t { by_type, by_coord, by_connected, by_capacity, by_radius, by_region, SORT_MODES };

int signalboxlist_stats_t::sort_mode = by_connected;
bool signalboxlist_stats_t::reverse = false;

static karte_ptr_t welt;

signalboxlist_stats_t::signalboxlist_stats_t(signalbox_t *sb)
{
	this->sb = sb;
	// pos button
	set_table_layout(5,1);
	gotopos.set_typ(button_t::posbutton_automatic);
	gotopos.set_targetpos(sb->get_pos().get_2d());
	add_component(&gotopos);
	label.set_min_size(scr_size(D_LABEL_WIDTH * 3 / 2, D_LABEL_HEIGHT));
	add_component(&label);

	add_component(&lb_connected);
	add_component(&lb_region);
	new_component<gui_fill_t>();

	update_label();
}


void signalboxlist_stats_t::update_label()
{
	// name
	cbuffer_t &buf = label.buf();
	buf.append( translator::translate(sb->get_name()) );
	label.update();

	// connected / capacity
	lb_connected.buf().printf(" (%3d/%3d)",
		sb->get_number_of_signals_controlled_from_this_box(),
		sb->get_first_tile()->get_tile()->get_desc()->get_capacity());
	lb_connected.update();

	// region name (pos)
	if (!welt->get_settings().regions.empty()) {
		lb_region.buf().printf("%s ", translator::translate(welt->get_region_name(sb->get_pos().get_2d()).c_str()));
	}
	lb_region.buf().printf("%s", sb->get_pos().get_2d().get_fullstr());
	lb_region.update();
}


void signalboxlist_stats_t::set_size(scr_size size)
{
	gui_aligned_container_t::set_size(size);
	label.set_size(scr_size(get_size().w - label.get_pos().x, label.get_size().h));
}


bool signalboxlist_stats_t::is_valid() const
{
	return signalbox_t::all_signalboxes.is_contained(sb);
}


bool signalboxlist_stats_t::infowin_event(const event_t * ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);

	if(  !swallowed  &&  IS_LEFTRELEASE(ev)  ) {
		sb->show_info();
		swallowed = true;
	}
	return swallowed;
}


void signalboxlist_stats_t::draw(scr_coord pos)
{
	update_label();

	gui_aligned_container_t::draw(pos);
}


bool signalboxlist_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	const signalboxlist_stats_t* fa = dynamic_cast<const signalboxlist_stats_t*>(aa);
	const signalboxlist_stats_t* fb = dynamic_cast<const signalboxlist_stats_t*>(bb);
	// good luck with mixed lists
	assert(fa != NULL  &&  fb != NULL);
	signalbox_t *a=fa->sb, *b=fb->sb;

	int cmp;
	switch( sort_mode ) {
		default:
		case by_type:
		{
			const char* a_name = translator::translate(a->get_name());
			const char* b_name = translator::translate(b->get_name());
			cmp = STRICMP(a_name, b_name);
			break;
		}
		case by_coord:
			cmp = 0;
			break;
		case by_connected:
			cmp = a->get_number_of_signals_controlled_from_this_box() - b->get_number_of_signals_controlled_from_this_box();
			break;
		case by_capacity:
			cmp = a->get_first_tile()->get_tile()->get_desc()->get_capacity() - b->get_first_tile()->get_tile()->get_desc()->get_capacity();
			break;
		case by_radius:
			cmp = a->get_first_tile()->get_tile()->get_desc()->get_radius() - b->get_first_tile()->get_tile()->get_desc()->get_radius();
			break;
		case by_region:
			cmp = welt->get_region(a->get_pos().get_2d()) - welt->get_region(b->get_pos().get_2d());
			break;
	}
	if (cmp == 0) {
		cmp = koord_distance( a->get_pos(), koord( 0, 0 ) ) - koord_distance( b->get_pos(), koord( 0, 0 ) );
		if( cmp == 0 ) {
			cmp = a->get_pos().x - b->get_pos().x;
		}
	}
	return reverse ? cmp > 0 : cmp < 0;
}




static const char *sort_text[SORT_MODES] = {
	"sb_type",
	"koord",
	"sb_connected",
	"Max. signals",
	"Radius",
	"by_region"
};

signalboxlist_frame_t::signalboxlist_frame_t(player_t *player) :
	gui_frame_t( translator::translate("sb_title"), player ),
	scrolly(gui_scrolled_list_t::windowskin, signalboxlist_stats_t::compare)
{
	this->player = player;
	last_signalbox_count = 0;

	set_table_layout(1,0);
	new_component<gui_label_t>("hl_txt_sort");

	add_table(4,0);
	{
		sortedby.clear_elements();
		for (int i = 0; i < SORT_MODES; i++) {
			sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
		}
		sortedby.set_selection(signalboxlist_stats_t::sort_mode);
		sortedby.add_listener(this);
		add_component(&sortedby);

		sort_asc.init(button_t::arrowup_state, "");
		sort_asc.set_tooltip(translator::translate("hl_btn_sort_asc"));
		sort_asc.add_listener(this);
		sort_asc.pressed = signalboxlist_stats_t::reverse;
		add_component(&sort_asc);

		sort_desc.init(button_t::arrowdown_state, "");
		sort_desc.set_tooltip(translator::translate("hl_btn_sort_desc"));
		sort_desc.add_listener(this);
		sort_desc.pressed = !signalboxlist_stats_t::reverse;
		add_component(&sort_desc);
		new_component<gui_margin_t>(LINESPACE);
	}
	end_table();

	add_component(&scrolly);
	fill_list();

	set_resizemode(diagonal_resize);
	scrolly.set_maximize(true);
	reset_min_windowsize();
}


/**
 * This method is called if an action is triggered
 */
bool signalboxlist_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if(comp == &sortedby) {
		signalboxlist_stats_t::sort_mode = max(0, v.i);
		scrolly.sort(0);
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		signalboxlist_stats_t::reverse = !signalboxlist_stats_t::reverse;
		scrolly.sort(0);
		sort_asc.pressed = signalboxlist_stats_t::reverse;
		sort_desc.pressed = !signalboxlist_stats_t::reverse;
	}
	return true;
}


void signalboxlist_frame_t::fill_list()
{
	scrolly.clear_elements();
	FOR(slist_tpl<signalbox_t*>, const sigb, signalbox_t::all_signalboxes) {
		if(sigb->get_owner() == player ) {
			scrolly.new_component<signalboxlist_stats_t>( sigb );
		}
	}
	scrolly.sort(0);
	scrolly.set_size( scrolly.get_size());

	last_signalbox_count = signalbox_t::all_signalboxes.get_count();
}


void signalboxlist_frame_t::draw(scr_coord pos, scr_size size)
{
	if(  signalbox_t::all_signalboxes.get_count() != last_signalbox_count  ) {
		fill_list();
	}

	gui_frame_t::draw(pos,size);
}
