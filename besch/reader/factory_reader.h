#ifndef __FACTORY_READER_H
#define __FACTORY_READER_H

#include "obj_reader.h"


// Knightly : new reader for field class besch
class factory_field_class_reader_t : public obj_reader_t {
	friend class factory_field_reader_t;	// Knightly : this is a special case due to besch restructuring

    static factory_field_class_reader_t the_instance;

    factory_field_class_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
public:
    static factory_field_class_reader_t *instance() { return &the_instance; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_ffldclass; }
    virtual const char *get_type_name() const { return "factory field class"; }
};


class factory_field_reader_t : public obj_reader_t {
    static factory_field_reader_t the_instance;

    factory_field_reader_t() { register_reader(); }

	// hold a field class besch under construction
	static obj_besch_t *incomplete_field_class_besch;

protected:
    virtual void register_obj(obj_besch_t *&data);
public:
    static factory_field_reader_t *instance() { return &the_instance; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_ffield; }
    virtual const char *get_type_name() const { return "factory field"; }
};


class factory_smoke_reader_t : public obj_reader_t {
    static factory_smoke_reader_t the_instance;

    factory_smoke_reader_t() { register_reader(); }

public:
    static factory_smoke_reader_t*instance() { return &the_instance; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_fsmoke; }
    virtual const char *get_type_name() const { return "factory smoke"; }
};


class factory_supplier_reader_t : public obj_reader_t {
    static factory_supplier_reader_t the_instance;

    factory_supplier_reader_t() { register_reader(); }
public:
    static factory_supplier_reader_t*instance() { return &the_instance; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_fsupplier; }
    virtual const char *get_type_name() const { return "factory supplier"; }
};


class factory_product_reader_t : public obj_reader_t {
    static factory_product_reader_t the_instance;

    factory_product_reader_t() { register_reader(); }
public:
    static factory_product_reader_t*instance() { return &the_instance; }

    /**
     * Read a factory product node. Does version check and
     * compatibility transformations.
     * @author Hj. Malthaner
     */
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_fproduct; }
    virtual const char *get_type_name() const { return "factory product"; }
};


class factory_reader_t : public obj_reader_t {
    static factory_reader_t the_instance;

    factory_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
public:

    static factory_reader_t*instance() { return &the_instance; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_factory; }
    virtual const char *get_type_name() const { return "factory"; }
};

#endif
