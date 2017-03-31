#ifndef OBJ_PAK_EXCEPTION_H
#define OBJ_PAK_EXCEPTION_H

#include <string>

class obj_pak_exception_t {
	private:
		std::string classname;
		std::string problem;

	public:
		obj_pak_exception_t(const char* clsname, const char* prob)
		{
			classname = clsname;
			problem = prob;
		}

		const char* get_class() const { return classname.c_str(); }
		const char* get_info()  const { return problem.c_str(); }
};

#endif
