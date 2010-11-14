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
#include <enums.h>
#include <getfiles.h>
#include <glade/glade.h>
#include <gui_handlers.h>
#include <listmgmt.h>
#include <logviewer_core.h>
#include <logviewer_events.h>
#include <logviewer_gui.h>
#include <math.h>
#include <mode_select.h>
#include <rtv_map_loader.h>
#include <stdlib.h>
#include <tabloader.h>
#include <timeout_handlers.h>
#include <widgetmgmt.h>

static gint max_viewables = 0;
static gboolean adj_scale = TRUE;
static gboolean blocked = FALSE;
static gfloat hue = -60.0;
static gfloat col_sat = 1.0;
static gfloat col_val = 1.0;

Logview_Data *lv_data = NULL;
gboolean playback_mode = FALSE;
static GStaticMutex update_mutex = G_STATIC_MUTEX_INIT;
extern Log_Info *log_info;
extern gconstpointer *global_data;


/*!
 \brief present_viewer_choices() presents the user with the a list of 
 variables from EITHER the realtime vars (if in realtime mode) or from a 
 datalog (playback mode)
 */
void present_viewer_choices(void)
{
	GtkWidget *window = NULL;
	GtkWidget *table = NULL;
	GtkWidget *frame = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *hbox = NULL;
	GtkWidget *button = NULL;
	GtkWidget *label = NULL;
	GtkWidget *sep = NULL;
	GtkWidget *darea = NULL;
	GList *list = NULL;
	gconstpointer * object = NULL;
	gint i = 0;
	gint j = 0;
	gint k = 0;
	gint table_rows = 0;
	gint table_cols = 5;
	gchar * name = NULL;
	gchar * tooltip = NULL;
	extern Rtv_Map *rtv_map;

	darea = lookup_widget("logviewer_trace_darea");
	lv_data->darea = darea;

	if (!darea)
	{
		dbg_func(CRITICAL,g_strdup(__FILE__": present_viewer_choices()\n\tpointer to drawing area was NULL, returning!!!\n"));
		return;
	}

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable(GTK_WINDOW(window),FALSE);
	/* Playback mode..... */
	if (playback_mode)
	{
		gtk_window_set_title(GTK_WINDOW(window),
				_("Playback Mode: Logviewer Choices"));
		frame = gtk_frame_new(_("Select Variables to playback from the list below..."));
		max_viewables = log_info->field_count;
	}
	else
	{
		/* Realtime Viewing mode... */
		gtk_window_set_title(GTK_WINDOW(window),
				_("Realtime Mode: Logviewer Choices"));
		frame = gtk_frame_new(_("Select Realtime Variables to view from the list below..."));
		max_viewables = rtv_map->derived_total;

	}
	g_signal_connect_swapped(G_OBJECT(window),"destroy_event",
			G_CALLBACK(reenable_select_params_button),
			NULL);
	g_signal_connect_swapped(G_OBJECT(window),"destroy_event",
			G_CALLBACK(save_default_choices),
			NULL);
	g_signal_connect_swapped(G_OBJECT(window),"destroy_event",
			G_CALLBACK(gtk_widget_destroy),
			(gpointer)window);
	g_signal_connect_swapped(G_OBJECT(window),"delete_event",
			G_CALLBACK(reenable_select_params_button),
			NULL);
	g_signal_connect_swapped(G_OBJECT(window),"delete_event",
			G_CALLBACK(save_default_choices),
			NULL);
	g_signal_connect_swapped(G_OBJECT(window),"delete_event",
			G_CALLBACK(gtk_widget_destroy),
			(gpointer)window);

	gtk_container_set_border_width(GTK_CONTAINER(window),5);
	gtk_container_add(GTK_CONTAINER(window),frame);

	vbox = gtk_vbox_new(FALSE,0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_container_add(GTK_CONTAINER(frame),vbox);

	table_rows = ceil((float)max_viewables/(float)table_cols);
	table = gtk_table_new(table_rows,table_cols,TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table),2);
	gtk_table_set_col_spacings(GTK_TABLE(table),5);
	gtk_container_set_border_width(GTK_CONTAINER(table),0);
	gtk_box_pack_start(GTK_BOX(vbox),table,FALSE,FALSE,0);

	j = 0;
	k = 0;
	if(get_list("viewables"))
	{
		g_list_free(get_list("viewables"));
		remove_list("viewables");
	}

	for (i=0;i<max_viewables;i++)
	{
		if (playback_mode)
			list = g_list_prepend(list,(gpointer)g_ptr_array_index(log_info->log_list,i));
		else
			list = g_list_prepend(list,(gpointer)g_ptr_array_index(rtv_map->rtv_list,i));
	}
	if (playback_mode)
		list=g_list_sort_with_data(list,list_object_sort,(gpointer)"lview_name");
	else
		list=g_list_sort_with_data(list,list_object_sort,(gpointer)"dlog_gui_name");

	for (i=0;i<max_viewables;i++)
	{
		object = NULL;
		name = NULL;
		tooltip = NULL;

		object = g_list_nth_data(list,i);
		printf("obj ptr %p\n",object);

		if (playback_mode)
			name = g_strdup(DATA_GET(object,"lview_name"));
		else
		{
			name = g_strdup(DATA_GET(object,"dlog_gui_name"));
			tooltip = g_strdup(DATA_GET(object,"tooltip"));
		}

		printf("name %s\n",name);

		button = gtk_check_button_new();
		label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label),name);
		gtk_container_add(GTK_CONTAINER(button),label);
		store_list("viewables",g_list_prepend(
					get_list("viewables"),(gpointer)button));
		if (tooltip)
			gtk_widget_set_tooltip_text(button,tooltip);

		if (object)
		{
			OBJ_SET(button,"object",(gpointer)object);
			/* so we can set the state from elsewhere...*/
			DATA_SET(object,"lview_button",(gpointer)button);
			if ((GBOOLEAN)DATA_GET(object,"being_viewed"))
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),TRUE);
		}
		g_signal_connect(G_OBJECT(button),"toggled",
				G_CALLBACK(view_value_set),
				NULL);
		gtk_table_attach (GTK_TABLE (table), button, j, j+1, k, k+1,
				(GtkAttachOptions) (GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);
		j++;

		if (j == table_cols)
		{
			k++;
			j = 0;
		}
		g_free(name);
	}
	g_list_free(list);

	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox),sep,FALSE,TRUE,20);

	hbox = gtk_hbox_new(FALSE,20);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,TRUE,0);
	button = gtk_button_new_with_label("Select All");
	gtk_box_pack_start(GTK_BOX(hbox),button,TRUE,TRUE,15);
	OBJ_SET(button,"state",GINT_TO_POINTER(TRUE));
	g_signal_connect(G_OBJECT(button),"clicked",
			G_CALLBACK(set_all_lview_choices_state),
			GINT_TO_POINTER(TRUE));
	button = gtk_button_new_with_label("De-select All");
	gtk_box_pack_start(GTK_BOX(hbox),button,TRUE,TRUE,15);
	OBJ_SET(button,"state",GINT_TO_POINTER(FALSE));
	g_signal_connect(G_OBJECT(button),"clicked",
			G_CALLBACK(set_all_lview_choices_state),
			GINT_TO_POINTER(FALSE));

	button = gtk_button_new_with_label("Close");
	gtk_box_pack_start(GTK_BOX(vbox),button,FALSE,TRUE,0);
	g_signal_connect_swapped(G_OBJECT(button),"clicked",
			G_CALLBACK(reenable_select_params_button),
			NULL);
	g_signal_connect_swapped(G_OBJECT(button),"clicked",
			G_CALLBACK(save_default_choices),
			NULL);
	g_signal_connect_swapped(G_OBJECT(button),"clicked",
			G_CALLBACK(gtk_widget_destroy),
			(gpointer)window);

	set_default_lview_choices_state();
	gtk_widget_show_all(window);
	return;
}


