/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_FACTORY_READER_H
#define DESCRIPTOR_READER_FACTORY_READER_H


#include "obj_reader.h"


class field_class_desc_t;

// new reader for field class desc
class factory_field_class_reader_t : public obj_reader_t
{
	friend class factory_field_group_reader_t; // this is a special case due to desc restructuring

	OBJ_READER_DEF(factory_field_class_reader_t, obj_ffldclass, "factory field class");

public:
	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};


class factory_field_group_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(factory_field_group_reader_t, obj_ffield, "factory field");

	// hold a field class desc under construction
	static field_class_desc_t* incomplete_field_class_desc;

protected:
	void register_obj(obj_desc_t*&) OVERRIDE;

public:
	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};


class factory_smoke_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(factory_smoke_reader_t, obj_fsmoke, "factory smoke");

public:
	obj_desc_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;
};


class factory_supplier_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(factory_supplier_reader_t, obj_fsupplier, "factory supplier");

public:
	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};


class factory_product_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(factory_product_reader_t, obj_fproduct, "factory product");

public:
	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};


class factory_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(factory_reader_t, obj_factory, "factory");

protected:
	void register_obj(obj_desc_t*&) OVERRIDE;
	bool successfully_loaded() const OVERRIDE;

public:
	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};


#endif
