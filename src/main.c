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
#include <comms_gui.h>
#include <config.h>
#include <conversions.h>
#include <core_gui.h>
#include <defines.h>
#include <debugging.h>
#include <dispatcher.h>
#include <enums.h>
#include <errno.h>
#include <gdk/gdkgl.h>
#include <getfiles.h>
#include <glade/glade.h>
#include <gui_handlers.h>
#include <init.h>
#include <locking.h>
#include <main.h>
#include <mtxsocket.h>
#include <serialio.h>
#include <stringmatch.h>
#include <threads.h>
#include <timeout_handlers.h>
#include <xmlcomm.h>

GThread * thread_dispatcher_id = NULL;
GThread * socket_thread_id = NULL;
gboolean ready = FALSE;
gint pf_dispatcher_id = -1;
gint gui_dispatcher_id = -1;
gboolean gl_ability = FALSE;
Serial_Params *serial_params = NULL;
GAsyncQueue *io_queue = NULL;
GAsyncQueue *pf_dispatch_queue = NULL;
GAsyncQueue *gui_dispatch_queue = NULL;
GObject *global_data = NULL;

/*!
 \brief main() is the typical main function in a C program, it performs
 all core initialization, loading of all main parameters, initializing handlers
 and entering gtk_main to process events until program close
 \param argc (gint) count of command line arguments
 \param argv (char **) array of command line args
 \returns TRUE
 */
gint main(gint argc, gchar ** argv)
{
	gchar * filename = NULL;
	gint socket = 0;

	if(!g_thread_supported())
		g_thread_init(NULL);

	gtk_init(&argc, &argv);
	glade_init();

	gl_ability = gdk_gl_init_check(&argc, &argv);

	gtk_set_locale();

	global_data = g_object_new(GTK_TYPE_INVISIBLE,NULL);
	g_object_ref(global_data);
	gtk_object_sink(GTK_OBJECT(global_data));
	handle_args(argc,argv);

	/* This will exit mtx if the locking fails! */
	create_mtx_lock();

	/* Allocate memory  */
	serial_params = g_malloc0(sizeof(Serial_Params));

	open_debug();	/* Open debug log */
	init();			/* Initialize global vars */
	make_megasquirt_dirs();	/* Create config file dirs if missing */
	/* Build table of strings to enum values */
	build_string_2_enum_table();

	filename = get_file(g_build_filename(INTERROGATOR_DATA_DIR,"comm.xml",NULL),NULL);
	load_comm_xml(filename);
	g_free(filename);

	/* Create Queue to listen for commands */
	io_queue = g_async_queue_new();
	pf_dispatch_queue = g_async_queue_new();
	gui_dispatch_queue = g_async_queue_new();

	read_config();
	setup_gui();		

	/* Startup the serial General I/O handler.... */
	thread_dispatcher_id = g_thread_create(thread_dispatcher,
			NULL, /* Thread args */
			TRUE, /* Joinable */
			NULL); /*GError Pointer */

	pf_dispatcher_id = g_timeout_add(50,(GtkFunction)pf_dispatcher,NULL);
	gui_dispatcher_id = g_timeout_add(30,(GtkFunction)gui_dispatcher,NULL);

	/* Open TCP socket for remote access */
	socket = setup_socket();
	if (socket)
	{
		socket_thread_id = g_thread_create(socket_thread_manager,
				GINT_TO_POINTER(socket), /* Thread args */
	                        TRUE, /* Joinable */
	                        NULL); /*GError Pointer */
	}


	/* Kickoff fast interrogation */
	g_timeout_add(500,(GtkFunction)early_interrogation,NULL);

	ready = TRUE;
	gtk_main();
	return (0) ;
}