gboolean reenable_select_params_button(GtkWidget *widget)
{
	gtk_widget_set_sensitive(GTK_WIDGET(lookup_widget("logviewer_select_params_button")),TRUE);
	return FALSE;

}

gboolean save_default_choices(GtkWidget *widget)
{
	GtkWidget *tmpwidget = NULL;
	GList * list = NULL;
	GList * defaults = NULL;
	gconstpointer *object = NULL;
	gchar *name = NULL;
	gint i = 0;

	defaults = get_list("logviewer_defaults");
	if (defaults)
	{
		g_list_foreach(defaults,(GFunc)g_free,NULL);
		g_list_free(defaults);
		defaults = NULL;
		remove_list("logviewer_defaults");
	}
	list = get_list("viewables");
	for (i=0;i<g_list_length(list);i++)
	{
		tmpwidget = g_list_nth_data(list,i);
		object = OBJ_GET(tmpwidget,"object");
		if ((gboolean)DATA_GET(object,"being_viewed"))
		{
			if (playback_mode)
				name = DATA_GET(object,"lview_name");
			else
				name = DATA_GET(object,"dlog_gui_name");

			defaults = g_list_append(defaults,g_strdup(name));
		}
	}
	store_list("logviewer_defaults",defaults);
	return FALSE;
}

/*!
 \brief view_value_set() is called when a value to be viewed is selected
 or not. We tag the widget with a marker if it is to be displayed
 \param widget (GtkWidget *) button clicked, we extract the object this
 represents and mark it
 \param data (gpointer) unused
 */
gboolean view_value_set(GtkWidget *widget, gpointer data)
{
	gconstpointer *object = NULL;
	gboolean state = FALSE;


	state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget));

	/* get object from widget */
	object = (gconstpointer *)OBJ_GET(widget,"object");
	if (!object)
	{
		dbg_func(CRITICAL,g_strdup(__FILE__": view_value_set()\n\t NO object was bound to the button\n"));
	}
	DATA_SET(object,"being_viewed",GINT_TO_POINTER(state));
	populate_viewer();
	return FALSE;
}


/*!
 \brief populate_viewer() creates/removes the list of viewable values from
 the objects in use (playback list or realtime vars list)
 */
void populate_viewer(void)
{
	gint i = 0;
	gint total = 0;
	Viewable_Value *v_value = NULL;
	gchar * name = NULL;
	gboolean being_viewed = FALSE;
	extern Rtv_Map *rtv_map;
	gconstpointer *object = NULL;

	g_static_mutex_lock(&update_mutex);

	/* Checks if hash is created, if not, makes one, allocates data
	 * for strcutres defining each viewable element., sets those attribute
	 * and adds them to the list, also checks if entires are removed and
	 * pulls them from the hashtable and de-allocates them...
	 */
	if (lv_data == NULL)
	{
		lv_data = g_new0(Logview_Data,1);
		lv_data->traces = g_hash_table_new(g_str_hash,g_str_equal);
		lv_data->info_width = 120;
	}

	/* check to see if it's already in the table, if so ignore, if not
	 * malloc datastructure, populate it's values and insert a pointer
	 * into the table for it..
	 */
	if (playback_mode)
		total = log_info->field_count;
	else
		total = rtv_map->derived_total;

	for (i=0;i<total;i++)
	{
		object = NULL;
		name = NULL;
		if (playback_mode)
		{
			object = g_ptr_array_index(log_info->log_list,i);        
			name = DATA_GET(object,"lview_name");
		}
		else
		{
			object = g_ptr_array_index(rtv_map->rtv_list,i); 
			name = g_strdup(DATA_GET(object,"dlog_gui_name"));
		}
		if (!name)
			dbg_func(CRITICAL,g_strdup("ERROR, name is NULL\n"));
		if (!object)
			dbg_func(CRITICAL,g_strdup("ERROR, object is NULL\n"));

		being_viewed = (GBOOLEAN)DATA_GET(object,"being_viewed");
		/* if not found in table check to see if we need to insert*/
		if (!(g_hash_table_lookup(lv_data->traces,name)))
		{
			if (being_viewed)	/* Marked viewable widget */
			{
				/* Call the build routine, feed it the drawing_area*/
				v_value = build_v_value(object);
				/* store location of master*/
				g_hash_table_insert(lv_data->traces,
						g_strdup(name),
						(gpointer)v_value);
				lv_data->tlist = g_list_prepend(lv_data->tlist,(gpointer)v_value);
			}
		}
		else
		{	/* If in table but now de-selected, remove it */
			if (!being_viewed)
			{
				v_value = (Viewable_Value *)g_hash_table_lookup(lv_data->traces,name);
				lv_data->tlist = g_list_remove(lv_data->tlist,(gpointer)v_value);
				if ((hue > 0) && ((gint)hue%1110 == 0))
				{
					hue-= 30;
					col_sat = 1.0;
					col_val = 0.75;
					/*printf("hue at 1110 deg, reducing to 1080, sat at 1.0, val at 0.75\n");*/
				}
				if ((hue > 0) && ((gint)hue%780 == 0))
				{
					hue-= 30;
					col_sat = 0.5;
					col_val = 1.0;
					/*printf("hue at 780 deg, reducing to 750, sat at 0.5, val at 1.0\n");*/
				}
				if ((hue > 0) && ((gint)hue%390 == 0)) /* phase shift */
				{
					hue-=30.0;
					col_sat=1.0;
					col_val = 1.0;
					/*printf("hue at 390 deg, reducing to 360, sat at 0.5, val at 1.0\n");*/
				}
				hue -=60;
				/*printf("angle at %f, sat %f, val %f\n",hue,col_sat,col_val);*/

				/* Remove entry in from hash table */
				g_hash_table_remove(lv_data->traces,name);

				/* Free all resources of the datastructure 
				 * before de-allocating it... 
				 */

				g_object_unref(v_value->trace_gc);
				g_object_unref(v_value->font_gc);
				g_free(v_value->vname);
				g_free(v_value);
				g_free(v_value->ink_rect);
				g_free(v_value->log_rect);
				v_value = NULL;
			}
		}
	}
	lv_data->active_traces = g_hash_table_size(lv_data->traces);
	/* If traces selected, emit a configure_Event to clear the window
	 * and draw the traces (IF ONLY reading a log for playback)
	 */
	g_static_mutex_unlock(&update_mutex);
	if ((lv_data->traces) && (g_list_length(lv_data->tlist) > 0))
		lv_configure_event(lookup_widget("logviewer_trace_darea"),NULL,NULL);

	return; 
}


