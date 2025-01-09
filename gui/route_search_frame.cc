#include "route_search_frame.h"
#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "components/gui_divider.h"
#include "components/gui_image.h"
#include "../simconvoi.h"
#include "../simhalt.h"
#include "../simline.h"
#include "../player/simplay.h"
#include "../simware.h"
#include "../simworld.h"
#include <variant>

class gui_traveler_button_t : public button_t, public action_listener_t
{
    linehandle_t line;
public:
    gui_traveler_button_t(linehandle_t line) : button_t()
    {
        this->line = line;
        init(button_t::posbutton, NULL);
        add_listener(this);
    }

    bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE
    {
        player_t *player = world()->get_active_player();
        if(  player == line->get_owner()  ) {
            player->simlinemgmt.show_lineinfo(player, line);
        }
        return true;
    }

    void draw(scr_coord offset) OVERRIDE
    {
        button_t::draw(offset);
        enable(line.is_bound()  &&  line->get_owner() == world()->get_active_player());
    }
};

class gui_halt_button_t : public button_t, public action_listener_t {
    halthandle_t halt;
public:
    gui_halt_button_t(halthandle_t halt) : button_t()
    {
        this->halt = halt;
        init(button_t::posbutton, NULL);
        add_listener(this);
    }

    bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE
    {
        halt->open_info_window();
        return true;
    }
};

route_search_frame_t::route_search_frame_t()
: gui_frame_t( translator::translate("Pax route search") ),
from_halt_label("From:"),
dest_halt_label("To:"),
result_container(1, 0)
{
    // clear the buffer
    snprintf(from_halt_input_text, lengthof(from_halt_input_text), "");
    snprintf(dest_halt_input_text, lengthof(dest_halt_input_text), "");

    set_table_layout(1,0);

    add_table(5, 1);
    {
        from_halt_input.set_text(from_halt_input_text, lengthof(from_halt_input_text));
        dest_halt_input.set_text(dest_halt_input_text, lengthof(dest_halt_input_text));
        add_component(&from_halt_label);
        add_component(&from_halt_input);
        add_component(&dest_halt_label);
        add_component(&dest_halt_input);
    }
    end_table();

    add_table(2, 1);
    {
        search_button.init(button_t::roundbox, "Search");
        search_button.add_listener(this);
        add_component(&search_button);

        reverse_search_button.init(button_t::roundbox, "Swap");
        reverse_search_button.add_listener(this);
        add_component(&reverse_search_button);
    }
    end_table();

    new_component<gui_divider_t>();

    result_container.set_table_layout(1,0);
    add_component(&result_container);
    result_container.new_component<gui_label_t>("Please enter halt names and press search.");

    set_resizemode(diagonal_resize);
    reset_min_windowsize();
}

bool route_search_frame_t::action_triggered(gui_action_creator_t* comp, value_t) {
    if(  comp==&search_button  ) {
        search_route();
    } else if(  comp==&reverse_search_button  ) {
        swap_halt_inputs();
        search_route();
    }
    return true;
}

halthandle_t find_halt(const char* name) {
    FOR(vector_tpl<halthandle_t>, const h, haltestelle_t::get_alle_haltestellen()) {
        if(  strcmp(h->get_name(), name)==0  ) {
            return h;
        }
    }
    return halthandle_t();
}

haltestelle_t::connection_t find_connection(halthandle_t from, halthandle_t to) {
    FOR(vector_tpl<haltestelle_t::connection_t>, const c, from->get_connections(0)) {
        if(  c.halt.is_bound()  &&  c.halt==to  ) {
            return c;
        }
    }
    return haltestelle_t::connection_t();
}

