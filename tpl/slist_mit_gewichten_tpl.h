/*
 *
 *  slist_mit_gewichten_tpl.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#ifndef __SLIST_MIT_GEWICHTEN_TPL_H
#define __SLIST_MIT_GEWICHTEN_TPL_H

#include "slist_tpl.h"


/*
 *  Autor:
 *      Volker Meyer
 */
template<class T> class slist_mit_gewichten_tpl : public slist_tpl<T> {
    int	gesamtgewicht;
public:
    slist_mit_gewichten_tpl()
    {
	gesamtgewicht = 0;
    }

    void insert(T data)
    {
	slist_tpl<T>::insert(data);
	gesamtgewicht += data->gib_gewichtung();
    }
    void insert(T data, int pos)
    {
	slist_tpl<T>::insert(data, pos);
	gesamtgewicht += data->gib_gewichtung();
    }
    void append(T data)
    {
	slist_tpl<T>::append(data);
	gesamtgewicht += data->gib_gewichtung();
    }
    bool remove(const T data)
    {
	if(slist_tpl<T>::remove(data)) {
	    gesamtgewicht -= data->gib_gewichtung();
	    return true;
	}
	return false;
    }
    bool remove_at(int pos)
    {
	T data = this->at(pos);
	if(slist_tpl<T>::remove(data)) {
	    gesamtgewicht -= data->gib_gewichtung();
	    return true;
	}
	return false;
    }
    T remove_first()
    {
	T data = slist_tpl<T>::remove_first();
	if(data) {
	    gesamtgewicht -= data->gib_gewichtung();
	}
	return data;
    }
    void clear()
    {
	slist_tpl<T>::clear();
	gesamtgewicht = 0;
    }
    void destroy()
    {
	slist_tpl<T>::destroy();
	gesamtgewicht = 0;
    }


    int gib_gesamtgewicht() const
    {
	return gesamtgewicht;
    }

    T gib_gewichted(int i) const
    {
	slist_iterator_tpl<T> iter(this);

	while(iter.next()) {
	    if(i < iter.get_current()->gib_gewichtung()) {
		return iter.get_current();
	    }
	    i -= iter.get_current()->gib_gewichtung();
	}
	return NULL;
    }
};

#endif