/*!
 \brief reset_logviewer_state() deselects any traces, resets the position 
 slider.  This function is called when switching from playback to rt mode
 and back
 */
void reset_logviewer_state(void)
{
	extern Rtv_Map *rtv_map;
	extern Log_Info *log_info;
	guint i = 0 ;
	gconstpointer * object = NULL;

	if (playback_mode)
	{
		if (!log_info)
			return;
		for (i=0;i<log_info->field_count;i++)
		{
			object = NULL;
			object = g_ptr_array_index(log_info->log_list,i);
			if (object)
				DATA_SET(object,"being_viewed",GINT_TO_POINTER(FALSE));
		}
	}
	else
	{
		if (!rtv_map)
			return;
		for (i=0;i<rtv_map->derived_total;i++)
		{
			object = NULL;
			object = g_ptr_array_index(rtv_map->rtv_list,i);
			if (object)
				DATA_SET(object,"being_viewed",GINT_TO_POINTER(FALSE));
		}
	}
	populate_viewer();

}


/*!
 \brief build_v_value() allocates a viewable_value structure and populates
 it with sane defaults and returns it to the caller
 \param object (gconstpointer *) objet to get soem of the data from
 \returns a newly allocated and populated Viewable_Value structure
 */
Viewable_Value * build_v_value(gconstpointer *object)
{
	Viewable_Value *v_value = NULL;
	GdkPixmap *pixmap =  NULL;

	pixmap = lv_data->pixmap;

	v_value = g_malloc(sizeof(Viewable_Value));		

	/* Set limits of this variable. (it's ranges, used for scaling */

	if (playback_mode)
	{
		/* textual name of the variable we're viewing.. */
		v_value->vname = g_strdup(DATA_GET(object,"lview_name"));
		/* data was already read from file and stored, copy pointer
		 * over to v_value so it can be drawn...
		 */
		v_value->data_source = g_strdup("data_array");
	}
	else
	{
		/* textual name of the variable we're viewing.. */
		v_value->vname = g_strdup(DATA_GET(object,"dlog_gui_name"));
		/* Array to keep history for resize/redraw and export 
		 * to datalog we use the _sized_ version to give a big 
		 * enough size to prevent reallocating memory too often. 
		 * (more initial mem usage,  but less calls to malloc...
		 */
		v_value->data_source = g_strdup("history");
	}
	/* Store pointer to object, but DO NOT FREE THIS on v_value destruction
	 * as its the SAME one used for all Viewable_Values */
	v_value->object = object;
	/* IS it a floating point value? */
	v_value->precision = (GINT)DATA_GET(object,"precision");
	v_value->lower = (gint)strtol(DATA_GET(object,"real_lower"),NULL,10);
	v_value->upper = (gint)strtol(DATA_GET(object,"real_upper"),NULL,10);
	/* Sets last "y" value to -1, needed for initial draw to be correct */
	v_value->last_y = -1;

	/* User adjustable scales... */
	v_value->cur_low = v_value->lower;
	v_value->cur_high = v_value->upper;
	v_value->min = 0;
	v_value->max = 0;

	/* Allocate the colors (GC's) for the font and trace */
	v_value->font_gc = initialize_gc(pixmap, FONT);
	v_value->trace_gc = initialize_gc(pixmap, TRACE);

	/* Allocate the structs to hold the text screen dimensions */
	v_value->ink_rect = g_new0(PangoRectangle, 1);
	v_value->log_rect = g_new0(PangoRectangle, 1);

	v_value->force_update = TRUE;
	v_value->highlight = FALSE;

	return v_value;
}


/*!
 \brief initialize_gc() allocates and initializes the graphics contexts for
 the logviewer trace window.
 \param drawable (GdkDrawable *) pointer to the drawable surface
 \param type (GcType) Graphics Context type? (I donno for sure)
 \returns Pointer to a GdkGC *
 */
