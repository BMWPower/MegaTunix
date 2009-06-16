/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * Most of this file contributed by Perry Harrington
 * slight changes applied (naming, addition ofbspot 1-3 vars)
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

#include <assert.h>
#include <config.h>
#include <configfile.h>
#include <datamgmt.h>
#include <defines.h>
#include <debugging.h>
#include <dep_processor.h>
#include <enums.h>
#include <firmware.h>
#include <glade/glade.h>
#include <gui_handlers.h>
#include <lookuptables.h>
#include <math.h>
#include "../mtxmatheval/mtxmatheval.h"
#include <multi_expr_loader.h>
#include <notifications.h>
#include <rtv_map_loader.h>
#include <rtv_processor.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>


extern GStaticMutex rtv_mutex;
extern GObject *global_data;
/*!
 \brief process_rt_vars() processes incoming realtime variables. It's a pretty
 complex function so read the sourcecode.. ;)
 \param incoming (void *) pointer to the raw incoming data
 */
void process_rt_vars(void *incoming)
{
	extern Rtv_Map *rtv_map;
	extern Firmware_Details *firmware;
	gint temp_units;
	guchar *raw_realtime = incoming;
	GObject * object = NULL;
	gchar * expr = NULL;
	GList * list= NULL;
	gint i = 0;
	gint j = 0;
	gfloat x = 0;
	gint offset = 0;
	DataSize size = MTX_U08;
	gfloat result = 0.0;
	gfloat tmpf = 0.0;
	gboolean temp_dep = FALSE;
	void *evaluator = NULL;
	GTimeVal timeval;
	gint current_index;
	GArray *history = NULL;
	gchar *special = NULL;
	GHashTable *hash = NULL;
	GTimeVal curr;
	GTimeVal begin;
	gint hours = 0;
	gint minutes = 0;
	gint seconds = 0;

	if (!incoming)
		printf("ERROR  INOPUT IS NULL!!!!\n");
	/* Store timestamps in ringbuffer */

	/* Backup current rtv copy */
	memcpy(firmware->rt_data_last,firmware->rt_data,firmware->rtvars_size);
	memcpy(firmware->rt_data,incoming,firmware->rtvars_size);
	temp_units = (gint)OBJ_GET(global_data,"temp_units");
	g_get_current_time(&timeval);
	g_array_append_val(rtv_map->ts_array,timeval);
	if (rtv_map->ts_array->len%250 == 0)
	{
		curr = g_array_index(rtv_map->ts_array,GTimeVal, rtv_map->ts_array->len-1);
		begin = g_array_index(rtv_map->ts_array,GTimeVal,0);
		tmpf = curr.tv_sec-begin.tv_sec-((curr.tv_usec-begin.tv_usec)/1000000);
		hours = tmpf > 3600.0 ? floor(tmpf/3600.0) : 0;
		tmpf -= hours*3600.0;
		minutes = tmpf > 60.0 ? floor(tmpf/60.0) : 0;
		tmpf -= minutes*60.0;
		seconds = (gint)tmpf;

		thread_update_logbar("dlog_view",NULL,g_strdup_printf("Currently %i samples stored, Total Logged Time (HH:MM:SS) (%02i:%02i:%02i)\n",rtv_map->ts_array->len,hours,minutes,seconds),FALSE,FALSE);
	}

	for (i=0;i<rtv_map->rtvars_size;i++)
	{
		/* Get list of derived vars for raw offset "i" */
		list = g_hash_table_lookup(rtv_map->offset_hash,GINT_TO_POINTER(i));
		if (list == NULL) /* no derived vars for this variable */
			continue;
		list = g_list_first(list);
		for (j=0;j<g_list_length(list);j++)
		{
			history = NULL;
			special = NULL;
			hash = NULL;
			object=(GObject *)g_list_nth_data(list,j);
			if (!GTK_IS_OBJECT(object))
			{
				dbg_func(COMPLEX_EXPR|CRITICAL,g_strdup_printf(__FILE__": rtv_processor()\n\t Object bound to list at offset %i is invalid!!!!\n",i));
				continue;
			}
			special = (gchar *)OBJ_GET(object,"special");
			if (special)
			{
				tmpf = handle_special(object,special);
				goto store_it;
			}
			temp_dep = (gboolean)OBJ_GET(object,"temp_dep");
			hash = (GHashTable *)OBJ_GET(object,"multi_expr_hash");
			if (hash)
			{
				tmpf = handle_multi_expression(object,raw_realtime,hash);
				goto store_it;
			}
			evaluator = (void *)OBJ_GET(object,"ul_evaluator");
			if (!evaluator)
			{
				expr = OBJ_GET(object,"ul_conv_expr");
				if (expr == NULL)
				{
					dbg_func(COMPLEX_EXPR|CRITICAL,g_strdup_printf(__FILE__": process_rt_vars()\n\t \"ul_conv_expr\" was NULL for control \"%s\", EXITING!\n",(gchar *)OBJ_GET(object,"internal_names")));
					exit (-3);
				}
				evaluator = evaluator_create(expr);
				if (!evaluator)
				{
					dbg_func(COMPLEX_EXPR|CRITICAL,g_strdup_printf(__FILE__": rtv_processor()\n\t Creating of evaluator for function \"%s\" FAILED!!!\n\n",expr));
				}
				assert(evaluator);
				OBJ_SET(object,"ul_evaluator",evaluator);
			}
			else
				assert(evaluator);
			offset = (gint)OBJ_GET(object,"offset");
			size = (DataSize)OBJ_GET(object,"size");
			if (OBJ_GET(object,"complex_expr"))
			{
				tmpf = handle_complex_expr(object,incoming,UPLOAD);
				goto store_it;
			}

			if (OBJ_GET(object,"lookuptable"))
			{
				/*dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": process_rt_vars()\n\tgetting Lookuptable for var using offset %i\n",offset));*/
				x = lookup_data(object,raw_realtime[offset]);
			}
			else
			{
				/*dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": process_rt_vars()\n\tNo Lookuptable needed for var using offset %i\n",offset));*/
				x = _get_sized_data((guint8 *)incoming,0,offset,size);
			}

			/*dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": process_rt_vars()\n\texpression is %s\n",evaluator_get_string(evaluator))); */
			tmpf = evaluator_evaluate_x(evaluator,x);
store_it:
			if (temp_dep)
			{
				/*dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": process_rt_vars()\n\tvar at offset %i is temp dependant.\n",offset));*/
				if (temp_units == CELSIUS)
					result = (tmpf-32.0)*(5.0/9.0);
				else
					result = tmpf;
			}
			else
				result = tmpf;
			/* Get history array and current index point */
			history = (GArray *)OBJ_GET(object,"history");
			current_index = (gint)OBJ_GET(object,"current_index");
			/* Store data in history buffer */
			dbg_func(MUTEX,g_strdup_printf(__FILE__": process_rt_vars() before lock rtv_mutex\n"));
			g_static_mutex_lock(&rtv_mutex);
			dbg_func(MUTEX,g_strdup_printf(__FILE__": process_rt_vars() after lock rtv_mutex\n"));
			g_array_append_val(history,result);
			/*printf("array size %i, current index %i, appended %f, readback %f previous %f\n",history->len,current_index,result,g_array_index(history, gfloat, current_index+1),g_array_index(history, gfloat, current_index));*/
			current_index++;
			OBJ_SET(object,"current_index",GINT_TO_POINTER(current_index));
			dbg_func(MUTEX,g_strdup_printf(__FILE__": process_rt_vars() before UNlock rtv_mutex\n"));
			g_static_mutex_unlock(&rtv_mutex);
			dbg_func(MUTEX,g_strdup_printf(__FILE__": process_rt_vars() after UNlock rtv_mutex\n"));

			/*printf("Result of %s is %f\n",(gchar *)OBJ_GET(object,"internal_names"),result);*/

		}
	}
	return;
}


