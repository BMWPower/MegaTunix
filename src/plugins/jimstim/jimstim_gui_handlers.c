/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

/*!
  \file src/plugins/jimstim/jimstim_gui_handlers.c
  \ingroup JimStimPlugin,Plugins
  \brief JimStim plugin GUI handlers
  \author David Andruczyk
  */

#include <jimstim_gui_handlers.h>
#include <jimstim_sweeper.h>
#include <gtk/gtk.h>

extern gconstpointer *global_data;

/*!
  \brief This is used for this plugin to initialize stuff on the main Gui
  */
G_MODULE_EXPORT void ecu_gui_init(void)
{
	/* We don't need anything specific to this ecu initialized */
}


/*!
  \brief ECU specific plugin handler for toggle buttons
  \param widget is the pointer to the toggle button
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean ecu_toggle_button_handler(GtkWidget *widget, gpointer data)
{
        return TRUE;
}


/*!
  \brief ECU specific plugin handler for standard buttons
  \param widget is the pointer to the standard button
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean ecu_std_button_handler(GtkWidget *widget, gpointer data)
{
	JimStimStdHandler handler;
	handler = (JimStimStdHandler)OBJ_GET(widget,"handler");

	switch (handler)
	{
		case SWEEP_START:
			jimstim_sweep_start(widget,data);
			break;
		case SWEEP_STOP:
			jimstim_sweep_end(widget,data);
			break;
		default:
			printf("ERROR, case not handled, jimstim ecu std button handler!\n");
			break;

	}
	return TRUE;
}


/*!
  \brief ECU specific plugin handler for radio/check buttons
  \param widget is the pointer to the radio/check button
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean ecu_bitmask_button_handler(GtkWidget *widget, gpointer data)
{
	return TRUE;
}


/*!
  \brief ECU specific plugin handler for spin buttons
  \param widget is the pointer to the spin button
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean ecu_spin_button_handler(GtkWidget *widget, gpointer data)
{
	return TRUE;
}


/*!
  \brief ECU specific plugin handler for text entries
  \param widget is the pointer to the text entry
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean ecu_entry_handler(GtkWidget *widget, gpointer data)
{
	return TRUE;
}



/*!
  \brief ECU specific plugin handler for combo boxes
  \param widget is the pointer to the combo box 
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean ecu_combo_handler(GtkWidget *widget, gpointer data)
{
	return TRUE;
}


/*!
  \brief ECU specific plugin handler for combo boxes
  \param widget is the pointer to the combo box 
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean ecu_update_combo(GtkWidget *widget, gpointer data)
{
	return TRUE;
}


/*!
  \brief ECU specific plugin handler for combo boxes
  \param widget is the pointer to the combo box 
  \param data is unused
  \returns TRUE
  */
G_MODULE_EXPORT gboolean jimstim_rpm_value_changed(GtkWidget *widget, gpointer data)
{
	gchar *tmpbuf = NULL;
	gchar *widget_name = NULL;
	GtkWidget *entry = NULL;
	gint val = 0;
	widget_name = OBJ_GET(widget,"special");
	g_return_val_if_fail(widget_name,FALSE);
	entry = lookup_widget_f(widget_name);
	g_return_val_if_fail(entry,FALSE);
	val = (GINT)gtk_range_get_value(GTK_RANGE(widget));
	if (GTK_IS_ENTRY(entry))
	{
		tmpbuf = g_strdup_printf("%i",val);
		gtk_entry_set_text(GTK_ENTRY(entry),tmpbuf);
		g_signal_emit_by_name(entry,"activate");
		g_free(tmpbuf);
	}
	return TRUE;
}


