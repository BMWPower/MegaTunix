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

#include <apicheck.h>
#include <api-versions.h>
#include <config.h>
#include <conversions.h>
#include <defines.h>
#include <debugging.h>
#include <fileio.h>
#include <getfiles.h>
#include <gui_handlers.h>
#include <init.h>
#include <interrogate.h>
#include <listmgmt.h>
#include <lookuptables.h>
#include "../mtxmatheval/mtxmatheval.h"
#include <mode_select.h>
#include <notifications.h>
#include <multi_expr_loader.h>
#include <offline.h>
#include <rtv_map_loader.h>
#include <string.h>
#include <stdlib.h>
#include <tabloader.h>
#include <threads.h>


gchar * offline_firmware_choice = NULL;
volatile gboolean offline = FALSE;
extern gint dbg_lvl;


/*!
 \brief set_offline_mode() is called when the "Offline Mode" button is clicked
 in the general tab and is used to present the user with list of firmware 
 choices to select one for loading to work in offline mode (no connection to
 an ECU)
 */
void set_offline_mode(void)
{
	extern GHashTable *dynamic_widgets;
	GtkWidget * widget = NULL;
	gchar * filename = NULL;
	Detection_Test *test = NULL;
	GArray *tests = NULL;
	GHashTable *tests_hash = NULL;
	gboolean tmp = TRUE;
	extern Firmware_Details *firmware;
	extern gboolean interrogated;
	extern Io_Cmds *cmds;
        extern GAsyncQueue *serial_repair_queue;


	/* Disable interrogation button */

	offline = TRUE;
	/* Cause Serial Searcher thread to abort.... */
	g_async_queue_push(serial_repair_queue,&tmp);

	filename = present_firmware_choices();
	if (!filename)
	{
		offline = FALSE;
		interrogated = FALSE;
		widget = g_hash_table_lookup(dynamic_widgets,"interrogate_button");
		if (GTK_IS_WIDGET(widget))
			gtk_widget_set_sensitive(GTK_WIDGET(widget),TRUE);
		widget = g_hash_table_lookup(dynamic_widgets,"offline_button");
		if (GTK_IS_WIDGET(widget))
			gtk_widget_set_sensitive(GTK_WIDGET(widget),TRUE);
		return;

	}

	widget = g_hash_table_lookup(dynamic_widgets,"interrogate_button");
	if (GTK_IS_WIDGET(widget))
		gtk_widget_set_sensitive(GTK_WIDGET(widget),FALSE);
	queue_function(g_strdup("kill_conn_warning"));

	tests = validate_and_load_tests(&tests_hash);
	if (!firmware)
		firmware = g_new0(Firmware_Details,1);
	load_firmware_details(firmware,filename);
	update_interrogation_gui(firmware,tests_hash);

	if (firmware->rt_cmd_key)
	{
		test = (Detection_Test *)g_hash_table_lookup(tests_hash,firmware->rt_cmd_key);
		if (test)
		{
			cmds->realtime_cmd = g_strdup(test->test_vector[0]);
			cmds->rt_cmd_len = g_strv_length(test->test_vector);
			firmware->rtvars_size = test->num_bytes;
		}
	}
	test = NULL;
	if (firmware->ve_cmd_key)
	{
		test = (Detection_Test *)g_hash_table_lookup(tests_hash,firmware->ve_cmd_key);
		if (test)
		{
			cmds->veconst_cmd = g_strdup(test->test_vector[0]);
			cmds->ve_cmd_len = g_strv_length(test->test_vector);
		}
	}

	interrogated = TRUE;

	io_cmd(IO_LOAD_REALTIME_MAP,NULL);
	io_cmd(IO_LOAD_GUI_TABS,NULL);
	io_cmd(IO_UPDATE_VE_CONST,NULL);

	widget = g_hash_table_lookup(dynamic_widgets,"interrogate_button");
	if (GTK_IS_WIDGET(widget))
		gtk_widget_set_sensitive(GTK_WIDGET(widget),FALSE);
	widget = g_hash_table_lookup(dynamic_widgets,"offline_button");
	if (GTK_IS_WIDGET(widget))
		gtk_widget_set_sensitive(GTK_WIDGET(widget),FALSE);
	g_list_foreach(get_list("get_data_buttons"),set_widget_sensitive,GINT_TO_POINTER(FALSE));

	offline_ecu_restore(NULL,NULL);
	free_tests_array(tests);

}


