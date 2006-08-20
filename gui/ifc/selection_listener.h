#ifndef gui_ifc_selection_listener_h
#define gui_ifc_selection_listener_h


class gui_komponente_t;

/**
 * Interface für Lauscher auf Komponenten mit selektierbaren Einträgen
 * @param c die Komponente auf der gewählt wurde
 * @param index der ausgewählte Index
 * @see scrolled_list_t
 * @author Hj. Malthaner
 * @version $Revision: 1.2 $
 */
class selection_listener_t
{
public:
    virtual void eintrag_gewaehlt(gui_komponente_t *c, int index) = 0;
};

#endif