GdkGC * initialize_gc(GdkDrawable *drawable, GcType type)
{
	GdkColor color;
	GdkGC * gc = NULL;
	GdkGCValues values;
	GdkColormap *cmap = NULL;

	cmap = gdk_colormap_get_system();

	switch((GcType)type)
	{
		case HIGHLIGHT:
			color.red = 60000;
			color.green = 0;
			color.blue = 0;
			gdk_colormap_alloc_color(cmap,&color,TRUE,TRUE);
			values.foreground = color;
			gc = gdk_gc_new_with_values(GDK_DRAWABLE(drawable),
					&values,
					GDK_GC_FOREGROUND);
			break;
		case FONT:
			color.red = 65535;
			color.green = 65535;
			color.blue = 65535;
			gdk_colormap_alloc_color(cmap,&color,TRUE,TRUE);
			values.foreground = color;
			gc = gdk_gc_new_with_values(GDK_DRAWABLE(drawable),
					&values,
					GDK_GC_FOREGROUND);
			break;

		case TRACE:
			hue += 60;
			/*printf("angle at %f, sat %f, val %f\n",hue,col_sat,col_val);*/

			if ((hue > 0) && ((gint)hue%360 == 0))
			{
				hue+=30.0;
				col_sat=0.5;
				col_val=1.0;
			}
			if ((hue > 0) && ((gint)hue%750 == 0))
			{
				hue+=30;
				col_sat=1.0;
				col_val = 0.75;
			}
			/*printf("JBA angle at %f, sat %f, val %f\n",hue,col_sat,col_val);*/
			color = get_colors_from_hue(hue,col_sat,col_val);
			gdk_colormap_alloc_color(cmap,&color,TRUE,TRUE);
			values.foreground = color;
			gc = gdk_gc_new_with_values(GDK_DRAWABLE(drawable),
					&values,
					GDK_GC_FOREGROUND);


			break;
		case GRATICULE:
			color.red = 36288;
			color.green = 2048;
			color.blue = 2048;
			gdk_colormap_alloc_color(cmap,&color,TRUE,TRUE);
			values.foreground = color;
			gc = gdk_gc_new_with_values(GDK_DRAWABLE(drawable),
					&values,
					GDK_GC_FOREGROUND);
			break;
		case TTM_AXIS:
			color.red = 32768;
			color.green = 32768;
			color.blue = 32768;
			gdk_colormap_alloc_color(cmap,&color,TRUE,TRUE);
			values.foreground = color;
			gc = gdk_gc_new_with_values(GDK_DRAWABLE(drawable),
					&values,
					GDK_GC_FOREGROUND);
			break;
		case TTM_TRACE:
			color.red = 0;
			color.green = 0;
			color.blue = 0;
			gdk_colormap_alloc_color(cmap,&color,TRUE,TRUE);
			values.foreground = color;
			gc = gdk_gc_new_with_values(GDK_DRAWABLE(drawable),
					&values,
					GDK_GC_FOREGROUND);
			break;
	}	
	return gc;	
}


/*!
 \brief get_colors_from_hue(gets a color back from an angle passed in degrees.
 The degrees represent the arc aroudn a color circle.
 \param hue (gfloat) degrees around the color circle
 \param sat (gfloat) col_sat from 0-1.0
 \param val (gfloat) col_val from 0-1.0
 \returns a GdkColor at the hue angle requested
 */
GdkColor get_colors_from_hue(gfloat hue, gfloat sat, gfloat val)
{
	static gint count = 0;
	GdkColor color;
	gint i = 0;
	gfloat tmp = 0.0;	
	gfloat fract = 0.0;
	gfloat S = sat;	/* using col_sat of 1.0*/
	gfloat V = val;	/* using Value of 1.0*/
	gfloat p = 0.0;
	gfloat q = 0.0;
	gfloat t = 0.0;
	gfloat r = 0.0;
	gfloat g = 0.0;
	gfloat b = 0.0;
	static GdkColormap *colormap = NULL;

	count++;
	if (!colormap)
		colormap = gdk_colormap_get_system();

	/*printf("get_color_from_hue count %i\n",count); */

	while (hue >= 360.0)
		hue -= 360.0;
	while (hue < 0.0)
		hue += 360.0;
	tmp = hue/60.0;
	i = floor(tmp);
	fract = tmp-i;

	p = V*(1.0-S);	
	q = V*(1.0-(S*fract));	
	t = V*(1.0-(S*(1.0-fract)));

	switch (i)
	{
		case 0:
			r = V;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = V;
			b = p;
			break;
		case 2:
			r = p;
			g = V;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = V;
			break;
		case 4:
			r = t;
			g = p;
			b = V;
			break;
		case 5:
			r = V;
			g = p;
			b = q;
			break;
	}
	color.red = r * 65535;
	color.green = g * 65535;
	color.blue = b * 65535;
	gdk_colormap_alloc_color(colormap,&color,FALSE,TRUE);

	return (color);	
}


/*!
 \brief draw_infotext() draws the static textual data for the trace on 
 the left hand side of the logviewer
 */
void draw_infotext(void)
{
	/* Draws the textual (static) info on the left side of the window..*/

	gint name_x = 0;
	gint name_y = 0;
	gint text_border = 10;
	gint info_ctr = 0;
	gint h = 0;
	gint i = 0;
	gint width = 0;
	gint height = 0;
	gint max = 0;
	Viewable_Value *v_value = NULL;
	PangoLayout *layout;
	GdkPixmap *pixmap = lv_data->pixmap;

	h = lv_data->darea->allocation.height;

	gdk_draw_rectangle(pixmap,
			lv_data->darea->style->black_gc,
			TRUE, 0,0,
			lv_data->info_width,h);


	if (!lv_data->font_desc)
	{
		lv_data->font_desc = pango_font_description_from_string("courier");
		pango_font_description_set_size(lv_data->font_desc,(10)*PANGO_SCALE);
	}
	if (!lv_data->highlight_gc)
		lv_data->highlight_gc = initialize_gc(lv_data->pixmap,HIGHLIGHT);
	
	lv_data->spread = (gint)((float)h/(float)lv_data->active_traces);
	name_x = text_border;
	layout = gtk_widget_create_pango_layout(lv_data->darea,NULL);
	for (i=0;i<lv_data->active_traces;i++)
	{
		v_value = (Viewable_Value *)g_list_nth_data(lv_data->tlist,i);
		info_ctr = (lv_data->spread * (i+1))- (lv_data->spread/2);

		pango_layout_set_markup(layout,v_value->vname,-1);
		pango_layout_set_font_description(layout,lv_data->font_desc);
		pango_layout_get_pixel_size(layout,&width,&height);
		name_y = info_ctr - height - 2;

		if (width > max)
			max = width;
		
		gdk_draw_layout(pixmap,v_value->trace_gc,name_x,name_y,layout);
	}
	lv_data->info_width = max + (text_border * 2.5);

	for (i=0;i<lv_data->active_traces;i++)
	{
		gdk_draw_rectangle(pixmap,
				lv_data->darea->style->white_gc,
				FALSE, 0,i*lv_data->spread,
				lv_data->info_width-1,lv_data->spread);
	}

}