/*!
 \brief present_firmware_choices() presents a dialog box with the firmware
 choices.
 \returns the name of the chosen firmware
 */
gchar * present_firmware_choices()
{
	gchar ** filenames = NULL;
	GtkWidget *dialog_window = NULL;
	GtkWidget *dialog = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *hbox = NULL;
	GtkWidget *ebox = NULL;
	GtkWidget *sep = NULL;
	GtkWidget *button = NULL;
	GtkWidget *label = NULL;
	gchar *tmpbuf = NULL;
	GArray *classes = NULL;
	GSList *group = NULL;
	ConfigFile *cfgfile = NULL;
	gint major = 0;
	gint minor = 0;
	gint i = 0;
	gint result = 0;

	extern gchar * offline_firmware_choice;


	filenames = get_files(g_strconcat(INTERROGATOR_DATA_DIR,PSEP,"Profiles",PSEP,NULL),g_strdup("prof"),&classes);
	if (!filenames)
	{
		if (dbg_lvl & CRITICAL)
			dbg_func(g_strdup_printf(__FILE__": present_firmware_choices()\n\t NO Interrogation profiles found, was MegaTunix installed properly?\n\n"));
		return NULL;
	}

	dialog_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	dialog = gtk_dialog_new_with_buttons("Select Firmware",
				GTK_WINDOW(dialog_window),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				"Load",
				GTK_RESPONSE_OK,
				NULL);

	vbox = gtk_vbox_new(TRUE,5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox),5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),vbox,TRUE,TRUE,0);
	label = gtk_label_new("Custom (personal) Profiles");
	gtk_box_pack_start(GTK_BOX(vbox),label,TRUE,TRUE,0);

	group = NULL;
	i = 0;
	/* Cycle list for PERSONAL interogation files */
	while (filenames[i]) 
	{
		cfgfile = cfg_open_file(filenames[i]);
		if (!cfgfile)
		{
			if (dbg_lvl & CRITICAL)
				dbg_func(g_strdup_printf(__FILE__": present_firmware_choices()\n\t Interrogation profile damaged!, was MegaTunix installed properly?\n\n"));
			i++;
			continue;
		}
		get_file_api(cfgfile,&major,&minor);
		if ((major != INTERROGATE_MAJOR_API) || (minor != INTERROGATE_MINOR_API))
		{
			thread_update_logbar("interr_view","warning",g_strdup_printf("Interrogation profile API mismatch (%i.%i != %i.%i):\n\tFile %s will be skipped\n",major,minor,INTERROGATE_MAJOR_API,INTERROGATE_MINOR_API,cfgfile->filename),FALSE,FALSE);
			i++;
			continue;
		}
		cfg_read_string(cfgfile,"interrogation_profile","name",&tmpbuf);
		cfg_free(cfgfile);
		g_free(cfgfile);

		if (g_array_index(classes,FileClass,i) == PERSONAL)
		{
			ebox = gtk_event_box_new();
			gtk_box_pack_start(GTK_BOX(vbox),ebox,TRUE,TRUE,0);
			hbox = gtk_hbox_new(FALSE,10);
			gtk_container_add(GTK_CONTAINER(ebox),hbox);
			label = gtk_label_new(g_strdup(tmpbuf));
			g_free(tmpbuf);
			gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
			button = gtk_radio_button_new(group);
			g_free(g_object_get_data(G_OBJECT(button),"filename"));
			g_object_set_data(G_OBJECT(button),"filename",g_strdup(filenames[i]));
			g_object_set_data(G_OBJECT(button),"handler",
					GINT_TO_POINTER(OFFLINE_FIRMWARE_CHOICE));
			g_signal_connect(button,
					"toggled",
					G_CALLBACK(toggle_button_handler),
					NULL);
			gtk_box_pack_end(GTK_BOX(hbox),button,FALSE,TRUE,0);
			group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
		}
		i++;
	}

	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox),sep,TRUE,TRUE,0);
	label = gtk_label_new("System Wide Profiles");
	gtk_box_pack_start(GTK_BOX(vbox),label,TRUE,TRUE,0);
	/* Cycle list for System interogation files */
	i = 0;
	while (filenames[i]) 
	{
		cfgfile = cfg_open_file(filenames[i]);
		if (!cfgfile)
		{
			if (dbg_lvl & CRITICAL)
				dbg_func(g_strdup_printf(__FILE__": present_firmware_choices()\n\t Interrogation profile damaged!, was MegaTunix installed properly?\n\n"));
			i++;
			continue;
		}
		get_file_api(cfgfile,&major,&minor);
		if ((major != INTERROGATE_MAJOR_API) || (minor != INTERROGATE_MINOR_API))
		{
			thread_update_logbar("interr_view","warning",g_strdup_printf("Interrogation profile API mismatch (%i.%i != %i.%i):\n\tFile %s will be skipped\n",major,minor,INTERROGATE_MAJOR_API,INTERROGATE_MINOR_API,cfgfile->filename),FALSE,FALSE);
			i++;
			continue;
		}
		cfg_read_string(cfgfile,"interrogation_profile","name",&tmpbuf);
		cfg_free(cfgfile);
		g_free(cfgfile);

		if (g_array_index(classes,FileClass,i) == SYSTEM)
		{
			ebox = gtk_event_box_new();
			gtk_box_pack_start(GTK_BOX(vbox),ebox,TRUE,TRUE,0);
			hbox = gtk_hbox_new(FALSE,10);
			gtk_container_add(GTK_CONTAINER(ebox),hbox);
			label = gtk_label_new(g_strdup(tmpbuf));
			g_free(tmpbuf);
			gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);
			button = gtk_radio_button_new(group);
			g_free(g_object_get_data(G_OBJECT(button),"filename"));
			g_object_set_data(G_OBJECT(button),"filename",g_strdup(filenames[i]));
			g_object_set_data(G_OBJECT(button),"handler",
					GINT_TO_POINTER(OFFLINE_FIRMWARE_CHOICE));
			g_signal_connect(button,
					"toggled",
					G_CALLBACK(toggle_button_handler),
					NULL);
			gtk_box_pack_end(GTK_BOX(hbox),button,FALSE,TRUE,0);
			group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
		}
		i++;
	}



	if (i==1)
		gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(button));
	else
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button),TRUE);
	g_strfreev(filenames);
	g_array_free(classes,TRUE);
	
	gtk_widget_show_all(dialog);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	gtk_widget_destroy(dialog_window);
	switch (result)
	{
		case GTK_RESPONSE_ACCEPT:
		case GTK_RESPONSE_OK:
			return offline_firmware_choice;
			break;
		default:
			return NULL;
	}

}

gint ptr_sort(gconstpointer a, gconstpointer b)
{
	return strcmp((gchar *)a, (gchar *) b);
}


gboolean offline_ecu_restore(GtkWidget *widget, gpointer data)
{
	MtxFileIO *fileio = NULL;
	gchar *filename = NULL;
	extern gboolean interrogated;
	extern GtkWidget *main_window;

	if (!interrogated)
		return FALSE;

	fileio = g_new0(MtxFileIO ,1);
	fileio->external_path = g_strdup("MTX_ecu_snapshots");
	fileio->parent = main_window;
	fileio->on_top = TRUE;
	fileio->title = g_strdup("Offline mode REQUIRES you to load ECU Settings from a file");
	fileio->action = GTK_FILE_CHOOSER_ACTION_OPEN;
	fileio->shortcut_folders = g_strdup("ecu_snapshots,MTX_ecu_snapshots");

	while (!filename)
		filename = choose_file(fileio);


	update_logbar("tools_view",NULL,g_strdup("Full Restore of ECU Initiated\n"),FALSE,FALSE);
	restore_all_ecu_settings(filename);
	g_free(filename);
	free_mtxfileio(fileio);
	return TRUE;

}
