#ifndef __SKIN_READER_H
#define __SKIN_READER_H

#include "obj_reader.h"


class skin_reader_t : public obj_reader_t {

protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;

    virtual skinverwaltung_t::skintyp_t get_skintype() const = 0;
public:
    virtual obj_type get_type() const = 0;
    virtual const char *get_type_name() const = 0;
};


class menuskin_reader_t : public skin_reader_t {
    static menuskin_reader_t the_instance;

    menuskin_reader_t() { register_reader(); }
protected:
    virtual skinverwaltung_t::skintyp_t get_skintype() const { return skinverwaltung_t::menu; }
public:
    static menuskin_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_menu; }
    virtual const char *get_type_name() const { return "menu"; }
};


class cursorskin_reader_t : public skin_reader_t {
    static cursorskin_reader_t the_instance;

    cursorskin_reader_t() { register_reader(); }
protected:
    virtual skinverwaltung_t::skintyp_t get_skintype() const { return skinverwaltung_t::cursor; }
public:
    static cursorskin_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_cursor; }
    virtual const char *get_type_name() const { return "cursor"; }

};


class symbolskin_reader_t : public skin_reader_t {
    static symbolskin_reader_t the_instance;

    symbolskin_reader_t() { register_reader(); }
protected:
    virtual skinverwaltung_t::skintyp_t get_skintype() const { return skinverwaltung_t::symbol; }
public:
    static symbolskin_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_symbol; }
    virtual const char *get_type_name() const { return "symbol"; }

};


class fieldskin_reader_t : public skin_reader_t {
    static fieldskin_reader_t the_instance;

    fieldskin_reader_t() { register_reader(); }
protected:
    virtual skinverwaltung_t::skintyp_t get_skintype() const { return skinverwaltung_t::nothing; }
public:
    static fieldskin_reader_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_field; }
    virtual const char *get_type_name() const { return "field"; }
};


class smoke_reader_t : public skin_reader_t {
    static smoke_reader_t the_instance;

    smoke_reader_t() { register_reader(); }
protected:
    virtual skinverwaltung_t::skintyp_t get_skintype() const {
	return skinverwaltung_t::nothing;
    }
public:
    static smoke_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_smoke; }
    virtual const char *get_type_name() const { return "smoke"; }

};


class miscimages_reader_t : public skin_reader_t {
    static miscimages_reader_t the_instance;

    miscimages_reader_t() { register_reader(); }
protected:
    virtual skinverwaltung_t::skintyp_t get_skintype() const { return skinverwaltung_t::misc; }
public:
    static miscimages_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_miscimages; }
    virtual const char *get_type_name() const { return "misc"; }
};

#endif
