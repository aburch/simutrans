#ifndef dings_wolke_t
#define dings_wolke_t



#include "../ifc/sync_steppable.h"
#include "../simimg.h"

class karte_t;
class rauch_besch_t;
template <class X> class stringhashtable_tpl;


/**
 * smoke clouds (formerly sync_wolke_t)
 * @author Hj. Malthaner
 */
class wolke_t : public ding_t, public sync_steppable
{
private:
	static stringhashtable_tpl<const rauch_besch_t *> besch_table;
	static const rauch_besch_t *gib_besch(const char *name);

public:
	static void register_besch(const rauch_besch_t *besch, const char *name);

private:
	sint32 insta_zeit;	// Wolken verschwinden, wenn alter max. erreicht
	uint8 base_y_off;
	uint8 increment:1;
	image_id base_image;

public:
	inline sint32 gib_insta_zeit() const { return insta_zeit; }

	wolke_t(karte_t *welt, loadsave_t *file);
	wolke_t(karte_t *welt, koord3d pos, sint8 xoff, sint8 yoff, image_id bild, bool increment);
	~wolke_t();

	bool sync_step(long delta_t);

	const char* gib_name() const { return "Wolke"; }
	enum ding_t::typ gib_typ() const { return sync_wolke; }

	void zeige_info() {} // show no info

	image_id gib_bild() const { return base_image + increment*(insta_zeit >> 9); }

	void rdwr(loadsave_t *file);

	virtual void entferne(spieler_t *sp);
};


/**
 * follwoing two classes are just for compatibility for old save games
 */
class async_wolke_t : public ding_t
{
public:
	async_wolke_t(karte_t *welt, loadsave_t *file);
	enum ding_t::typ gib_typ() const { return async_wolke; }
	image_id gib_bild() const { return IMG_LEER; }
};

class raucher_t : public ding_t
{
public:
	raucher_t(karte_t *welt, loadsave_t *file);
	enum ding_t::typ gib_typ() const { return raucher; }
	image_id gib_bild() const { return IMG_LEER; }
};

#endif
