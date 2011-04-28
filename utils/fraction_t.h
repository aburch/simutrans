/*
 * fraction_t.h
 *
 *  Created on: 28.04.2011
 *      Author: Bernd Gabriel
 */

#ifndef FRACTION_T_H_
#define FRACTION_T_H_

#ifdef USE_FRACTION64_T
	#include "fraction64_t.h"
	typedef fraction64_t fraction_t;
#else
	#include "fraction32_t.h"
	typedef fraction32_t fraction_t;
#endif

#endif /* FRACTION_T_H_ */
