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
#include <debugging.h>
#include <enums.h>
#include <gui_handlers.h>
#include <runtime_gui.h>
#include <logviewer_gui.h>
#include <notifications.h>
#include <structures.h>
#include <timeout_handlers.h>
#include <threads.h>

gint realtime_id = 0;
static gint forced_id = 0;
static gint update_rate = 24;


void start_realtime_tickler()
{
	extern struct Serial_Params *serial_params;

	if (realtime_id == 0)
	{
		realtime_id = g_timeout_add(serial_params->read_wait,
				(GtkFunction)signal_read_rtvars,NULL);
		update_logbar("comms_view",NULL,"Realtime Reader started\n",TRUE,FALSE);
	}
	else
	{
		dbg_func(__FILE__": start_realtime_tickler()\n\tTickler already running\n",CRITICAL);
		update_logbar("comms_view","warning","Realtime Reader ALREADY started\n",TRUE,FALSE);
	}
}

void stop_realtime_tickler()
{
	if (realtime_id)
	{
		g_source_remove(realtime_id);
		update_logbar("comms_view",NULL,"Realtime Reader stopped\n",TRUE,FALSE);
		realtime_id = 0;
	}
	else
	{
		dbg_func(__FILE__": stop_realtime_tickler()\n\tTickler already stopped...\n",CRITICAL);
		update_logbar("comms_view","warning","Realtime Reader ALREADY stopped\n",TRUE,FALSE);
	}
}

gboolean signal_read_rtvars()
{
	gint length = 0;
	extern gboolean connected;
	extern GAsyncQueue *io_queue;
	static gint errcount = 0;

	length = g_async_queue_length(io_queue);
	/* IF queue depth is too great we should not make the problem worse
	 * so we skip a call as we're probably trying to go faster than the 
	 * MS and/or serail port can go....
	 */
	if (length > 2)
		return TRUE;

	dbg_func(__FILE__": signal_read_rtvars()\n\tsending message to thread to read RT vars\n",SERIAL_RD|SERIAL_WR);

	if (errcount >10)
	{
		stop_realtime_tickler();
		errcount = 0;
	}
	if (connected)
		io_cmd(IO_REALTIME_READ,NULL);			
	else
	{	
		if (errcount == 1)
		{
			io_cmd(IO_COMMS_TEST,NULL);
			errcount++;
			return TRUE;
		}
		errcount++;
		dbg_func(__FILE__": signal_read_rtvars()\n\tNOT connected, not queing message to thread handler....\n",CRITICAL);

	}

	return TRUE;	/* Keep going.... */
}

void force_an_update()
{
      extern gboolean forced_update;
      if (forced_id == 0)
      {
              forced_update = TRUE;
              gtk_timeout_add((int)((3.0/update_rate)*1000.0),
                                (GtkFunction)cancel_forced_update,NULL);
      }
}

gboolean cancel_forced_update()
{
        extern gboolean forced_update;
        forced_update = FALSE;
        return FALSE;
}

gboolean early_interrogation()
{
	io_cmd(IO_INTERROGATE_ECU,NULL);
	return FALSE;
}

