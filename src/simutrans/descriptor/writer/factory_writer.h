/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_FACTORY_WRITER_H
#define DESCRIPTOR_WRITER_FACTORY_WRITER_H


#include <string>
#include "obj_writer.h"
#include "../objversion.h"


// new writer class for field class desc
class factory_field_class_writer_t : public obj_writer_t
{
private:
	static factory_field_class_writer_t the_instance;

	factory_field_class_writer_t() { register_writer(false); }

public:
	static factory_field_class_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_ffldclass; }
	const char* get_type_name() const OVERRIDE { return "factory field class"; }

	void write_obj(FILE* fp, obj_node_t& parent, const char* field_name, int snow_image, int production, int capacity, int weight);
};


class factory_field_group_writer_t : public obj_writer_t
{
private:
	static factory_field_group_writer_t the_instance;

	factory_field_group_writer_t() { register_writer(false); }

public:
	static factory_field_group_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_ffield; }
	const char *get_type_name() const OVERRIDE { return "factory field"; }

	void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj) OVERRIDE;
};


class factory_smoke_writer_t : public obj_writer_t
{
private:
	static factory_smoke_writer_t the_instance;

	factory_smoke_writer_t() { register_writer(false); }

public:
	static factory_smoke_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_fsmoke; }
	const char *get_type_name() const OVERRIDE { return "factory smoke"; }

	void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj) OVERRIDE;
};


class factory_product_writer_t : public obj_writer_t
{
private:
	static factory_product_writer_t the_instance;

	factory_product_writer_t() { register_writer(false); }

public:
	static factory_product_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_fproduct; }
	const char* get_type_name() const OVERRIDE { return "factory product"; }

	void write_obj(FILE* outfp, obj_node_t& parent, int capacity, int factor, const char* warename);
};


class factory_supplier_writer_t : public obj_writer_t
{
private:
	static factory_supplier_writer_t the_instance;

	factory_supplier_writer_t() { register_writer(false); }

public:
	static factory_supplier_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_fsupplier; }
	const char* get_type_name() const OVERRIDE { return "factory supplier"; }

	void write_obj(FILE* outfp, obj_node_t& parent, int capacity, int count, int consumption, const char* warename);
};


class factory_writer_t : public obj_writer_t
{
private:
	static factory_writer_t the_instance;

	factory_writer_t() { register_writer(true); }

protected:
	std::string get_node_name(FILE* fp) const OVERRIDE;

public:
	void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_factory; }
	const char* get_type_name() const OVERRIDE { return "factory"; }
};


#endif
