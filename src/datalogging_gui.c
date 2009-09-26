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

#include <args.h>
#include <config.h>
#include <datalogging_gui.h>
#include <defines.h>
#include <debugging.h>
#include <enums.h>
#include <errno.h>
#include <fileio.h>
#include <firmware.h>
#include <getfiles.h>
#include <glib.h>
#include <gui_handlers.h>
#include <math.h>
#include <notifications.h>
#include <rtv_map_loader.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <timeout_handlers.h>
#include <unistd.h>
#include <widgetmgmt.h>


/* global vars (owned here...) */
gchar *delimiter;
gboolean begin = TRUE;

/* External global vars */
extern gint ready;
extern Rtv_Map *rtv_map;
extern GObject *global_data;

/* Static vars to all functions in this file... */
static gboolean logging_active = FALSE;
static gboolean header_needed = FALSE;


/*!
 \brief populate_dlog_choices_pf() is called when the datalogging tab is loaded
 by glade AFTER the realtime variable definitions have been loaded and 
 processed.  All of the logable variables are then placed here to user 
 selecting during datalogging.
 */
EXPORT void populate_dlog_choices_pf()
{
	guint i,j,k;
	GtkWidget *vbox = NULL;
	GtkWidget *table = NULL;
	GtkWidget *button = NULL;
	GtkWidget *label = NULL;
	gint table_rows = 0;
	GObject * object = NULL;
	gchar * dlog_name = NULL;
	gchar * tooltip = NULL;
	extern GtkTooltips *tip;
	extern gint preferred_delimiter;
	extern gboolean tabs_loaded;
	extern gboolean rtvars_loaded;
	extern gboolean connected;
	extern gboolean interrogated;
	extern volatile gboolean leaving;

	if ((!tabs_loaded) || (leaving))
		return;
	if (!((connected) && (interrogated)))
		return;
	if (!rtvars_loaded)
	{
		dbg_func(CRITICAL,g_strdup(__FILE__": populate_dlog_choices_pf()\n\tCRITICAL ERROR, Realtime Variable definitions NOT LOADED!!!\n\n"));
		return;
	}
	set_title(g_strdup("Populating Datalogger..."));

	vbox = lookup_widget("dlog_logable_vars_vbox1");
	if (!GTK_IS_WIDGET(vbox))
	{
		printf("datalogger not present,  returning\n");
		return;
	}
	table_rows = ceil((float)rtv_map->derived_total/(float)TABLE_COLS);
	table = gtk_table_new(table_rows,TABLE_COLS,TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table),0);
	gtk_table_set_col_spacings(GTK_TABLE(table),0);
	gtk_container_set_border_width(GTK_CONTAINER(table),0);
	gtk_box_pack_start(GTK_BOX(vbox),table,TRUE,TRUE,0);

	/* Update status of the delimiter buttons... */

	switch (preferred_delimiter)
	{
		case COMMA:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget("dlog_comma_delimit_radio_button")),TRUE);
			gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(lookup_widget("dlog_comma_delimit_radio_button")));
			break;
		case TAB:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget("dlog_tab_delimit_radio_button")),TRUE);
			gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(lookup_widget("dlog_tab_delimit_radio_button")));
			break;
		default:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget("dlog_comma_delimit_radio_button")),TRUE);
			gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(lookup_widget("dlog_comma_delimit_radio_button")));
			break;

	}
	j = 0;	
	k = 0;
	for (i=0;i<rtv_map->derived_total;i++)
	{
		tooltip = NULL;
		dlog_name = NULL;
		object = g_array_index(rtv_map->rtv_list,GObject *,i);
		dlog_name = OBJ_GET(object,"dlog_gui_name");
		button = gtk_check_button_new();
		label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label),dlog_name);
		gtk_container_add(GTK_CONTAINER(button),label);
		tooltip = g_strdup(OBJ_GET(object,"tooltip"));
		if (tooltip)
			gtk_tooltips_set_tip(tip,button,tooltip,NULL);
		g_free(tooltip);

		/* Bind button to the object, Done so that we can set the state
		 * of the buttons from elsewhere... 
		 */
		OBJ_SET(object,"dlog_button",(gpointer)button);
				
		/* Bind object to the button */
		OBJ_SET(button,"object",(gpointer)object);
				
		g_signal_connect(G_OBJECT(button),"toggled",
				G_CALLBACK(log_value_set),
				NULL);
		if ((gboolean)OBJ_GET(object,"log_by_default")==TRUE)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),TRUE);
		gtk_table_attach (GTK_TABLE (table), button, j, j+1, k, k+1,
				(GtkAttachOptions) (GTK_EXPAND|GTK_FILL|GTK_SHRINK),
				(GtkAttachOptions) (GTK_FILL|GTK_SHRINK),
				0, 0);
		j++;

		if (j == TABLE_COLS)
		{
			k++;
			j = 0;
		} 
	}
	gtk_widget_show_all(vbox);
	return;
}


