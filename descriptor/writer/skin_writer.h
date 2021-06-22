/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_SKIN_WRITER_H
#define DESCRIPTOR_WRITER_SKIN_WRITER_H


#include "obj_writer.h"

#include "../objversion.h"
#include "../../tpl/slist_tpl.h"

#include <string>


class skin_writer_t : public obj_writer_t
{
protected:
	std::string get_node_name(FILE* fp) const OVERRIDE { return name_from_next_node(fp); }

public:
	void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj) OVERRIDE;
	void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj, const slist_tpl<std::string>& imagekeys);

	obj_type get_type() const OVERRIDE = 0;
	const char* get_type_name() const OVERRIDE = 0;
};


class menuskin_writer_t : public skin_writer_t
{
private:
	static menuskin_writer_t the_instance;

	menuskin_writer_t() { register_writer(true); }

public:
	static menuskin_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_menu; }
	const char* get_type_name() const OVERRIDE { return "menu"; }
};


class cursorskin_writer_t : public skin_writer_t
{
private:
	static cursorskin_writer_t the_instance;

	cursorskin_writer_t() { register_writer(true); }

public:
	static cursorskin_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_cursor; }
	const char* get_type_name() const OVERRIDE { return "cursor"; }
};


class symbolskin_writer_t : public skin_writer_t
{
private:
	static symbolskin_writer_t the_instance;

	symbolskin_writer_t() { register_writer(true); }

public:
	static symbolskin_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_symbol; }
	const char* get_type_name() const OVERRIDE { return "symbol"; }
};


class smoke_writer_t : public skin_writer_t
{
private:
	static smoke_writer_t the_instance;

	smoke_writer_t() { register_writer(true); }

public:
	static smoke_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_smoke; }
	const char* get_type_name() const OVERRIDE { return "smoke"; }
};


class field_writer_t : public skin_writer_t
{
private:
	static field_writer_t the_instance;

	field_writer_t() { register_writer(true); }

public:
	static field_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_field; }
	const char* get_type_name() const OVERRIDE { return "field"; }
};


/*
 * Used for images needed by the game but not yet integrated as real objects
 */
class miscimages_writer_t : public skin_writer_t
{
private:
	static miscimages_writer_t the_instance;

	miscimages_writer_t() { register_writer(true); }

public:
	static miscimages_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_miscimages; }
	const char* get_type_name() const OVERRIDE { return "misc"; }
};


#endif