/*!
 \brief handle_complex_expr() handles a complex mathematcial expression for
 an variable represented by a GObject.
 \param object (GObject *) pointer to the object containing the conversion 
 expression and other relevant data
 \param incoming (void *) pointer to the raw data
 \param type (ConvType) enumeration stating if this is an upload or
 download conversion
 \returns a float of the result of the mathematical expression
 */
gfloat handle_complex_expr(GObject *object, void * incoming,ConvType type)
{
	gchar **symbols = NULL;
	gint *expr_types = NULL;
	guchar *raw_data = incoming;
	gint total_symbols = 0;
	gint i = 0;
	gint page = 0;
	DataSize size = MTX_U08;
	gint offset = 0;
	guint bitmask = 0;
	guint bitshift = 0;
	gint canID = 0;
	void * evaluator = NULL;
	gchar **names = NULL;
	gdouble * values = NULL;
	gchar * tmpbuf = NULL;
	gint lower_limit = 0;
	gint upper_limit = 0;
	gdouble result = 0.0;


	symbols = (gchar **)OBJ_GET(object,"expr_symbols");
	expr_types = (gint *)OBJ_GET(object,"expr_types");
	total_symbols = (gint)OBJ_GET(object,"total_symbols");
	lower_limit = (gint)OBJ_GET(object,"lower_limit");
	upper_limit = (gint)OBJ_GET(object,"upper_limit");

	names = g_new0(gchar *, total_symbols);
	values = g_new0(gdouble, total_symbols);

	for (i=0;i<total_symbols;i++)
	{
		page = 0;
		offset = 0;
		bitmask = 0;
		bitshift = 0;
		switch ((ComplexExprType)expr_types[i])
		{
			case VE_EMB_BIT:
				size = MTX_U08;

				tmpbuf = g_strdup_printf("%s_page",symbols[i]);
				page = (gint) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_offset",symbols[i]);
				offset = (gint) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_canID",symbols[i]);
				canID = (gint) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_bitmask",symbols[i]);
				bitmask = (gint) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				bitshift = get_bitshift(bitmask);
				names[i]=g_strdup(symbols[i]);
				values[i]=(gdouble)(((get_ecu_data(canID,page,offset,size)) & bitmask) >> bitshift);
				//printf("raw ecu at page %i, offset %i is %i\n",page,offset,get_ecu_data(canID,page,offset,size));
				//printf("value masked by %i, shifted by %i is %i\n",bitmask,bitshift,(get_ecu_data(canID,page,offset,size) & bitmask) >> bitshift);
				dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": handle_complex_expr()\n\t Embedded bit, name: %s, value %f\n",names[i],values[i]));
				break;
			case VE_VAR:
				tmpbuf = g_strdup_printf("%s_page",symbols[i]);
				page = (gint) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_offset",symbols[i]);
				offset = (gint) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_canID",symbols[i]);
				canID = (gint) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_size",symbols[i]);
				size = (DataSize) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				names[i]=g_strdup(symbols[i]);
				values[i]=(gdouble)get_ecu_data(canID,page,offset,size);
				dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": handle_complex_expr()\n\t VE Variable, name: %s, value %f\n",names[i],values[i]));
				break;
			case RAW_VAR:
				tmpbuf = g_strdup_printf("%s_offset",symbols[i]);
				offset = (gint) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_size",symbols[i]);
				size = (DataSize) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				names[i]=g_strdup(symbols[i]);
				values[i]=(gdouble)_get_sized_data(raw_data,0,offset,size);
				dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": handle_complex_expr()\n\t RAW Variable, name: %s, value %f\n",names[i],values[i]));
				break;
			case RAW_EMB_BIT:
				size = MTX_U08;
				tmpbuf = g_strdup_printf("%s_offset",symbols[i]);
				offset = (gint) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_bitmask",symbols[i]);
				bitmask = (gint) OBJ_GET(object,tmpbuf);
				g_free(tmpbuf);
				bitshift = get_bitshift(bitmask);
				names[i]=g_strdup(symbols[i]);
				values[i]=(gdouble)(((_get_sized_data(raw_data,0,offset,size)) & bitmask) >> bitshift);
				dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": handle_complex_expr()\n\t RAW Embedded Bit, name: %s, value %f\n",names[i],values[i]));
				break;
			default:
				dbg_func(COMPLEX_EXPR|CRITICAL,g_strdup_printf(__FILE__": handle_complex_expr()\n\t UNDEFINE Variable, this will cause a crash!!!!\n"));
				break;
		}

	}
	if (type == UPLOAD)
	{
		evaluator = (void *)OBJ_GET(object,"ul_evaluator");
		if (!evaluator)
		{
			evaluator = evaluator_create(OBJ_GET(object,"ul_conv_expr"));
			OBJ_SET(object,"ul_evaluator",evaluator);

		}
	}
	else if (type == DOWNLOAD)
	{
		evaluator = (void *)OBJ_GET(object,"dl_evaluator");
		if (!evaluator)
		{
			evaluator = evaluator_create(OBJ_GET(object,"dl_conv_expr"));
			OBJ_SET(object,"dl_evaluator",evaluator);
		}
	}
	else
	{
		dbg_func(COMPLEX_EXPR|CRITICAL,g_strdup_printf(__FILE__": handle_complex_expr()\n\tevaluator type undefined for %s\n",(gchar *)glade_get_widget_name(GTK_WIDGET(object))));
	}
	if (!evaluator)
	{
		dbg_func(COMPLEX_EXPR|CRITICAL,g_strdup_printf(__FILE__": handle_complex_expr()\n\tevaluator missing for %s\n",(gchar *)glade_get_widget_name(GTK_WIDGET(object))));
		exit (-1);
	}

	assert(evaluator);

	result = evaluator_evaluate(evaluator,total_symbols,names,values);
	if (result < lower_limit)
		result = lower_limit;
	if (result > upper_limit)
		result = upper_limit;
	for (i=0;i<total_symbols;i++)
	{
		dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": handle_complex_expr()\n\tkey %s value %f\n",names[i],values[i]));
		g_free(names[i]);
	}
	dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": handle_complex_expr()\n\texpression is %s\n",evaluator_get_string(evaluator)));
	g_free(names);
	g_free(values);
	return result;
}