/*!
 \brief draw_valtext() draws the dynamic values for the traces on 
 the left hand side of the logviewer. This is optimized so that if the value
 becomes temporarily static, it won't keep blindly updating the screen and
 wasting CPU time.
 \param force_draw (gboolean) when true to write the values to screen for
 all controls no matter if hte previous value is the same or not.
 */
void draw_valtext(gboolean force_draw)
{
	gint last_index = 0;
	gfloat val = 0.0;
	gfloat last_val = 0.0;
	gint val_x = 0;
	gint val_y = 0;
	gint info_ctr = 0;
	gint h = 0;
	gint i = 0;
	GArray *array = NULL;
	Viewable_Value *v_value = NULL;
	PangoLayout *layout;
	GdkPixmap *pixmap = lv_data->pixmap;

	h = lv_data->darea->allocation.height;

	if (!lv_data->font_desc)
	{
		lv_data->font_desc = pango_font_description_from_string("courier");
		pango_font_description_set_size(lv_data->font_desc,(10)*PANGO_SCALE);
	}
	
	val_x = 7;
	for (i=0;i<lv_data->active_traces;i++)
	{
		v_value = (Viewable_Value *)g_list_nth_data(lv_data->tlist,i);
		info_ctr = (lv_data->spread * (i+1))- (lv_data->spread/2);
		val_y = info_ctr + 1;

		last_index = v_value->last_index;
		array = DATA_GET(v_value->object,v_value->data_source);
		val = g_array_index(array,gfloat,last_index);
		if (array->len > 1)
			last_val = g_array_index(array,gfloat,last_index-1);
		/* IF this value matches the last one,  don't bother
		 * updating the text as there's no point... */
		if ((val == last_val) && (!force_draw) && (!v_value->force_update))
			continue;
		
		v_value->force_update = FALSE;
		gdk_draw_rectangle(pixmap,
				lv_data->darea->style->black_gc,
				TRUE,
				v_value->ink_rect->x+val_x,
				v_value->ink_rect->y+val_y,
				lv_data->info_width-1-v_value->ink_rect->x-val_x,
				v_value->ink_rect->height);

		layout = gtk_widget_create_pango_layout(lv_data->darea,g_strdup_printf("%1$.*2$f",val,v_value->precision));

		pango_layout_set_font_description(layout,lv_data->font_desc);
		pango_layout_get_pixel_extents(layout,v_value->ink_rect,v_value->log_rect);
		gdk_draw_layout(pixmap,v_value->font_gc,val_x,val_y,layout);
	}

}


/*!
 \brief update_logview_traces_pf() updates each trace in turn and then scrolls 
 the display
 \param force_redraw (gboolean) flag to force all data to be redrawn not 
 just the new data...
 \returns TRUE
 */
EXPORT gboolean update_logview_traces_pf(gboolean force_redraw)
{
	extern gboolean connected;
	extern gboolean interrogated;

	if (playback_mode)
		return TRUE;

	if (!((connected) && (interrogated)))
		return FALSE;
	
	if (!lv_data)
		return FALSE;

	if ((lv_data->traces) && (g_list_length(lv_data->tlist) > 0))
	{
		adj_scale = TRUE;
		g_static_mutex_lock(&update_mutex);
		trace_update(force_redraw);
		g_static_mutex_unlock(&update_mutex);
		scroll_logviewer_traces();
	}

	return TRUE;
}


/*!
 \brief pb_update_logview_traces() updates each trace in turn and then scrolls 
 the display
 \param force_redraw (gboolean) flag to force all data to be redrawn not 
 just the new data...
 \returns TRUE
 */
gboolean pb_update_logview_traces(gboolean force_redraw)
{

	if (!playback_mode)
		return TRUE;
	if ((lv_data->traces) && (g_list_length(lv_data->tlist) > 0))
	{
		adj_scale = TRUE;
		g_static_mutex_lock(&update_mutex);
		trace_update(force_redraw);
		g_static_mutex_unlock(&update_mutex);
		scroll_logviewer_traces();
	}

	return TRUE;
}


/*!
 \brief trace_update() updates a trace onscreen,  this is run for EACH 
 individual trace (yeah, not very optimized)
 \param redraw_all (gpointer) flag to redraw all or just recent data
 */
