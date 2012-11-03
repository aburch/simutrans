/*
 * Copyright (c) 2006 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/* This file defines several UI coordinates */

#ifndef list_button_h
#define list_button_h

#define BUTTON_WIDTH (92)
#define BUTTON_HEIGHT (14)

#define TITLEBAR_HEIGHT (16)

#define DIALOG_LEFT (4)
#define DIALOG_TOP (4)
#define DIALOG_RIGHT (4)
#define DIALOG_BOTTOM (4)

// space between two elements
#define DIALOG_SPACER (3)

#define BUTTON_SPACER (2)

#define BUTTON1_X (DIALOG_LEFT)
#define BUTTON2_X (DIALOG_LEFT+1*(BUTTON_WIDTH+BUTTON_SPACER))
#define BUTTON3_X (DIALOG_LEFT+2*(BUTTON_WIDTH+BUTTON_SPACER))
#define BUTTON4_X (DIALOG_LEFT+3*(BUTTON_WIDTH+BUTTON_SPACER))

#define TOTAL_WIDTH (DIALOG_LEFT+4*BUTTON_WIDTH+3*BUTTON_SPACER+DIALOG_RIGHT)

#define INDICATOR_WIDTH (20)
#define INDICATOR_HEIGHT (4)

#endif