/*!
 \brief start_datalogging() enables logging and if RT vars aren't running it
 starts them.
 */
void start_datalogging(void)
{
	extern volatile gboolean offline;
	extern gboolean forced_update;

	if (logging_active)
		return;   /* Logging already running ... */
	if (lookup_widget("dlog_logable_vars_vbox1"))
		gtk_widget_set_sensitive(lookup_widget("dlog_logable_vars_vbox1"),FALSE);
	if (lookup_widget("dlog_format_delimit_hbox1"))
		gtk_widget_set_sensitive(lookup_widget("dlog_format_delimit_hbox1"),FALSE);
	if (lookup_widget("dlog_select_log_button"))
		gtk_widget_set_sensitive(lookup_widget("dlog_select_log_button"),FALSE);

	header_needed = TRUE;
	logging_active = TRUE;
	update_logbar("dlog_view",NULL,g_strdup("DataLogging Started...\n"),FALSE,FALSE);

	if (!offline)
		start_tickler(RTV_TICKLER);
	forced_update=TRUE;
	return;
}


/*!
 \brief stop_datalogging() stops the datalog process. It DOES not stop realtime
 variable readback though
 */
void stop_datalogging()
{
	GIOChannel *iochannel = NULL;
	if (!logging_active)
		return;

	logging_active = FALSE;

	if (lookup_widget("dlog_logable_vars_vbox1"))
		gtk_widget_set_sensitive(lookup_widget("dlog_logable_vars_vbox1"),TRUE);
	if (lookup_widget("dlog_format_delimit_hbox1"))
		gtk_widget_set_sensitive(lookup_widget("dlog_format_delimit_hbox1"),TRUE);
	if (lookup_widget("dlog_select_log_button"))
		gtk_widget_set_sensitive(lookup_widget("dlog_select_log_button"),TRUE);
	if (lookup_widget("dlog_stop_logging_button"))
		gtk_widget_set_sensitive(lookup_widget("dlog_stop_logging_button"),FALSE);
	if (lookup_widget("dlog_start_logging_button"))
		gtk_widget_set_sensitive(lookup_widget("dlog_start_logging_button"),FALSE);
	gtk_label_set_text(GTK_LABEL(lookup_widget("dlog_file_label")),"No Log Selected Yet");


	update_logbar("dlog_view",NULL,g_strdup("DataLogging Stopped...\n"),FALSE,FALSE);
	iochannel = (GIOChannel *) OBJ_GET(lookup_widget("dlog_select_log_button"),"data");
	if (iochannel)
	{
		g_io_channel_shutdown(iochannel,TRUE,NULL);
		g_io_channel_unref(iochannel);
	}

	OBJ_SET(lookup_widget("dlog_select_log_button"),"data",NULL);

	return;
}



/*! 
 \brief log_value_set() gets called when a variable is selected for 
 logging so that it can be marked as being logged
 */
gboolean log_value_set(GtkWidget * widget, gpointer data)
{
	GObject *object = NULL;
	gboolean state = FALSE;

	state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));
                
	/* get object from widget */
	object = (GObject *)OBJ_GET(widget,"object");
	OBJ_SET(object,"being_logged",GINT_TO_POINTER(state));

	return TRUE;
}


/*!
 \brief write_log_header() writes the top line of the datalog with field names
 \param iofile (Io_File *) pointer to the datalog output file 
 \param override (gboolean),  if true ALL variabels are logged, if FALSE
 only selected variabels are logged
 */