void trace_update(gboolean redraw_all)
{
	GdkPixmap * pixmap = NULL;
	gint w = 0;
	gint h = 0;
	gfloat val = 0.0;
	gfloat last_val = 0.0;
	gfloat percent = 0.0;
	gfloat last_percent = 0.0;
	gint len = 0;
	gint lo_width;
	gint total = 0;
	guint last_index = 0;
	guint i = 0;
	gint j = 0;
	gint x = 0;
	gfloat log_pos = 0.0;
	gfloat newpos = 0.0;
	GArray *array = NULL;
	GdkPoint pts[2048]; /* Bad idea as static...*/
	Viewable_Value *v_value = NULL;
	gint lv_zoom;
	/*static gulong sig_id = 0;*/
	static GtkWidget *scale = NULL;

	pixmap = lv_data->pixmap;

	lv_zoom = (GINT)DATA_GET(global_data,"lv_zoom");
	/*
	if (sig_id == 0)
		sig_id = g_signal_handler_find(lookup_widget("logviewer_log_position_hscale"),G_SIGNAL_MATCH_FUNC,0,0,NULL,(gpointer)logviewer_log_position_change,NULL);
		*/

	if (!scale)
		scale = lookup_widget("logviewer_log_position_hscale");
	w = lv_data->darea->allocation.width;
	h = lv_data->darea->allocation.height;

	log_pos = (gfloat)((GINT)OBJ_GET(lv_data->darea,"log_pos_x100"))/100.0;
	/*printf("log_pos is %f\n",log_pos);*/
	/* Full screen redraw, only with configure events (usually) */
	if ((gboolean)redraw_all)
	{
		lo_width = lv_data->darea->allocation.width-lv_data->info_width;
		for (i=0;i<g_list_length(lv_data->tlist);i++)
		{
			v_value = (Viewable_Value *)g_list_nth_data(lv_data->tlist,i);
			array = DATA_GET(v_value->object,v_value->data_source);
			len = array->len;
			if (len == 0)	/* If empty */
			{
				return;
			}
			/*printf("length is %i\n", len);*/
			len *= (log_pos/100.0);
			/*printf("length after is  %i\n", len);*/
			/* Determine total number of points 
			 * that'll fit on the window
			 * taking into account the scroll amount
			 */
			total = len < lo_width/lv_zoom ? len : lo_width/lv_zoom;


			/* Draw is reverse order, from right to left, 
			 * easier to think out in my head... :) 
			 */	
			for (x=0;x<total;x++)
			{
				val = g_array_index(array,gfloat,len-1-x);
				percent = 1.0-(val/(float)(v_value->upper-v_value->lower));
				pts[x].x = w-(x*lv_zoom)-1;
				pts[x].y = (gint) (percent*(h-2))+1;
			}
			gdk_draw_lines(pixmap,
					v_value->trace_gc,
					pts,
					total);
			if (v_value->highlight)
			{
				for (j=0;j<total;j++)	
					pts[j].y -= 1;
				gdk_draw_lines(pixmap,
						lv_data->darea->style->white_gc,
						pts,
						total);
				for (j=0;j<total;j++)	
					pts[j].y += 2;
				gdk_draw_lines(pixmap,
						lv_data->darea->style->white_gc,
						pts,
						total);
			}

			v_value->last_y = pts[0].y;
			v_value->last_index = len-1;

			/*printf ("last index displayed was %i from %i,%i to %i,%i\n",v_value->last_index,pts[1].x,pts[1].y, pts[0].x,pts[0].y );*/
		}
		draw_valtext(TRUE);
		/*printf("redraw complete\n");*/
		return;
	}
	/* Playback mode, playing from logfile.... */
	if (playback_mode)
	{
		for (i=0;i<g_list_length(lv_data->tlist);i++)
		{
			v_value = (Viewable_Value *)g_list_nth_data(lv_data->tlist,i);
			array = DATA_GET(v_value->object,v_value->data_source);
			last_index = v_value->last_index;
			if(last_index >= array->len)
				return;

			/*printf("got data from array at index %i\n",last_index+1);*/
			val = g_array_index(array,gfloat,last_index+1);
			percent = 1.0-(val/(float)(v_value->upper-v_value->lower));
			if (val > (v_value->max))
				v_value->max = val;
			if (val < (v_value->min))
				v_value->min = val;

			gdk_draw_line(pixmap,
					v_value->trace_gc,
					w-lv_zoom-1,v_value->last_y,
					w-1,(gint)(percent*(h-2))+1);
			/*printf("drawing from %i,%i to %i,%i\n",w-lv_zoom-1,v_value->last_y,w-1,(gint)(percent*(h-2))+1);*/

			v_value->last_y = (gint)((percent*(h-2))+1);

			v_value->last_index = last_index + 1;
			if (adj_scale)
			{
				newpos = 100.0*((gfloat)(v_value->last_index)/(gfloat)array->len);
				blocked=TRUE;
				gtk_range_set_value(GTK_RANGE(scale),newpos);
				blocked=FALSE;
				OBJ_SET(lv_data->darea,"log_pos_x100",GINT_TO_POINTER((gint)(newpos*100.0)));
				adj_scale = FALSE;
				if (newpos >= 100)
					stop_tickler(LV_PLAYBACK_TICKLER);
				/*	printf("playback reset slider to position %i\n",(gint)(newpos*100.0));*/
			}
			if (v_value->highlight)
			{
				gdk_draw_line(pixmap,
						lv_data->darea->style->white_gc,
						w-lv_zoom-1,v_value->last_y-1,
						w-1,(gint)(percent*(h-2)));
				gdk_draw_line(pixmap,
						lv_data->darea->style->white_gc,
						w-lv_zoom-1,v_value->last_y+1,
						w-1,(gint)(percent*(h-2))+2);
			}
		}
		draw_valtext(FALSE);
		return;
	}

	/* REALTIME mode... all traces updated at once.. */
	for (i=0;i<g_list_length(lv_data->tlist);i++)
	{
		v_value = (Viewable_Value *)g_list_nth_data(lv_data->tlist,i);
		array = DATA_GET(v_value->object,v_value->data_source);
		val = g_array_index(array,gfloat, array->len-1);

		if (val > (v_value->max))
			v_value->max = val;
		if (val < (v_value->min))
			v_value->min = val;

		if (v_value->last_y == -1)
			v_value->last_y = (gint)((percent*(h-2))+1);

		/* If watching at the edge (full realtime) */
		if (log_pos == 100)
		{
			v_value->last_index = array->len-1;
			percent = 1.0-(val/(float)(v_value->upper-v_value->lower));
			gdk_draw_line(pixmap,
					v_value->trace_gc,
					w-lv_zoom-1,v_value->last_y,
					w-1,(gint)(percent*(h-2))+1);
		}
		else
		{	/* Watching somewhat behind realtime... */
			last_index = v_value->last_index;

			last_val = g_array_index(array,gfloat,last_index);
			last_percent = 1.0-(last_val/(float)(v_value->upper-v_value->lower));
			val = g_array_index(array,gfloat,last_index+1);
			percent = 1.0-(val/(float)(v_value->upper-v_value->lower));

			v_value->last_index = last_index + 1;
			gdk_draw_line(pixmap,
					v_value->trace_gc,
					w-lv_zoom-1,(last_percent*(h-2))+1,
					w-1,(gint)(percent*(h-2))+1);
			if (adj_scale)
			{
				newpos = 100.0*((gfloat)v_value->last_index/(gfloat)array->len);
				blocked = TRUE;
				gtk_range_set_value(GTK_RANGE(scale),newpos);
				blocked = FALSE;
				OBJ_SET(lv_data->darea,"log_pos_x100",GINT_TO_POINTER((gint)(newpos*100.0)));
				adj_scale = FALSE;
			}
		}
		/* Draw the data.... */
		v_value->last_y = (gint)((percent*(h-2))+1);
		if (v_value->highlight)
		{
			gdk_draw_line(pixmap,
					lv_data->darea->style->white_gc,
					w-lv_zoom-1,v_value->last_y-1,
					w-1,(gint)(percent*(h-2)));
			gdk_draw_line(pixmap,
					lv_data->darea->style->white_gc,
					w-lv_zoom-1,v_value->last_y+1,
					w-1,(gint)(percent*(h-2))+2);
		}
	}
	/* Update textual data */
	draw_valtext(FALSE);
}


