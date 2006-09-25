#ifndef gui_components_flowtext_h
#define gui_components_flowtext_h

#include "../../ifc/gui_action_creator.h"
#include "action_listener.h"
#include "../../utils/cstring_t.h"


/**
 * A component for floating text. It does not use any templates on purpose!
 * @author Hj. Malthaner
 */
class gui_flowtext_t : public gui_komponente_action_creator_t
{
private:

    enum attributes {
	ATT_NONE, ATT_NEWLINE,
	ATT_A_START, ATT_A_END,
	ATT_H1_START, ATT_H1_END,
	ATT_EM_START, ATT_EM_END,
	ATT_STRONG_START, ATT_STRONG_END,
	ATT_UNKNOWN};

    class node_t {
    public:

       cstring_t text;
       int att;

       node_t * next;

       node_t(const cstring_t &text, int att);
    };


    /**
     * Hyperlink position conatiner
     * @author Hj. Malthaner
     */
    class hyperlink_t {
    public:
      koord tl;     // top left display position
      koord br;     // bottom right display position
      cstring_t param;
      hyperlink_t * next;

      hyperlink_t() {next = 0;};
    };


    node_t      * nodes;
    hyperlink_t * links;

    koord output(koord pos, bool doit) const;


    char title[128];

public:

    gui_flowtext_t();

    ~gui_flowtext_t();

    /**
     * Sets the text to display.
     * @author Hj. Malthaner
     */
    void set_text(const char *text);

    const char * get_title() const;

    koord get_preferred_size() const;

    /**
     * Paints the component
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset);

    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    void infowin_event(const event_t *);
};

#endif
