#ifndef label_h
#define label_h

#include "../simobj.h"
#include "../display/simimg.h"

class label_t : public obj_t
{
public:
	label_t(loadsave_t *file);
	label_t(koord3d pos, spieler_t *sp, const char *text);
	~label_t();

	void laden_abschliessen();

	void zeige_info();

	typ get_typ() const { return obj_t::label; }

	image_id get_bild() const;
};

#endif
