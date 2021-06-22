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
	virtual std::string get_node_name(FILE* fp) const { return name_from_next_node(fp); }

public:
	virtual void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj);
	void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj, const slist_tpl<std::string>& imagekeys);

	virtual obj_type get_type() const = 0;
	virtual const char* get_type_name() const = 0;
};


class menuskin_writer_t : public skin_writer_t
{
private:
	static menuskin_writer_t the_instance;

	menuskin_writer_t() { register_writer(true); }

public:
	static menuskin_writer_t* instance() { return &the_instance; }

	virtual obj_type get_type() const { return obj_menu; }
	virtual const char* get_type_name() const { return "menu"; }
};


class cursorskin_writer_t : public skin_writer_t
{
private:
	static cursorskin_writer_t the_instance;

	cursorskin_writer_t() { register_writer(true); }

public:
	static cursorskin_writer_t* instance() { return &the_instance; }

	virtual obj_type get_type() const { return obj_cursor; }
	virtual const char* get_type_name() const { return "cursor"; }
};


class symbolskin_writer_t : public skin_writer_t
{
private:
	static symbolskin_writer_t the_instance;

	symbolskin_writer_t() { register_writer(true); }

public:
	static symbolskin_writer_t* instance() { return &the_instance; }

	virtual obj_type get_type() const { return obj_symbol; }
	virtual const char* get_type_name() const { return "symbol"; }
};


class smoke_writer_t : public skin_writer_t
{
private:
	static smoke_writer_t the_instance;

	smoke_writer_t() { register_writer(true); }

public:
	static smoke_writer_t* instance() { return &the_instance; }

	virtual obj_type get_type() const { return obj_smoke; }
	virtual const char* get_type_name() const { return "smoke"; }
};


class field_writer_t : public skin_writer_t
{
private:
	static field_writer_t the_instance;

	field_writer_t() { register_writer(true); }

public:
	static field_writer_t* instance() { return &the_instance; }

	virtual obj_type get_type() const { return obj_field; }
	virtual const char* get_type_name() const { return "field"; }
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

	virtual obj_type get_type() const { return obj_miscimages; }
	virtual const char* get_type_name() const { return "misc"; }
};


#endif