/*!
 \brief handle_multi_expression() is used to handle RT Vars that take
 multiple possible conversions based on ECU state
 \param object (GObject *) object representing this derived variable
 \param handler_name (gchar *) string name of special handler case to be done
 */
gfloat handle_multi_expression(GObject *object,guchar* raw_realtime,GHashTable *hash)
{
	MultiExpr *multi = NULL;
	gint offset = 0;
	gfloat result = 0.0;
	gfloat x = 0.0;
	gchar *key = NULL;
	gchar *hash_key = NULL;
	extern GHashTable *sources_hash;

	if (!GTK_IS_OBJECT(object))
	{
		dbg_func(COMPLEX_EXPR,g_strdup_printf("__FILE__ ERROR: multi_expression object is NULL!\n"));
		return 0.0;
	}
	key = (gchar *)OBJ_GET(object,"source_key");
	if (!key)
	{
		dbg_func(COMPLEX_EXPR,g_strdup_printf("__FILE__ ERROR: multi_expression source key is NULL!\n"));
		return 0.0;
	}
	hash_key  = (gchar *)g_hash_table_lookup(sources_hash,key);
	if (!hash_key)
	{
		dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": ERROR: multi_expression hash key is NULL!\n"));
		printf(__FILE__": ERROR: multi_expression hash key is NULL!\n");
		return 0.0;
	}
	multi = (MultiExpr *)g_hash_table_lookup(hash,hash_key);
	if (!multi)
	{
		dbg_func(COMPLEX_EXPR,g_strdup_printf(__FILE__": handle_multi_expression()\n\t data struct NOT found for key \"%s\"\n",hash_key));
		return 0.0;
	}

	offset = (gint)OBJ_GET(object,"offset");
	if (multi->lookuptable)
		x = direct_lookup_data(multi->lookuptable,raw_realtime[offset]);
	else
		x = (float)raw_realtime[offset];

	 result = evaluator_evaluate_x(multi->ul_eval,x);

	 return result;
}




