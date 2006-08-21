/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute, etc. this as long as all the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

#ifndef __SERIALIO_H__
#define __SERIALIO_H__

#include <gtk/gtk.h>
#include <enums.h>

/* Prototypes */
void open_serial(gchar *);
void close_serial(void);
void setup_serial_params(void);
void toggle_serial_control_lines(void );
void set_ms_page(gint);
void flush_serial(gint,gint);
/* Prototypes */

#endif
