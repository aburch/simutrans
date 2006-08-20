/*
 * simintr.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simintr_h
#define simintr_h


class karte_t;
class karte_vollansicht_t;


void set_frame_time(long t);

bool reduce_frame_time();
bool increase_frame_time();

long get_frame_time();
long get_actual_frame_time();
long get_average_frame_time();

void intr_refresh_display(bool dirty);

void intr_set(karte_t *welt, karte_vollansicht_t *view, int refresh);
void intr_set(karte_t *welt, karte_vollansicht_t *view);


/**
 * currently only used by the pause tool. Use with care!
 * @author Hj. Malthaner
 */
void intr_set_last_time(long time);


extern "C" void intr_enable();
extern "C" void intr_disable();



void interrupt_check();
void interrupt_check(char *caller_info);

// standard version
#define INT_CHECK(info) interrupt_check();

// debug version
// #define INT_CHECK(info) interrupt_check(info);

#endif
