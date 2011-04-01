/*
 * Copyright (C) 2002-2011 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
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

#include <config.h>
#include <configfile.h>
#include <debugging.h>
#include <defines.h>
#include <init.h>
#include <multi_expr_loader.h>
#include <enums.h>
#include <keyparser.h>
#include <stdlib.h>
#include <string.h>
#include <stringmatch.h>



/*!
 \brief multi_expr_loader() is called when a "multi_expr_keys" key is found in
 a realtimemap, and triggers the loading of al lthe keys/values that
 will allow megatunix to process a special variable that requires handling of
 multiple circumstances
 \param object (gconstpointer *) place to store the retrieved data
 \param cfgfile (ConfigFile *) pointer to cfgfile that contains the data
 \param section (gchar *) sectio nto read the data from
 \see check_dependancies
 */
G_MODULE_EXPORT void load_multi_expressions(gconstpointer *object, ConfigFile *cfgfile,gchar * section)
{
	gchar *tmpbuf = NULL;
	gchar ** keys = NULL;
	gchar ** l_limits = NULL;
	gchar ** u_limits = NULL;
	gchar ** ltables = NULL;
	gchar ** ul_mults = NULL;
	gchar ** ul_adds = NULL;
	gint num_keys = 0;
	gint i = 0;
	gint lowest = 0;
	gint highest = 0;
	GHashTable *hash = NULL;
	MultiExpr *multi = NULL;

	if (!cfg_read_string(cfgfile,section,"multi_expr_keys",&tmpbuf))
	{
		dbg_func(CRITICAL,g_strdup_printf(__FILE__": load_multi_expressions()\n\t Can't find \"multi_expr_keys\" in the \"[%s]\" section, exiting!\n",section));
		exit (-4);
	}
	else
	{
		keys = parse_keys(tmpbuf,&num_keys,",");
		g_free(tmpbuf);
	}



	if (!cfg_read_string(cfgfile,section,"lower_limits",&tmpbuf))
	{
		dbg_func(CRITICAL,g_strdup_printf(__FILE__": load_multi_expression()\n\t Key \"lower_limits\" NOT FOUND in section \"[%s]\", EXITING!!\n",section));
		exit (-4);
	}
	else
	{
		l_limits = g_strsplit(tmpbuf,",",-1);
		g_free(tmpbuf);
	}
	if (!cfg_read_string(cfgfile,section,"upper_limits",&tmpbuf))
	{
		dbg_func(CRITICAL,g_strdup_printf(__FILE__": load_multi_expression()\n\t Key \"upper_limits\" NOT FOUND in section \"[%s]\", EXITING!!\n",section));
		exit (-4);
	}
	else
	{
		u_limits = g_strsplit(tmpbuf,",",-1);
		g_free(tmpbuf);
	}
	if (!cfg_read_string(cfgfile,section,"multi_lookuptables",&tmpbuf))
	{
		dbg_func(CRITICAL,g_strdup_printf(__FILE__": load_multi_expression()\n\t Key \"multi_lookuptables\" NOT FOUND in section \"[%s]\", EXITING!!\n",section));
		exit (-4);
	}
	else
	{
		ltables = g_strsplit(tmpbuf,",",-1);
		g_free(tmpbuf);
	}
	if (!cfg_read_string(cfgfile,section,"fromecu_mults",&tmpbuf))
	{
		dbg_func(CRITICAL,g_strdup_printf(__FILE__": load_multi_expression()\n\t Key \"fromecu_mults\" NOT FOUND in section \"[%s]\", EXITING!!\n",section));
		exit (-4);
	}
	else
	{
		ul_mults = g_strsplit(tmpbuf,",",-1);
		g_free(tmpbuf);
	}
	if (!cfg_read_string(cfgfile,section,"fromecu_adds",&tmpbuf))
	{
		dbg_func(CRITICAL,g_strdup_printf(__FILE__": load_multi_expression()\n\t Key \"fromecu_adds\" NOT FOUND in section \"[%s]\", EXITING!!\n",section));
		exit (-4);
	}
	else
	{
		ul_adds = g_strsplit(tmpbuf,",",-1);
		g_free(tmpbuf);
	}
	/* Create hash table to store structures for each one */
	hash = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,free_multi_expr);
	lowest = G_MAXINT32;
	highest = G_MININT32;
	for (i=0;i<num_keys;i++)
	{
		multi = g_new0(MultiExpr, 1);
		multi->lower_limit = (GINT)strtol(l_limits[i],NULL,10);
		multi->upper_limit = (GINT)strtol(u_limits[i],NULL,10);
		if (multi->lower_limit < lowest)
			lowest = multi->lower_limit;
		if (multi->upper_limit > highest)
			highest = multi->upper_limit;

		if (strlen(ltables[i]) == 0)
			multi->lookuptable = NULL;
		else
			multi->lookuptable = g_strdup(ltables[i]);
		multi->fromecu_mult = g_new0(gfloat, 1);
		multi->fromecu_add = g_new0(gfloat, 1);
		*multi->fromecu_mult = (gfloat)g_strtod(ul_mults[i],NULL);
		*multi->fromecu_add = (gfloat)g_strtod(ul_adds[i],NULL);

		g_hash_table_insert(hash,g_strdup(keys[i]),multi);
	}
	DATA_SET_FULL(object,"real_lower",g_strdup_printf("%i",lowest),g_free);
	DATA_SET_FULL(object,"real_upper",g_strdup_printf("%i",highest),g_free);
	g_strfreev(l_limits);
	g_strfreev(u_limits);
	g_strfreev(ltables);
	g_strfreev(ul_mults);
	g_strfreev(ul_adds);
	g_strfreev(keys);
	DATA_SET_FULL(object,"multi_expr_hash",hash,g_hash_table_destroy);
}

G_MODULE_EXPORT void free_multi_expr(gpointer data)
{
	MultiExpr *multi = (MultiExpr *)data;
	cleanup(multi->fromecu_mult);	
	cleanup(multi->fromecu_add);	
	cleanup(multi->lookuptable);	
	cleanup(multi);
}


G_MODULE_EXPORT void free_multi_source(gpointer data)
{
	MultiSource *multi = (MultiSource *)data;
	cleanup(multi->source);	
	cleanup(multi->fromecu_mult);	
	cleanup(multi->fromecu_add);	
	cleanup(multi->suffix);
	cleanup(multi);
}