/*!
 \brief handle_special() is used to handle special derived variables that
 DO NOT use any data fromthe realtime variables.  In this case it's only to
 create the high resoluation clock variable.
 \param object (GObject *) object representing this derived variable
 \param handler_name (gchar *) string name of special handler case to be done
 */
gfloat handle_special(GObject *object,gchar *handler_name)
{
	static GTimeVal now;
	static GTimeVal last;
	static gfloat cumu = 0.0;
	extern gboolean begin;

	if (g_strcasecmp(handler_name,"hr_clock")==0)
	{
		if (begin == TRUE)
		{       
			g_get_current_time(&now);
			last.tv_sec = now.tv_sec;
			last.tv_usec = now.tv_usec;
			begin = FALSE;
			return 0.0;
		}
		else
		{
			g_get_current_time(&now);
			cumu += (now.tv_sec-last.tv_sec)+
				((double)(now.tv_usec-last.tv_usec)/1000000.0);
			last.tv_sec = now.tv_sec;
			last.tv_usec = now.tv_usec;
			return cumu;
		}

	}
	else
	{
		dbg_func(CRITICAL,g_strdup_printf(__FILE__": handle_special()\n\t handler name is not recognized, \"%s\"\n",handler_name));
	}
	return 0.0;
}


/*!
 \brief lookup_current_value() gets the current value of the derived
 variable requested by name.
 \param internal_name (gchar *) name of the variable to get the data for.
 \param value (gflaot *) where to put the value
 \returns TRUE on successful lookup, FALSE on failure
 */
