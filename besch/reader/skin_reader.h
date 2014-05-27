#ifndef __SKIN_READER_H
#define __SKIN_READER_H

#include "obj_reader.h"


class skin_reader_t : public obj_reader_t {
public:
	obj_besch_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;

protected:
	void register_obj(obj_besch_t*&) OVERRIDE;
	bool successfully_loaded() const OVERRIDE;

	virtual skinverwaltung_t::skintyp_t get_skintype() const = 0;
};


class menuskin_reader_t : public skin_reader_t {
	static menuskin_reader_t the_instance;

	menuskin_reader_t() { register_reader(); }
protected:
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::menu; }
public:
	static menuskin_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_menu; }
	char const* get_type_name() const OVERRIDE { return "menu"; }
};


class cursorskin_reader_t : public skin_reader_t {
	static cursorskin_reader_t the_instance;

	cursorskin_reader_t() { register_reader(); }
protected:
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::cursor; }
public:
	static cursorskin_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_cursor; }
	char const* get_type_name() const OVERRIDE { return "cursor"; }

};


class symbolskin_reader_t : public skin_reader_t {
	static symbolskin_reader_t the_instance;

	symbolskin_reader_t() { register_reader(); }
protected:
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::symbol; }
public:
	static symbolskin_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_symbol; }
	char const* get_type_name() const OVERRIDE { return "symbol"; }

};


class fieldskin_reader_t : public skin_reader_t {
	static fieldskin_reader_t the_instance;

	fieldskin_reader_t() { register_reader(); }
protected:
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::nothing; }
public:
	static fieldskin_reader_t *instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_field; }
	char const* get_type_name() const OVERRIDE { return "field"; }
};


class smoke_reader_t : public skin_reader_t {
	static smoke_reader_t the_instance;

	smoke_reader_t() { register_reader(); }
protected:
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::nothing; }

public:
	static smoke_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_smoke; }
	char const* get_type_name() const OVERRIDE { return "smoke"; }

};


class miscimages_reader_t : public skin_reader_t {
	static miscimages_reader_t the_instance;

	miscimages_reader_t() { register_reader(); }
protected:
	skinverwaltung_t::skintyp_t get_skintype() const OVERRIDE { return skinverwaltung_t::misc; }
public:
	static miscimages_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_miscimages; }
	char const* get_type_name() const OVERRIDE { return "misc"; }
};

#endif
