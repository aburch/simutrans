#ifndef OBJ_PAK_EXCEPTION_H
#define OBJ_PAK_EXCEPTION_H

class obj_pak_exception_t {
	private:
		cstring_t classname;
		cstring_t problem;

	public:
		obj_pak_exception_t(const char* clsname, const char* prob)
		{
			classname = clsname;
			problem = prob;
		}

		const char* get_class() const { return classname; }
		const char* get_info()  const { return problem; }
};

#endif
