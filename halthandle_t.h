#ifndef halthandle_t_h
#define halthandle_t_h

// #include "tpl/id_handle_tpl.h"
#include "tpl/quickstone_tpl.h"

class haltestelle_t;

//typedef handle_as_id_tpl<haltestelle_t> halthandle_t;
typedef quickstone_tpl<haltestelle_t> halthandle_t;

#endif
