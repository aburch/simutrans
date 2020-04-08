/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_NOTIFICATION_H
#define UTILS_NOTIFICATION_H


/**
 * Support notifying objects when other objects are inserted, updated or deleted.
 *
 * First used to notify players, that a factory is going to be deleted.
 */
enum notification_t {
	notify_create,	// notified after object is created (and inserted into the slist_tpl<>)!
	notify_change,	// notified after object is changed. Sorted lists may have to be resorted.
	notify_delete	// notified immediately before object is deleted (and before nulled in the slist_tpl<>)!
};

#endif
