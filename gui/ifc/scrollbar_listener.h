#ifndef gui_ifc_scrollbar_lauscher_h
#define gui_ifc_scrollbar_lauscher_h

class scrollbar_t;

/**
 * This interface must be implemented by all classes which want to
 * listen for scrollbar moves/changes
 * @author Niels Roest, Hj. Malthaner
 */
class scrollbar_listener_t
{
public:

    /**
     * This method is called if the slider is moved.
     * The value ranges from 0 to (and including) range.
     * @author Niels Roest, Hj. Malthaner
     */
    virtual void scrollbar_moved(scrollbar_t *sb, int range, int value) = 0;
};

#endif
