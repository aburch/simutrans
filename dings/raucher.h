#ifndef __RAUCHER_H
#define __RAUCHER_H

class karte_t;
class ding_t;
class rauch_besch_t;
template <class X> class stringhashtable_tpl;


class raucher_t : public ding_t
{
private:
	static stringhashtable_tpl<const rauch_besch_t *> besch_table;
	static const rauch_besch_t *gib_besch(const char *name);

	const rauch_besch_t *besch;
	uint8 zustand:1;

public:
	static void register_besch(const rauch_besch_t *besch, const char *name);

	raucher_t(karte_t *welt, loadsave_t *file);
	raucher_t(karte_t *welt, koord3d pos, const rauch_besch_t *besch);

	void set_active( bool state ) { zustand=state; }

	bool step(long delta_t);
	void zeige_info();

	void rdwr(loadsave_t *file);

	enum ding_t::typ gib_typ() const { return raucher; }
};

#endif
