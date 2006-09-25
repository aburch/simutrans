#ifndef __SOUND_READER_H
#define __SOUND_READER_H

#include "obj_reader.h"


class sound_reader_t : public obj_reader_t {
    static sound_reader_t the_instance;

	sound_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;
public:
	virtual ~sound_reader_t() {}
    static sound_reader_t*instance() { return &the_instance; }

    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_sound; }
    virtual const char *get_type_name() const { return "sound"; }
};

#endif // __SOUND_READER_H
