#ifndef __OBJ_PAK_EXCEPTION_H
#define __OBJ_PAK_EXCEPTION_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class obj_pak_exception_t {
    cstring_t   classname;
    cstring_t   problem;
public:
    obj_pak_exception_t(const char *clsname, const char *prob)
    {
	classname = clsname;
	problem = prob;
    }
    const cstring_t &get_class()
    {
	return classname;
    }
    const cstring_t &get_info()
    {
	return problem;
    }
};

#endif // __OBJ_PAK_EXCEPTION_H
