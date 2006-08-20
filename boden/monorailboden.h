#ifndef monorailboden_h
#define monorailboden_h

#include "grund.h"

class monorailboden_t : public grund_t
{
private:
	uint8 slope;

public:
	monorailboden_t(karte_t *welt, loadsave_t *file);
	monorailboden_t(karte_t *welt, koord3d pos);

	virtual void rdwr(loadsave_t *file);

	inline bool ist_bruecke() const { return false; };

	virtual hang_t::typ gib_grund_hang() const { return (hang_t::typ)slope; }

	inline const char *gib_name() const {return "Monorailboden";};
	inline enum typ gib_typ() const {return monorailboden;};

	virtual bool zeige_info();
	void calc_bild();

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