/*!
 \brief scroll_logviewer_traces() scrolls the traces to the left
 */
void scroll_logviewer_traces(void)
{
	gint start = lv_data->info_width;
	gint end = lv_data->info_width;
	gint w = 0;
	gint h = 0;
	gint lv_zoom = 0;
	GdkPixmap *pixmap = NULL;
	GdkPixmap *pmap = NULL;
	static GtkWidget * widget = NULL;

	if (!widget)
		widget = lookup_widget("logviewer_trace_darea");
	if (!widget)
		return;
	pixmap = lv_data->pixmap;
	pmap = lv_data->pmap;
	if (!pixmap)
		return;

	lv_zoom = (GINT)DATA_GET(global_data,"lv_zoom");
	w = widget->allocation.width;
	h = widget->allocation.height;
	start = end + lv_zoom;

	/* NASTY NASTY NASTY win32 hack to get it to scroll because
	 * draw_drawable seems to fuckup on windows when souce/dest are 
	 * in the same widget...  This works however on EVERY OTHER
	 * OS where GTK+ runs.  grr.....
	 */
#ifdef __WIN32__
	/* Scroll the screen to the left... */
	gdk_threads_enter();
	gdk_draw_drawable(pmap,
			widget->style->black_gc,
			pixmap,
			lv_data->info_width+lv_zoom,0,
			lv_data->info_width,0,
			w-lv_data->info_width-lv_zoom,h);

	gdk_draw_drawable(pixmap,
			widget->style->black_gc,
			pmap,
			lv_data->info_width,0,
			lv_data->info_width,0,
			w-lv_data->info_width,h);
#else

	/* Scroll the screen to the left... */
	gdk_draw_drawable(pixmap,
			widget->style->black_gc,
			pixmap,
			lv_data->info_width+lv_zoom,0,
			lv_data->info_width,0,
			w-lv_data->info_width-lv_zoom,h);
#endif

	/* Init new "blank space" as black */
	gdk_draw_rectangle(pixmap,
			widget->style->black_gc,
			TRUE,
			w-lv_zoom,0,
			lv_zoom,h);

	gdk_window_clear(widget->window);
	gdk_threads_leave();
}


/*!
 \brief set_all_lview_choices_state() sets all the logables to either be 
 selected or deselected (select all fucntionality)
 \param widget (GtkWidget *) unused
 \param data (gpointer) state to set the widgets to
 \returns TRUE
 */
gboolean set_all_lview_choices_state(GtkWidget *widget, gpointer data)
{
	gboolean state = (GBOOLEAN)data;

	g_list_foreach(get_list("viewables"),set_widget_active,GINT_TO_POINTER(state));

	return TRUE;
}


/*!
 \brief set_default_lview_choices_state() sets the default logviewer values
 */
void set_default_lview_choices_state(void)
{
	GList *defaults = NULL;
	GList *list = NULL;
	GtkWidget * widget = NULL;
	gint i = 0;
	gint j = 0;
	gchar * name;
	gchar * potential;
	gconstpointer *object;

	defaults = get_list("logviewer_defaults");
	list = get_list("viewables");
	for (i=0;i<g_list_length(defaults);i++)
	{
		name = g_list_nth_data(defaults,i);
		for (j=0;j<g_list_length(list);j++)
		{
			widget = g_list_nth_data(list,j);
			object = OBJ_GET(widget,"object");
			if (playback_mode)
				potential = DATA_GET(object,"lview_name");
			else
				potential = DATA_GET(object,"dlog_gui_name");
			if (g_strcasecmp(name,potential) == 0)

				set_widget_active(GTK_WIDGET(widget),GINT_TO_POINTER(TRUE));
		}
	}
	return;
}


/*!
 \brief logviewer_log_position_change() gets called when the log position 
 slider is moved by the user to fast forware/rewind through a datalog
 \param widget (GtkWidget *) widget that received the event
 \param data (gpointer) unused
 \returns TRUE
 */
EXPORT gboolean logviewer_log_position_change(GtkWidget * widget, gpointer data)
{
	gfloat val = 0.0;
	GtkWidget *darea = NULL;

	/* If we pass "TRUE" as the widget data we just ignore this signal as 
	 * the redraw routine wil have to adjsut the slider as it scrolls 
	 * through the data...
	 */
	if ((GBOOLEAN)data)
		return TRUE;
	if (blocked)
		return TRUE;

	val = gtk_range_get_value(GTK_RANGE(widget));
	darea = lookup_widget("logviewer_trace_darea");
	if (GTK_IS_WIDGET(darea))
		OBJ_SET(darea,"log_pos_x100",GINT_TO_POINTER((gint)(val*100)));
	lv_configure_event(darea,NULL,NULL);
	scroll_logviewer_traces();
	if ((val >= 100.0) && (playback_mode))
		stop_tickler(LV_PLAYBACK_TICKLER);
	return TRUE;
}


/*!
 \brief set_logviewer_mode() sets things up for playback mode
 */
