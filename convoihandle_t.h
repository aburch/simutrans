#ifndef convoihandle_t_h
#define convoihandle_t_h

//#include "tpl/id_handle_tpl.h"
#include "tpl/quickstone_tpl.h"

class convoi_t;

//typedef handle_as_id_tpl<convoi_t> convoihandle_t;
typedef quickstone_tpl<convoi_t> convoihandle_t;

#endif
