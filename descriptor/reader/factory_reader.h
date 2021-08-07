/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_FACTORY_READER_H
#define DESCRIPTOR_READER_FACTORY_READER_H


#include "obj_reader.h"


class field_class_desc_t;

// new reader for field class desc
class factory_field_class_reader_t : public obj_reader_t {
	friend class factory_field_group_reader_t; // this is a special case due to desc restructuring

	static factory_field_class_reader_t the_instance;

	factory_field_class_reader_t() { register_reader(); }
public:
	static factory_field_class_reader_t *instance() { return &the_instance; }

	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_ffldclass; }
	char const* get_type_name() const OVERRIDE { return "factory field class"; }
};


class factory_field_group_reader_t : public obj_reader_t {
	static factory_field_group_reader_t the_instance;

	factory_field_group_reader_t() { register_reader(); }

	// hold a field class desc under construction
	static field_class_desc_t* incomplete_field_class_desc;

protected:
	void register_obj(obj_desc_t*&) OVERRIDE;
public:
	static factory_field_group_reader_t *instance() { return &the_instance; }

	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_ffield; }
	char const* get_type_name() const OVERRIDE { return "factory field"; }
};


class factory_smoke_reader_t : public obj_reader_t {
	static factory_smoke_reader_t the_instance;

	factory_smoke_reader_t() { register_reader(); }

public:
	static factory_smoke_reader_t*instance() { return &the_instance; }

	obj_desc_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_fsmoke; }
	char const* get_type_name() const OVERRIDE { return "factory smoke"; }
};


class factory_supplier_reader_t : public obj_reader_t {
	static factory_supplier_reader_t the_instance;

	factory_supplier_reader_t() { register_reader(); }
public:
	static factory_supplier_reader_t*instance() { return &the_instance; }

	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_fsupplier; }
	char const* get_type_name() const OVERRIDE { return "factory supplier"; }
};


class factory_product_reader_t : public obj_reader_t {
	static factory_product_reader_t the_instance;

	factory_product_reader_t() { register_reader(); }
public:
	static factory_product_reader_t*instance() { return &the_instance; }

	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_fproduct; }
	char const* get_type_name() const OVERRIDE { return "factory product"; }
};


class factory_reader_t : public obj_reader_t {
	static factory_reader_t the_instance;

	factory_reader_t() { register_reader(); }
protected:
	void register_obj(obj_desc_t*&) OVERRIDE;
	bool successfully_loaded() const OVERRIDE;

public:

	static factory_reader_t*instance() { return &the_instance; }

	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_factory; }
	char const* get_type_name() const OVERRIDE { return "factory"; }
};

#endif
