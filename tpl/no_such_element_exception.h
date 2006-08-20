#ifndef tpl_no_such_element_exception_h
#define tpl_no_such_element_exception_h


/**
 * Container throw this if the requested element doesn't exist
 * and there is no way to return a value like NULL to tell the
 * caller about the problem.
 *
 * @author Hj. Malthaner
 */
class no_such_element_exception
{
 public:

  /**
   * Error message to be shown to the user
   *
   * @author Hj. Malthaner
   */
  char message [4096];

  no_such_element_exception(const char * container,
			    const char * type,
			    int element,
			    const char * reason);

};

#endif