gboolean lookup_current_value(gchar *internal_name, gfloat *value)
{
	extern Rtv_Map *rtv_map;
	GObject * object = NULL;
	GArray * history = NULL;
	gint index = 0;
	
	if (!internal_name)
	{
		*value = 0.0;
		return FALSE;
	}
	object = g_hash_table_lookup(rtv_map->rtv_hash,internal_name);
	if (!object)
	{
		*value = -1;
		return FALSE;
	}
	history = (GArray *)OBJ_GET(object,"history");
	index = (gint)OBJ_GET(object,"current_index");
	if (!history)
		return FALSE;
	if (index < 0)
		return FALSE;
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_current_value() before lock rtv_mutex\n"));
	g_static_mutex_lock(&rtv_mutex);
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_current_value() after lock rtv_mutex\n"));
	*value = g_array_index(history,gfloat,index);
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_current_value() before UNlock rtv_mutex\n"));
	g_static_mutex_unlock(&rtv_mutex);
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_current_value() after UNlock rtv_mutex\n"));
	return TRUE;
}


/*!
 \brief lookup_previous_value() gets the current value of the derived
 variable requested by name.
 \param internal_name (gchar *) name of the variable to get the data for.
 \param value (gflaot *) where to put the value
 \returns TRUE on successful lookup, FALSE on failure
 */
gboolean lookup_previous_value(gchar *internal_name, gfloat *value)
{
	extern Rtv_Map *rtv_map;
	GObject * object = NULL;
	GArray * history = NULL;
	gint index = 0;

	if (!internal_name)
	{
		*value = 0.0;
		return FALSE;
	}
	object = g_hash_table_lookup(rtv_map->rtv_hash,internal_name);
	if (!object)
		return FALSE;
	history = (GArray *)OBJ_GET(object,"history");
	index = (gint)OBJ_GET(object,"current_index");
	if (!history)
		return FALSE;
	if (index < 0)
		return FALSE;
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_value() before lock rtv_mutex\n"));
	g_static_mutex_lock(&rtv_mutex);
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_value() after lock rtv_mutex\n"));
	if (index > 0)
		index -= 1;  /* get PREVIOUS one */
	*value = g_array_index(history,gfloat,index);
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_value() before UNlock rtv_mutex\n"));
	g_static_mutex_unlock(&rtv_mutex);
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_value() after UNlock rtv_mutex\n"));
	return TRUE;
}


/*!
 \brief lookup_previous_nth_value() gets the nth previosu value of the derived
 variable requested by name. i.e. if n = 0 it gets current,  n=5 means
 5 samples "back in time"
 \param internal_name (gchar *) name of the variable to get the data for.
 \param value (gflaot *) where to put the value
 \returns TRUE on successful lookup, FALSE on failure
 */
gboolean lookup_previous_nth_value(gchar *internal_name, gint n, gfloat *value)
{
	extern Rtv_Map *rtv_map;
	GObject * object = NULL;
	GArray * history = NULL;
	gint index = 0;

	if (!internal_name)
	{
		*value = 0.0;
		return FALSE;
	}
	object = g_hash_table_lookup(rtv_map->rtv_hash,internal_name);
	if (!object)
		return FALSE;
	history = (GArray *)OBJ_GET(object,"history");
	index = (gint)OBJ_GET(object,"current_index");
	if (!history)
		return FALSE;
	if (index < 0)
		return FALSE;
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_nth_value() before lock rtv_mutex\n"));
	g_static_mutex_lock(&rtv_mutex);
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_nth_value() after lock rtv_mutex\n"));
	if (index > n)
		index -= n;  /* get PREVIOUS nth one */
	*value = g_array_index(history,gfloat,index);
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_nth_value() before UNlock rtv_mutex\n"));
	g_static_mutex_unlock(&rtv_mutex);
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_nth_value() after UNlock rtv_mutex\n"));
	history = (GArray *)OBJ_GET(object,"history");
	return TRUE;
}


/*!
 \brief lookup_previous_n_values() gets the n previous values of the derived
 variable requested by name. i.e. if n = 0 it gets current,  n=5 means
 5 samples "back in time"
 \param internal_name (gchar *) name of the variable to get the data for.
 \param value (gflaot *) where to put the value
 \returns TRUE on successful lookup, FALSE on failure
 */
