#ifndef __FACTORY_WRITER_H
#define __FACTORY_WRITER_H

#include "obj_writer.h"
#include "../objversion.h"


class factory_smoke_writer_t : public obj_writer_t {
    static factory_smoke_writer_t the_instance;

    factory_smoke_writer_t() { register_writer(false); }
public:
    static factory_smoke_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_fsmoke; }
    virtual const char *get_type_name() const { return "factory smoke"; }

    void write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);
};


class factory_product_writer_t : public obj_writer_t {
    static factory_product_writer_t the_instance;

    factory_product_writer_t() { register_writer(false); }
public:
    static factory_product_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_fproduct; }
    virtual const char *get_type_name() const { return "factory product"; }

    void write_obj(FILE *outfp, obj_node_t &parent, int capacity, int factor, const char *warename);
};


class factory_supplier_writer_t : public obj_writer_t {
    static factory_supplier_writer_t the_instance;

    factory_supplier_writer_t() { register_writer(false); }
public:
    static factory_supplier_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_fsupplier; }
    virtual const char *get_type_name() const { return "factory supplier"; }

    void write_obj(FILE *outfp, obj_node_t &parent, int capacity, int count, int verbrauch, const char *warename);
};


class factory_writer_t : public obj_writer_t {
    static factory_writer_t the_instance;

    factory_writer_t() { register_writer(true); }
protected:
    virtual cstring_t get_node_name(FILE *fp) const;
public:
    virtual void write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj);

    virtual obj_type get_type() const { return obj_factory; }
    virtual const char *get_type_name() const { return "factory"; }

};

#endif // __FACTORY_WRITER_H
