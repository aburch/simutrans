#ifndef __HAUSBESCH_H
#define __HAUSBESCH_H

class haus_tile_t;
class haus_besch_data_t;
dfgdfg
/**
 * Diese Klasse beschreibt die Bauparameter eines Gebäudetyps.
 * Klasse ist ein Zeigerklasse, die normal als Wert übergeben wird.
 * @author V. Meyer
 */
class haus_besch_t {
public:
protected:
    haus_besch_data_t *data;

    static haus_besch_data_t nul;

    bool ist_utyp(utyp utype) const
    { return  gib_typ() == gebaeude_t::unbekannt &&  gib_utyp() == utype; }

    haus_besch_t(haus_besch_data_t *data) { this->data = data; }
public:
    inline static haus_besch_t gib_von_tile(haus_tile_t *tile);

    static void init_nul();

    haus_besch_t(tabfileobj_t *obj);
    haus_besch_t();

    static void check_special_ids();

    haus_besch_t(const haus_besch_t &other) { data = other.data; }

    haus_besch_t & operator= (const haus_besch_t &x) { data = x.data; return *this; }
    bool operator== (const haus_besch_t &x) { return data == x.data; }
    bool operator!= (const haus_tile_t &x) { return data != x.data; }

    bool is_bound() const { return data != &nul; }

    const char *gib_name() const
    {   return data->name; }
    koord gib_groesse(int layout = 0) const
    { return (layout & 1) ? koord(data->h, data->b) : koord(data->b, data->h); }
    int gib_h(int layout = 0) const
    { return (layout & 1) ? data->b : data->h; }
    int gib_b(int layout = 0) const
    { return (layout & 1) ? data->h : data->b; }
    int gib_level() const { return data->level; }

    gebaeude_t::typ gib_typ() const { return data->type; }
    utyp gib_utyp() const { return data->utype; }

    haus_tile_t *gib_tile(int layout, int x, int y) const;

    int gib_build_time() const
    { return data->build_time; }

    bool ist_ohne_info() const
    { return data->noinfo; }
    bool ist_fabrik() const
    { return ist_utyp(fabrik); }
    bool ist_ausflugsziel() const
    { return ist_utyp(sehenswuerdigkeit) || ist_utyp(special); }
    bool ist_rathaus() const
    { return ist_utyp(rathaus); }
};

class haus_tile_t {
public:
    short	    img;
    short	    img2;
    unsigned char   height;
    unsigned char   height2;
    unsigned char   anims;
    unsigned char   idx;    // index in data_t structure

    inline haus_besch_t gib_besch();
};

struct haus_besch_data_t {
    gebaeude_t::typ	type;
    utyp		utype;
    int			level;	    // or passengers;
    int			build_time; // == inhabitants at build time
    short		b;
    short		h;
    short		layouts;    // 1 2 or 4
    bool		noinfo;	    // reset flag FLAG_ZEIGE_INFO?
    const char *	name;
    haus_tile_t		tiles[1];   // will be of size dims.x*dims.y*layouts
};

inline static besch_t hausbauer_t::besch_t::gib_von_tile(haus_tile_t *tile)
{ return (haus_besch_data_t *)(tile - tile->idx + 1) - 1;}

inline hausbauer_t::besch_t hausbauer_t::besch_t::haus_tile_t::gib_besch()
{
    return gib_von_tile(this);
}

#endif // __HAUSBESCH_H
