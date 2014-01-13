#ifndef GUI_COMPONENTS_FLOWTEXT_H
#define GUI_COMPONENTS_FLOWTEXT_H

#include <string>

#include "gui_action_creator.h"
#include "gui_komponente.h"
#include "../../tpl/slist_tpl.h"


/**
 * A component for floating text.
 * @author Hj. Malthaner
 */
class gui_flowtext_t :
	public gui_action_creator_t,
	public gui_komponente_t
{
public:
	gui_flowtext_t();

	/**
	 * Sets the text to display.
	 * @author Hj. Malthaner
	 */
	void set_text(const char* text);

	const char* get_title() const;

	scr_size get_preferred_size();

	scr_size get_text_size();

	/**
	 * Paints the component
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord offset);

	bool infowin_event(event_t const*) OVERRIDE;

	bool dirty;
	scr_coord last_offset;

private:
	scr_size output(scr_coord pos, bool doit, bool return_max_width=true);

	enum attributes
	{
		ATT_NONE,
		ATT_NO_SPACE,	// same as none, but no trailing space
		ATT_NEWLINE,
		ATT_A_START,      ATT_A_END,
		ATT_H1_START,     ATT_H1_END,
		ATT_EM_START,     ATT_EM_END,
		ATT_IT_START,     ATT_IT_END,
		ATT_STRONG_START, ATT_STRONG_END,
		ATT_UNKNOWN
	};

	struct node_t
	{
		node_t(const std::string &text_, attributes att_) : text(text_), att(att_) {}

		std::string text;
		attributes att;
	};

	/**
	 * Hyperlink position container
	 * @author Hj. Malthaner
	 */
	struct hyperlink_t
	{
		hyperlink_t(const std::string &param_) : param(param_) {}

		scr_coord    tl;    // top left display position
		scr_coord    br;    // bottom right display position
		std::string  param;
	};

	slist_tpl<node_t>      nodes;
	slist_tpl<hyperlink_t> links;
	char title[128];
};

#endif