void write_log_header(GIOChannel *iochannel, gboolean override)
{
	guint i = 0;
	gint j = 0;
	gint total_logables = 0;
	gsize count = 0;
	GString *output;
	GObject * object = NULL;
	gchar * string = NULL;
	extern gint preferred_delimiter;
	extern Firmware_Details *firmware;
	if (!iochannel)
	{
		dbg_func(CRITICAL,g_strdup(__FILE__": write_log_header()\n\tIOChannel pointer was undefined, returning NOW...\n"));
		return;
	}
	/* Count total logable variables */
	for (i=0;i<rtv_map->derived_total;i++)
	{
		object = g_array_index(rtv_map->rtv_list,GObject *,i);
		if((override) || ((gboolean)OBJ_GET(object,"being_logged")))
			total_logables++;
	}
	output = g_string_sized_new(64); /* pre-allccate for 64 chars */

	string = g_strdup_printf("\"%s\"\r\n",firmware->actual_signature);
	output = g_string_append(output,string); 
	for (i=0;i<rtv_map->derived_total;i++)
	{
		object = g_array_index(rtv_map->rtv_list,GObject *,i);
		if((override) || ((gboolean)OBJ_GET(object,"being_logged")))
		{
			/* If space delimited, QUOTE the header names */
			string = (gchar *)OBJ_GET(object,"dlog_field_name");
			output = g_string_append(output,string); 

			j++;
			if (j < total_logables)
				output = g_string_append(output,delimiter);
		}
	}
	output = g_string_append(output,"\r\n");
	g_io_channel_write_chars(iochannel,output->str,output->len,&count,NULL);
	g_string_free(output,TRUE);

}


/*!
 \brief run_datalog_pf() gets called each time data arrives after rtvar 
 processing and logs the selected values to the file
 */
EXPORT void run_datalog_pf(void)
{
	guint i = 0;
	gint j = 0;
	gsize count = 0;
	gint total_logables = 0;
	GString *output;
	GIOChannel *iochannel = NULL;
	GObject *object = NULL;
	gfloat value = 0.0;
	GArray *history = NULL;
	gint current_index = 0;
	gint precision = 0;
	gchar *tmpbuf = NULL;
	extern gboolean interrogated;
	extern gboolean connected;

	if (!((connected) && (interrogated)))
		return;

	if (!logging_active) /* Logging isn't enabled.... */
		return;

	iochannel = (GIOChannel *) OBJ_GET(lookup_widget("dlog_select_log_button"),"data");
	if (!iochannel)
	{
		dbg_func(CRITICAL,g_strdup(__FILE__": run_datalog_pf()\n\tIo_File undefined, returning NOW!!!\n"));
		return;
	}

	for (i=0;i<rtv_map->derived_total;i++)
	{
		object = g_array_index(rtv_map->rtv_list,GObject *,i);
		if((gboolean)OBJ_GET(object,"being_logged"))
			total_logables++;
	}

	output = g_string_sized_new(64); /*64 char initial size */

	if (header_needed)
	{
		write_log_header(iochannel, FALSE);
		header_needed = FALSE;
		begin = TRUE;
	}
	j = 0;
	for(i=0;i<rtv_map->derived_total;i++)
	{
		object = g_array_index(rtv_map->rtv_list,GObject *,i);
		if (!((gboolean)OBJ_GET(object,"being_logged")))
			continue;

		history = (GArray *)OBJ_GET(object,"history");
		current_index = (gint)OBJ_GET(object,"current_index");
		precision = (gint)OBJ_GET(object,"precision");
		value = g_array_index(history, gfloat, current_index);

		tmpbuf = g_strdelimit(g_strdup_printf("%1$.*2$f",value,precision),",",'.');
		g_string_append(output,tmpbuf);
		j++;

		/* Print delimiter to log here so there isnt an extra
		 * char at the end fo the line 
		 */
		if (j < total_logables)
			output = g_string_append(output,delimiter);
	}
	output = g_string_append(output,"\r\n");
	g_io_channel_write_chars(iochannel,output->str,output->len,&count,NULL);
	g_string_free(output,TRUE);

}


/*!
 \brief dlog_select_all() selects all variables for logging
 */
void dlog_select_all()
{
	guint i = 0;
	GObject * object = NULL;
	GtkWidget *button = NULL;

	/* Check all logable choices */
	for (i=0;i<rtv_map->derived_total;i++)
	{
		object = NULL;
		button = NULL;
		object = g_array_index(rtv_map->rtv_list,GObject *, i);
		button = (GtkWidget *)OBJ_GET(object,"dlog_button");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),TRUE);
	}
}


