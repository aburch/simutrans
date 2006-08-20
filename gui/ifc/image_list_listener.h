#ifndef gui_ifc_image_list_lauscher_h
#define gui_ifc_image_list_lauscher_h


class image_list_t;

/**
 * Interface für Lauscher auf einem image_list_t
 * @see image_list_t
 * @author Hj. Malthaner
 * @version $Revision: 1.3 $
 */
class image_list_listener_t
{
public:
    virtual void bild_gewaehlt(image_list_t *, int bild_index) = 0;
};

#endif
