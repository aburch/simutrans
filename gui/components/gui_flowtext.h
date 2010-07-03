#ifndef GUI_COMPONENTS_FLOWTEXT_H
#define GUI_COMPONENTS_FLOWTEXT_H

#include <string>

#include "../../ifc/gui_action_creator.h"
#include "gui_komponente.h"
#include "../../tpl/slist_tpl.h"
#include "action_listener.h"


/**
 * A component for floating text. It does not use any templates on purpose!
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

	koord get_preferred_size();

	/**
	 * Paints the component
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord offset);

	/**
	 * Events werden hiermit an die GUI-Komponenten gemeldet
	 * @author Hj. Malthaner
	 */
	bool infowin_event(const event_t*);

	bool dirty;
	koord last_offset;

private:
	koord output(koord pos, bool doit);

	enum attributes
	{
		ATT_NONE,
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
		node_t(const std::string &text_, attributes att_) : text(text_), att(att_), next(0) {}

		std::string text;
		attributes att;
		node_t*    next;
	};

	/**
	 * Hyperlink position container
	 * @author Hj. Malthaner
	 */
	struct hyperlink_t
	{
		hyperlink_t(const std::string &param_) : param(param_), next(0) {}

		koord        tl;    // top left display position
		koord        br;    // bottom right display position
		std::string  param;
		hyperlink_t* next;
	};

	slist_tpl<node_t>      nodes;
	slist_tpl<hyperlink_t> links;
	char title[128];
};

#endif
