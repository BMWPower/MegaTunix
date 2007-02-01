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

#include <config.h>
#include <defines.h>
#include <debugging.h>
#include <fileio.h>
#include <gtk/gtk.h>
#include <listmgmt.h>
#include <notifications.h>
#include <structures.h>
#include <tabloader.h>

extern GdkColor red;
extern GdkColor black;
static gboolean warning_present = FALSE;
static GtkWidget *warning_dialog;


/*!
 \brief set_group_color() sets all the widgets in the passed group to 
 the color passed.
 \param color (GuiColor enumeration) the color to set the widgets to
 \param group (gchar *) textual name of the group of controls to alter color
 \see set_widget_color
 */
void set_group_color(GuiColor color, gchar *group)
{
	g_list_foreach(get_list(group), set_widget_color,(gpointer)color);
}


/*!
 \brief set_reqfuel_color() sets all the widgets in the reqfuel group as 
 defined by the page number passed to the color passed.
 \param color (GuiColor enumeration) the color to set the widgets to
 \param table_num (gint) the table number to determien the right group of 
 controls to change the color on
 \see set_widget_color
 */
void set_reqfuel_color(GuiColor color, gint table_num)
{
	gchar *name = NULL;
	name = g_strdup_printf("reqfuel_%i_ctrl",table_num);
	g_list_foreach(get_list(name), set_widget_color,(gpointer)color);
	g_free(name);
}


/*!
 \brief set_widget_color() sets all the widgets in the passed group to 
 the color passed.
 \param widget (gpointer) the widget to  change color
 \param color (gpointer) enumeration of the color to switch to..
 */
void set_widget_color(gpointer widget, gpointer color)
{
	switch ((GuiColor)color)
	{
		case RED:
			if (GTK_IS_BUTTON(widget))
			{
				gtk_widget_modify_fg(GTK_BIN(widget)->child,
						GTK_STATE_NORMAL,&red);
				gtk_widget_modify_fg(GTK_BIN(widget)->child,
						GTK_STATE_PRELIGHT,&red);
			}
			else if (GTK_IS_LABEL(widget))
			{
				gtk_widget_modify_fg(widget,
						GTK_STATE_NORMAL,&red);
				gtk_widget_modify_fg(widget,
						GTK_STATE_PRELIGHT,&red);
			}
			else
			{
				gtk_widget_modify_text(GTK_WIDGET(widget),
						GTK_STATE_NORMAL,&red);
				gtk_widget_modify_text(GTK_WIDGET(widget),
						GTK_STATE_INSENSITIVE,&red);
			}
			break;
		case BLACK:
			if (GTK_IS_BUTTON(widget))
			{
				gtk_widget_modify_fg(GTK_BIN(widget)->child,
						GTK_STATE_NORMAL,&black);
				gtk_widget_modify_fg(GTK_BIN(widget)->child,
						GTK_STATE_PRELIGHT,&black);
			}
			else if (GTK_IS_LABEL(widget))
			{
				gtk_widget_modify_fg(widget,
						GTK_STATE_NORMAL,&black);
				gtk_widget_modify_fg(widget,
						GTK_STATE_PRELIGHT,&black);
			}
			else
			{	
				gtk_widget_modify_text(GTK_WIDGET(widget),
						GTK_STATE_NORMAL,&black);
				gtk_widget_modify_text(GTK_WIDGET(widget),
						GTK_STATE_INSENSITIVE,&black);
			}
			break;
	}
}


/*!
 \brief update_logbar() updates the logbar passed with the text passed to it
 \param view_name (gchar *) textual name of the textview the text is supposed 
 to go to. (required)
 \param tagname (gchar *) textual tagname to be used to set attributes on the
 text (optional, can be NULL)
 \param message (gchar *) message to display (required)
 \param count (gboolean) flag to show a running count or not
 \param clear (gboolean) if set, clear display before displaying text
 */