gboolean lookup_previous_n_values(gchar *internal_name, gint n, gfloat *values)
{
	extern Rtv_Map *rtv_map;
	GObject * object = NULL;
	GArray * history = NULL;
	gint index = 0;
	gint i = 0;

	if (!internal_name)
	{
		for (i=0;i<n;i++)
			values[i] = 0.0;
		return FALSE;
	}
	object = g_hash_table_lookup(rtv_map->rtv_hash,internal_name);
	if (!object)
		return FALSE;
	history = (GArray *)OBJ_GET(object,"history");
	index = (gint)OBJ_GET(object,"current_index");
	if (!history)
		return FALSE;
	if (index < 0)
		return FALSE;
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_n_values() before lock rtv_mutex\n"));
	g_static_mutex_lock(&rtv_mutex);
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_n_values() after lock rtv_mutex\n"));
	if (index > n)
	{
		for (i=0;i<n;i++)
		{
			index--;  /* get PREVIOUS nth one */
			values[i] = g_array_index(history,gfloat,index);
		}
	}
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_n_values() before UNlock rtv_mutex\n"));
	g_static_mutex_unlock(&rtv_mutex);
	dbg_func(MUTEX,g_strdup_printf(__FILE__": lookup_previous_n_values() after UNlock rtv_mutex\n"));
	return TRUE;
}


/*!
 \brief lookup_precision() gets the current precision of the derived
 variable requested by name.
 \param internal_name (gchar *) name of the variable to get the data for.
 \param value (gint *) where to put the value
 \returns TRUE on successful lookup, FALSE on failure
 */
gboolean lookup_precision(gchar *internal_name, gint *precision)
{
	extern Rtv_Map *rtv_map;
	GObject * object = NULL;

	if (!internal_name)
	{
		*precision = 0;
		return FALSE;
	}
	object = g_hash_table_lookup(rtv_map->rtv_hash,internal_name);
	if (!object)
	{
		*precision = 0;
		return FALSE;
	}
	*precision = (gint )OBJ_GET(object,"precision");
	return TRUE;
}


/*!
 \brief flush_rt_arrays() flushed the history buffers for all the realtime
 variables
 */
void flush_rt_arrays()
{
	extern Rtv_Map *rtv_map;
	GArray *history = NULL;
	gint i = 0;
	gint j = 0;
	GObject * object = NULL;
	gint current_index = 0;
	GList *list = NULL;

	/* Flush and recreate the timestamp array */
	g_array_free(rtv_map->ts_array,TRUE);
	rtv_map->ts_array = g_array_sized_new(FALSE,TRUE,sizeof(GTimeVal),4096);

	for (i=0;i<rtv_map->rtvars_size;i++)
	{
		/* Get list of derived vars for raw offset "i" */
		list = g_hash_table_lookup(rtv_map->offset_hash,GINT_TO_POINTER(i));
		if (list == NULL) /* no derived vars for this variable */
			continue;
		list = g_list_first(list);
		for (j=0;j<g_list_length(list);j++)
		{
			object=(GObject *)g_list_nth_data(list,j);
			if (!GTK_IS_OBJECT(object))
				continue;
			dbg_func(MUTEX,g_strdup_printf(__FILE__": flush_rt_arrays() before lock rtv_mutex\n"));
			g_static_mutex_lock(&rtv_mutex);
			dbg_func(MUTEX,g_strdup_printf(__FILE__": flush_rt_arrays() after lock rtv_mutex\n"));
			history = (GArray *)OBJ_GET(object,"history");
			current_index = (gint)OBJ_GET(object,"current_index");
			/* TRuncate array,  but don't free/recreate as it
			 * makes the logviewer explode!
			 */
			g_array_free(history,TRUE);
			history = g_array_sized_new(FALSE,TRUE,sizeof(gfloat),4096);
			OBJ_SET(object,"history",(gpointer)history);
			OBJ_SET(object,"current_index",GINT_TO_POINTER(-1));
			dbg_func(MUTEX,g_strdup_printf(__FILE__": flush_rt_arrays() before UNlock rtv_mutex\n"));
			g_static_mutex_unlock(&rtv_mutex);
			dbg_func(MUTEX,g_strdup_printf(__FILE__": flush_rt_arrays() after UNlock rtv_mutex\n"));
	                /* bind history array to object for future retrieval */
		}

	}
	update_logbar("dlog_view","warning",g_strdup("Realtime Variables History buffers flushed...\n"),FALSE,FALSE);

}