void set_logviewer_mode(Lv_Mode mode)
{
	GtkWidget *widget = NULL;
	GtkAdjustment *adj = NULL;
	gchar *fname = NULL;
	gchar *filename = NULL;
	GladeXML *xml = NULL;
	static GtkWidget * playback_controls_window = NULL;

	reset_logviewer_state();
	free_log_info();
	if (mode == LV_PLAYBACK)
	{
		playback_mode = TRUE;
		gtk_widget_set_sensitive(lookup_widget("logviewer_select_params_button"), FALSE);
		gtk_widget_set_sensitive(lookup_widget("logviewer_select_logfile_button"), TRUE);
		gtk_widget_hide(lookup_widget("logviewer_rt_control_vbox1"));
		gtk_widget_show(lookup_widget("logviewer_playback_control_vbox1"));
		gtk_widget_show(lookup_widget("scroll_speed_vbox"));
		widget = lookup_widget("logviewer_log_position_hscale");
		if (!GTK_IS_WIDGET(playback_controls_window))
		{
			fname = g_build_filename(GUI_DATA_DIR,"logviewer.glade",NULL);
			filename = get_file(g_strdup(fname),NULL);
			if (filename)
			{
				xml = glade_xml_new(filename, "logviewer_controls_window",NULL);
				g_free(filename);
				g_free(fname);
				glade_xml_signal_autoconnect(xml);
				playback_controls_window = glade_xml_get_widget(xml,"logviewer_controls_window");
				OBJ_SET(glade_xml_get_widget(xml,"goto_start_button"),"handler",GINT_TO_POINTER(LV_GOTO_START));
				OBJ_SET(glade_xml_get_widget(xml,"goto_end_button"),"handler",GINT_TO_POINTER(LV_GOTO_END));
				OBJ_SET(glade_xml_get_widget(xml,"rewind_button"),"handler",GINT_TO_POINTER(LV_REWIND));
				OBJ_SET(glade_xml_get_widget(xml,"fast_forward_button"),"handler",GINT_TO_POINTER(LV_FAST_FORWARD));
				OBJ_SET(glade_xml_get_widget(xml,"play_button"),"handler",GINT_TO_POINTER(LV_PLAY));
				OBJ_SET(glade_xml_get_widget(xml,"stop_button"),"handler",GINT_TO_POINTER(LV_STOP));
				register_widget("logviewer_controls_hbox",glade_xml_get_widget(xml,"controls_hbox"));
				widget = lookup_widget("logviewer_scroll_hscale");
				if (GTK_IS_WIDGET(widget))
				{
					adj = gtk_range_get_adjustment(GTK_RANGE(widget));
					gtk_range_set_adjustment(GTK_RANGE(glade_xml_get_widget(xml,"scroll_speed")),adj);
				}
				widget = lookup_widget("logviewer_log_position_hscale");
				if (GTK_IS_WIDGET(widget))
				{
					adj = gtk_range_get_adjustment(GTK_RANGE(widget));
					gtk_range_set_adjustment(GTK_RANGE(glade_xml_get_widget(xml,"log_position_hscale")),adj);
				}

			}

		}
		else
			gtk_widget_show_all(playback_controls_window);

		widget = lookup_widget("logviewer_log_position_hscale");
		if (GTK_IS_RANGE(widget))
			gtk_range_set_value(GTK_RANGE(widget),0.0);
		hue = -60.0;
		col_sat = 1.0;
		col_val = 1.0;
	}
	else if (mode == LV_REALTIME)
	{

		if (GTK_IS_WIDGET(playback_controls_window))
		{
			widget = lookup_widget("logviewer_controls_hbox");
			gtk_widget_set_sensitive(widget,FALSE);
			gtk_widget_hide(playback_controls_window);
		}

		stop_tickler(LV_PLAYBACK_TICKLER);
		playback_mode = FALSE;
		gtk_widget_set_sensitive(lookup_widget("logviewer_select_logfile_button"), FALSE);
		gtk_widget_set_sensitive(lookup_widget("logviewer_select_params_button"), TRUE);
		gtk_widget_show(lookup_widget("logviewer_rt_control_vbox1"));
		gtk_widget_hide(lookup_widget("logviewer_playback_control_vbox1"));
		gtk_widget_hide(lookup_widget("scroll_speed_vbox"));
		widget = lookup_widget("logviewer_log_position_hscale");
		if (GTK_IS_RANGE(widget))
			gtk_range_set_value(GTK_RANGE(widget),100.0);
		hue = -60.0;
		col_sat = 1.0;
		col_val = 1.0;
	}
}


/*!
 \brief finish_logviewer() sets button default states for the logviewer after
 it is created from it's glade config file
 */
EXPORT void finish_logviewer(void)
{
	GtkWidget * widget = NULL;
	gint lv_zoom = 0;

	lv_zoom = (GINT)DATA_GET(global_data,"lv_zoom");

	lv_data = g_new0(Logview_Data,1);
	lv_data->traces = g_hash_table_new(g_str_hash,g_str_equal);
	lv_data->info_width = 120;

	if (playback_mode)
		set_logviewer_mode(LV_PLAYBACK);
	else
		set_logviewer_mode(LV_REALTIME);

	widget = lookup_widget("logviewer_zoom_spinner");
	if (GTK_IS_SPIN_BUTTON(widget))
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),lv_zoom);

	return;
}


/*!
 \brief slider_ket_press_event() doesn't do anything yet (stub)
 */
EXPORT gboolean slider_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	return FALSE;
}



void write_logviewer_defaults(ConfigFile *cfgfile)
{
	GList * list = NULL;
	gint i = 0;
	gchar * name = NULL;
	GString *string = NULL;

	list = get_list("logviewer_defaults");
	if (list)
	{
		string = g_string_new(NULL);
		for (i=0;i<g_list_length(list);i++)
		{
			name = g_list_nth_data(list,i);
			g_string_append(string,name);
			if (i < (g_list_length(list)-1))
				g_string_append(string,",");
		}
		cfg_write_string(cfgfile,"Logviewer","defaults",string->str);
		g_string_free(string,TRUE);
	}
	else
		cfg_write_string(cfgfile,"Logviewer","defaults","");
}


void read_logviewer_defaults(ConfigFile *cfgfile)
{
	gchar *tmpbuf = NULL;
	GList *defaults = NULL;
	gchar **vector = NULL;
	gint i = 0;

	cfg_read_string(cfgfile,"Logviewer","defaults",&tmpbuf);
	if (!tmpbuf)
		return;
	
	vector = g_strsplit(tmpbuf,",",-1);
	g_free(tmpbuf);
	for (i=0;i<g_strv_length(vector);i++)
		defaults = g_list_append(defaults,g_strdup(vector[i]));
	g_strfreev(vector);
	if (defaults)
		store_list("logviewer_defaults",defaults);

}
