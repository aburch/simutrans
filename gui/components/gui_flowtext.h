#ifndef gui_components_flowtext_h
#define gui_components_flowtext_h

#include "../../ifc/gui_komponente.h"
#include "../../utils/cstring_t.h"

class hyperlink_listener_t;

/**
 * A component for floating text. It does not use any templates on purpose!
 * @author Hj. Malthaner
 */
class gui_flowtext_t : public gui_komponente_t
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


    /**
     * Node for the list of listeners
     * @author Hj. Malthaner
     */
    class listener_t {
    public:
      hyperlink_listener_t *callback;
      listener_t *next;
      listener_t() {next = 0;};
    };

    node_t      * nodes;
    hyperlink_t * links;
    listener_t  * listeners;

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


    void add_listener(hyperlink_listener_t *callback);


    /**
     * Paints the component
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset) const;



    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    virtual void infowin_event(const event_t *);

};

#endif