/*!
 \brief dlog_deselect_all() resets the logged choices to having NONE selected
 */
void dlog_deselect_all(void)
{
	guint i = 0;
	GtkWidget * button = NULL;
	GObject * object = NULL;

	/* Uncheck all logable choices */
	for (i=0;i<rtv_map->derived_total;i++)
	{
		object = NULL;
		button = NULL;
		object = g_array_index(rtv_map->rtv_list,GObject *, i);
		button = (GtkWidget *)OBJ_GET(object,"dlog_button");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),FALSE);
	}
}

/*!
 \brief dlog_select_defaults() resets the logged choices to the ones defined 
 in the RealtimeMap file
 */
void dlog_select_defaults(void)
{
	guint i = 0;
	GtkWidget * button = NULL;
	GObject * object = NULL;
	gboolean state=FALSE;

	/* Uncheck all logable choices */
	for (i=0;i<rtv_map->derived_total;i++)
	{
		object = NULL;
		button = NULL;
		state = FALSE;
		object = g_array_index(rtv_map->rtv_list,GObject *, i);
		button = (GtkWidget *)OBJ_GET(object,"dlog_button");
		state = (gboolean)OBJ_GET(object,"log_by_default");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),state);
	}
}


EXPORT gboolean select_datalog_for_export(GtkWidget *widget, gpointer data)
{
	MtxFileIO *fileio = NULL;
	gchar *filename = NULL;
	GIOChannel *iochannel = NULL;
	extern Firmware_Details *firmware;
	struct tm *tm = NULL;
	time_t *t = NULL;

	t = g_malloc(sizeof(time_t));
	time(t);
	tm = localtime(t);
	g_free(t);

	fileio = g_new0(MtxFileIO ,1);
	fileio->external_path = g_strdup("MTX_Datalogs");
	fileio->title = g_strdup("Choose a filename for datalog export");
	fileio->parent = lookup_widget("main_window");
	fileio->on_top =TRUE;
	fileio->default_filename = g_strdup_printf("%s-%.4i_%.2i_%.2i-%.2i%.2i.log",g_strdelimit(firmware->name," ,",'_'),tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min);
	fileio->default_extension= g_strdup("log");
	fileio->action = GTK_FILE_CHOOSER_ACTION_SAVE;

	filename = choose_file(fileio);
	if (filename == NULL)
	{
		update_logbar("dlog_view",g_strdup("warning"),g_strdup("NO FILE opened for normal datalogging!\n"),FALSE,FALSE);
		return FALSE;
	}

	iochannel = g_io_channel_new_file(filename, "a+",NULL);
	if (!iochannel)
	{
		update_logbar("dlog_view",g_strdup("warning"),g_strdup("File open FAILURE! \n"),FALSE,FALSE);
		return FALSE;
	}

	gtk_widget_set_sensitive(lookup_widget("dlog_stop_logging_button"),TRUE);
	gtk_widget_set_sensitive(lookup_widget("dlog_start_logging_button"),TRUE);
	OBJ_SET(lookup_widget("dlog_select_log_button"),"data",(gpointer)iochannel);
	gtk_label_set_text(GTK_LABEL(lookup_widget("dlog_file_label")),g_filename_to_utf8(filename,-1,NULL,NULL,NULL));
	update_logbar("dlog_view",NULL,g_strdup("DataLog File Opened\n"),FALSE,FALSE);

	free_mtxfileio(fileio);
	return TRUE;
}


gboolean autolog_dump(gpointer data)
{
	CmdLineArgs *args = NULL;
	gchar *filename = NULL;
	GIOChannel *iochannel = NULL;
	static gint dlog_index = 0;

	args = (CmdLineArgs *)OBJ_GET(global_data,"args");

	filename = g_strdup_printf("%s%s%s_%.3i.log",args->autolog_dump_dir,PSEP,args->autolog_basename,dlog_index);
		
	iochannel = g_io_channel_new_file(filename, "a+",NULL);
	dump_log_to_disk(iochannel);
	g_io_channel_shutdown(iochannel,TRUE,NULL);
	g_io_channel_unref(iochannel);
	dlog_index++;
	update_logbar("dlog_view",NULL,g_strdup_printf("Autolog dump (log number %i) successfully completed.\n",dlog_index),FALSE,FALSE);
	g_free(filename);
	return TRUE;
}


