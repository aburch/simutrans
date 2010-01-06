/*
 * Copyright (c) 2010 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Support notifying objects when other objects are inserted, updated or deleted.
 * 
 * First used to notify players, that a fabrik is going to be deleted.
 */

#pragma once
#ifndef NOTIFICATION_H
#define NOTIFICATION_H

enum notification_t {
	notify_create,	// notified after object is created (and inserted into the slist_tpl<>)!
	notify_change,	// notified after object is changed. Sorted lists may have to be resorted.
	notify_delete	// notified immediately before object is deleted (and before nulled in the slist_tpl<>)!
};

#endif