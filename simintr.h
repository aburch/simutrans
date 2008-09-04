/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef simintr_h
#define simintr_h


class karte_t;
class karte_ansicht_t;


bool reduce_frame_time();
bool increase_frame_time();
unsigned long get_frame_time();
void set_frame_time(unsigned long time);


void intr_refresh_display(bool dirty);

void intr_set(karte_t *welt, karte_ansicht_t *view);


/**
 * currently only used by the pause tool. Use with care!
 * @author Hj. Malthaner
 */
void intr_set_last_time(long time);
long intr_get_last_time();


void intr_enable();
void intr_disable();


// force sync_step (done before sleeping)
void interrupt_force();

void interrupt_check();
void interrupt_check(const char* caller_info);

// standard version
#define INT_CHECK(info) interrupt_check();

// debug version
// #define INT_CHECK(info) interrupt_check(info);

#endif