void  update_logbar(
		gchar * view_name, 
		gchar * tagname, 
		gchar * message,
		gboolean count,
		gboolean clear)
{
	GtkTextIter iter;
	GtkTextBuffer * textbuffer = NULL;
	GtkWidget *parent = NULL;
	GtkAdjustment * adj = NULL;
	gint counter = -1;
	gchar *tmpbuf = NULL;
	gpointer result = NULL;
	GtkWidget * widget = NULL;
	extern GHashTable *dynamic_widgets;

	widget = (GtkWidget *)g_hash_table_lookup(dynamic_widgets,view_name);
	if (!widget)
		return;
	if (!GTK_IS_OBJECT(widget))
	{
		dbg_func(g_strdup_printf(__FILE__": update_logbar()\n\t Textview name passed: \"%s\" wasn't registered, not updating\n",view_name),CRITICAL);
		return;
	}

	/* Add the message to the end of the textview */
	textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
	if (!textbuffer)
		return;
	gtk_text_buffer_get_end_iter (textbuffer, &iter);
	result = g_object_get_data(G_OBJECT(widget),"counter");	
	if (result == NULL)
		counter = 0;
	else
		counter = (gint)result;

	if (count)
	{
		counter++;
		tmpbuf = g_strdup_printf(" %i. ",counter);
		g_object_set_data(G_OBJECT(widget),"counter",GINT_TO_POINTER(counter));	
	}

	if (tagname == NULL)
	{
		if (count) /* if TRUE, display counter, else don't */
			gtk_text_buffer_insert(textbuffer,&iter,tmpbuf,-1);
		gtk_text_buffer_insert(textbuffer,&iter,message,-1);
	}
	else
	{
		if (count) /* if TRUE, display counter, else don't */
			gtk_text_buffer_insert_with_tags_by_name(textbuffer,
					&iter,tmpbuf,-1,tagname,NULL);

		gtk_text_buffer_insert_with_tags_by_name(textbuffer,&iter,
				message,-1,tagname,NULL);
	}

	g_free(message);
	/* Get it's parent (the scrolled window) and slide it to the
	 * bottom so the new message is visible... 
	 */
	parent = gtk_widget_get_parent(widget);
	if (parent != NULL)
	{
		adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(parent));
		adj->value = adj->upper;
	}

	if (tmpbuf)
		g_free(tmpbuf);
	return;	
}


/*!
 \brief conn_warning() displays a warning message when connection is 
 either lost or not detected with the ECU
 */
EXPORT void conn_warning(void)
{
	gchar *buff = NULL;
	buff = g_strdup("The MegaSquirt ECU appears to be currently disconnected.  This means that either one of the following occurred:\n   1. Wrong Comm port is selected on the Communications Tab\n   2. The MegaSquirt serial link is not plugged in\n   3. The MegaSquirt ECU does not have adequate power.\n   4. The MegaSquirt ECU is in bootloader mode.\n\nSuggest checking the serial settings on the Communications page first, and then check the Serial Status log at the bottom of that page. If the Serial Status log says \"I/O with MegaSquirt Timeout\", it means one of a few possible problems:\n   1. You selected the wrong COM port (older systems came with two, most newer ones only have one, try the other one...)\n   2. Faulty cable to the MegaSquirt unit. (Should be a straight thru DB9 Male/Female cable)\n   3. The cable is OK, but the MS doesn't have adequate power.\n   4. If you have the ECU in bootloader mode, none of the lights lite up, and a terminal program will show a \"Boot>\" prompt Disconnect the Boot jumper and power cycle the ECU.\n\nIf it says \"Serial Port NOT Opened, Can NOT Test ECU Communications\", this can mean one of two things:\n   1. The COM port doesn't exist on your computer,\n\t\t\t\t\tOR\n   2. You don't have permission to open the port. (/dev/ttySx).\nUNIX ONLY: Change the permissions on /dev/ttyS* to 666 (\"chmod 666 /dev/ttyS*\" as root), NOTE: This has potential security implications. Check the Unix/Linux system documentation regarding \"security\" for more information...\n");
	if (!warning_present)
		warn_user(buff);
	g_free(buff);
}


/*!
 \brief kill_conn_warning() removes the no connection warning message.
  Takes no parameters.
 */
EXPORT void kill_conn_warning()
{
	if (!warning_present)
		return;
	if (GTK_IS_WIDGET(warning_dialog))
		close_dialog(warning_dialog,warning_dialog);
}


/*!
 \brief warn_user() displays a warning message on the screen as a error dialog
 \param message (gchar *) the text to display
 */
void warn_user(gchar *message)
{
	warning_dialog = gtk_message_dialog_new(NULL,0,GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,message);
	g_signal_connect (G_OBJECT(warning_dialog),
			"delete_event",
			G_CALLBACK (close_dialog),
			warning_dialog);
	g_signal_connect (G_OBJECT(warning_dialog),
			"destroy_event",
			G_CALLBACK (close_dialog),
			warning_dialog);
	g_signal_connect (G_OBJECT(warning_dialog),
			"response",
			G_CALLBACK (close_dialog),
			warning_dialog);

	warning_present = TRUE;
	gtk_widget_show_all(warning_dialog);
}


/*!
 \brief close_dialog() is a handler to closeth edialog and reset the flag
 showing if it was up or not so it prevents displaying the error multiple
 times
 \param widget (GtkWidget *) widget to destroy
 \param data (gpointer) unused
 */
gint close_dialog(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(widget);
	warning_present = FALSE;
	warning_dialog = NULL;
	return TRUE;
}



/*!
 \brief set_title() appends text to the titlebar of the application to
 give user notifications...
 \param text (gchar *) text to append, (static strings only please)
 */
void set_title(gchar * text)
{
	extern GtkWidget *main_window;
	gchar * tmpbuf = NULL;
	extern gboolean leaving;

	if ((!main_window) || (leaving))
		return;
	tmpbuf = g_strconcat("MegaTunix ",VERSION,",   ",text,NULL);

	gtk_window_set_title(GTK_WINDOW(main_window),tmpbuf);
	g_free(tmpbuf);
	g_free(text);
}
