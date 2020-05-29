#ifndef gui_convoy_payloadinfo_h
#define gui_convoy_payloadinfo_h

#include "../../display/scr_coord.h"
#include "gui_container.h"
#include "../../convoihandle_t.h"

// Display convoy's payload information along with goods category symbol. @Ranan 2020
class gui_convoy_payloadinfo_t : public gui_container_t
{
private:
	convoihandle_t cnv;

public:
	gui_convoy_payloadinfo_t(convoihandle_t cnv);

	void set_cnv(convoihandle_t c) { cnv = c; }

	void draw(scr_coord offset);
};

#endif