EXPORT gboolean internal_datalog_dump(GtkWidget *widget, gpointer data)
{
	MtxFileIO *fileio = NULL;
	gchar *filename = NULL;
	GIOChannel *iochannel = NULL;
	extern Firmware_Details *firmware;
	struct tm *tm = NULL;
	time_t *t = NULL;

	t = g_malloc(sizeof(time_t));
	time(t);
	tm = localtime(t);
	g_free(t);



	fileio = g_new0(MtxFileIO ,1);
	fileio->external_path = g_strdup("MTX_Datalogs");
	fileio->title = g_strdup("Choose a filename for internal datalog export");
	fileio->parent = lookup_widget("main_window");
	fileio->on_top =TRUE;
	fileio->default_filename = g_strdup_printf("%s-%.4i_%.2i_%.2i-%.2i%.2i.log",g_strdelimit(firmware->name," ,",'_'),tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min);
	fileio->default_extension= g_strdup("log");
	fileio->action = GTK_FILE_CHOOSER_ACTION_SAVE;

	filename = choose_file(fileio);
	if (filename == NULL)
	{
		update_logbar("dlog_view",g_strdup("warning"),g_strdup("NO FILE opened for internal log export,  aborting dump!\n"),FALSE,FALSE);
		return FALSE;
	}

	iochannel = g_io_channel_new_file(filename, "a+",NULL);
	if (iochannel)
		update_logbar("dlog_view",NULL,g_strdup("File opened successfully for internal log dump\n"),FALSE,FALSE);
	dump_log_to_disk(iochannel);
	g_io_channel_shutdown(iochannel,TRUE,NULL);
	g_io_channel_unref(iochannel);
	update_logbar("dlog_view",NULL,g_strdup("Internal datalog successfully dumped to disk\n"),FALSE,FALSE);
	free_mtxfileio(fileio);
	g_free(filename);
	return TRUE;

}

/*!
 \brief dump_log_to_disk() dumps the contents of the RTV arrays to disk as a
 datalog file
 */
void dump_log_to_disk(GIOChannel *iochannel)
{
	guint i = 0;
	guint x = 0;
	guint j = 0;
	gsize count = 0;
	GString *output;
	GObject * object = NULL;
	GArray **histories = NULL;
	gint *precisions = NULL;
	gchar *tmpbuf = NULL;
	gfloat value = 0.0;
	extern gint realtime_id;
	gboolean restart_tickler = FALSE;

	if (realtime_id)
	{
		restart_tickler = TRUE;
		stop_tickler(RTV_TICKLER);
	}

	output = g_string_sized_new(1024); 

	write_log_header(iochannel, TRUE);

	histories = g_new0(GArray *,rtv_map->derived_total);
	precisions = g_new0(gint ,rtv_map->derived_total);

	for(i=0;i<rtv_map->derived_total;i++)
	{
		object = g_array_index(rtv_map->rtv_list,GObject *,i);
		histories[i] = (GArray *)OBJ_GET(object,"history");
		precisions[i] = (gint)OBJ_GET(object,"precision");
	}

	for (x=0;x<rtv_map->ts_array->len;x++)
	{
		j = 0;
		for(i=0;i<rtv_map->derived_total;i++)
		{
			value = g_array_index(histories[i], gfloat, x);
			/*tmpbuf = g_ascii_formatd(buf,G_ASCII_DTOSTR_BUF_SIZE,"%1$.*2$f",value,precisions[i]);*/
			tmpbuf = g_strdelimit(g_strdup_printf("%1$.*2$f",value,precisions[i]),",",'.');
			g_string_append(output,tmpbuf);
			j++;

			/* Print delimiter to log here so there isnt an extra
			 * char at the end fo the line 
			 */
			if (j < rtv_map->derived_total)
				output = g_string_append(output,delimiter);
		}
		output = g_string_append(output,"\r\n");
	}
	g_io_channel_write_chars(iochannel,output->str,output->len,&count,NULL);
	g_free(precisions);
	g_free(histories);
	g_string_free(output,TRUE);
	if (restart_tickler)
		start_tickler(RTV_TICKLER);

}
