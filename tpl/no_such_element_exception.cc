#include <stdio.h>

#include "no_such_element_exception.h"

no_such_element_exception::no_such_element_exception(const char * container,
						     const char * type,
						     int element,
						     const char * reason)
{
  sprintf(message,
	  "No such element!\n"
	  "  Container: %s\n"
	  "  Type: %s\n"
	  "  Element %d\n"
	  "  Reason %s\n",
	  container,
	  type,
	  element,
	  reason);
}