void route_search_frame_t::append_connection_row(haltestelle_t::connection_t connection) {
    if(  connection.weight==0  ) {
        result_container.new_component<gui_label_t>("No connection found!");
        return;
    }
    result_container.add_table(4, 1);

    // construct weight text
    char text[16];
    uint32 weight;
    if(  world()->get_settings().get_time_based_routing_enabled(0)  ) {
        // When TBGR is enabled for pax, weight is in ticks. We have to convert it to the OTRP departure time unit.
        weight = (uint64)connection.weight * world()->get_settings().get_spacing_shift_divisor() / world()->ticks_per_world_month;
    } else {
        // weight is in route cost.
        weight = connection.weight;
    }
    snprintf(text, 16, "<%d>", weight);
    auto label_with_buf = result_container.new_component<gui_label_buf_t>();
    label_with_buf->buf().append(text);

    result_container.new_component<gui_traveler_button_t>(
        std::holds_alternative<linehandle_t>(connection.best_weight_traveler) ? 
        std::get<linehandle_t>(connection.best_weight_traveler) : linehandle_t()
    );

    // Set icon
    const waytype_t waytype = std::visit([&](const auto& t) {
        return t->get_schedule()->get_waytype();
    }, connection.best_weight_traveler);
    gui_image_t *im = NULL;
	switch(waytype) {
		case road_wt: im = result_container.new_component<gui_image_t>(skinverwaltung_t::bushaltsymbol->get_image_id(0)); break;
		case track_wt: im = result_container.new_component<gui_image_t>(skinverwaltung_t::zughaltsymbol->get_image_id(0)); break;
        case tram_wt: im = result_container.new_component<gui_image_t>(skinverwaltung_t::tramhaltsymbol->get_image_id(0)); break;
		case water_wt: im = result_container.new_component<gui_image_t>(skinverwaltung_t::schiffshaltsymbol->get_image_id(0)); break;
		case air_wt: im = result_container.new_component<gui_image_t>(skinverwaltung_t::airhaltsymbol->get_image_id(0)); break;
		case monorail_wt: im = result_container.new_component<gui_image_t>(skinverwaltung_t::monorailhaltsymbol->get_image_id(0)); break;
		case maglev_wt: im = result_container.new_component<gui_image_t>(skinverwaltung_t::maglevhaltsymbol->get_image_id(0)); break;
		case narrowgauge_wt: im = result_container.new_component<gui_image_t>(skinverwaltung_t::narrowgaugehaltsymbol->get_image_id(0)); break;
		default:             result_container.new_component<gui_empty_t>(); break;
	}
	if (im) {
		im->enable_offset_removal(true);
	}

    // Set traveler name
    const char* best_weight_traveler_name = std::visit([&](const auto& t) {
        return t->get_name();
    }, connection.best_weight_traveler);
    const player_t* player = std::visit([&](const auto& t) {
        return t->get_owner();
    }, connection.best_weight_traveler);
    const uint8 color_idx = player->get_player_color1();
    result_container.new_component<gui_label_t>(best_weight_traveler_name, color_idx_to_rgb(color_idx));

    result_container.end_table();
}

void route_search_frame_t::append_halt_row(halthandle_t halt) {
    result_container.add_table(2, 1);
    result_container.new_component<gui_halt_button_t>(halt);
    result_container.new_component<gui_label_t>(halt->get_name());
    result_container.end_table();
}

void route_search_frame_t::search_route() {
    result_container.remove_all();
    halthandle_t from_halt = find_halt(from_halt_input.get_text());
    if(  !from_halt.is_bound()  ) {
        result_container.new_component<gui_label_t>("From halt not found.");
        return;
    }
    halthandle_t dest_halt = find_halt(dest_halt_input.get_text());
    if(  !dest_halt.is_bound()  ) {
        result_container.new_component<gui_label_t>("To halt not found.");
        return;
    }
    ware_t dummy_ware = ware_t();
    dummy_ware.menge = 100;
    dummy_ware.index = 0;
    dummy_ware.set_zielpos(dest_halt->get_init_pos());
    halthandle_t start_halt_array[1] = {from_halt};
    haltestelle_t::search_route(start_halt_array, 1, false, dummy_ware);

    if(  !dummy_ware.get_ziel().is_bound()  ) {
        result_container.new_component<gui_label_t>("No route found!");
        return;
    }

    append_halt_row(from_halt);
    halthandle_t transit_from = from_halt;
    FOR(vector_tpl<halthandle_t>, const h, dummy_ware.get_transit_halts()) {
        // Fetch the fastest traveler
        auto connection = find_connection(transit_from, h);
        append_connection_row(connection);
        append_halt_row(h);
        transit_from = h;
    }

    reset_min_windowsize();
}

void route_search_frame_t::swap_halt_inputs() {
    char temp[256];
    strcpy(temp, from_halt_input_text);
    strcpy(from_halt_input_text, dest_halt_input_text);
    strcpy(dest_halt_input_text, temp);
    from_halt_input.set_text(from_halt_input_text, lengthof(from_halt_input_text));
    dest_halt_input.set_text(dest_halt_input_text, lengthof(dest_halt_input_text));
}
