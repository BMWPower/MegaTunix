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

#include <config.h>
#include <configfile.h>
#include <datamgmt.h>
#include <defines.h>
#include <debugging.h>
#include <dep_processor.h>
#include <enums.h>


/*!
 \brief check_dependancies() extracts the dependancy information from the 
 object and checks each one in turn until one evauates to false, in that
 case it returns FALSE, otherwise if all deps succeed it'll return TRUE
 \param object (gconstpointer *) object containing dependacy information
 \returns TRUE if dependancy evaluates to TRUE, FALSE on any dep in the chain 
 evaluating to FALSE.
 */
G_MODULE_EXPORT gboolean check_dependancies(gconstpointer *object )
{
	gint i = 0;
	gint page = 0;
	gint offset = 0;
	gint bitmask = 0;
	gint bitshift = 0;
	gint bitval = 0;
	DataSize size = 0;
	gint canID = 0;
	gchar ** deps = NULL;
	gchar * tmpbuf = NULL;
	gint type = 0;
	gint num_deps = 0;

	num_deps = (GINT)DATA_GET(object,"num_deps");
	deps = DATA_GET(object,"deps");
	/*printf("number of deps %i, %i\n",num_deps,g_strv_length(deps));*/
	for (i=0;i<num_deps;i++)
	{
		/*printf("dep name %s\n",deps[i]);*/
		tmpbuf = g_strdup_printf("%s_type",deps[i]);
		type = (GINT)DATA_GET(object,tmpbuf);
		g_free(tmpbuf);
		switch (type)
		{
			case ECU_EMB_BIT:
				/*printf("ECU_EMB_BIT\n");*/
				tmpbuf = g_strdup_printf("%s_page",deps[i]);
				page = (GINT)DATA_GET(object,tmpbuf);
				/*printf("page %i\n",page);*/
				g_free(tmpbuf);

				tmpbuf = g_strdup_printf("%s_offset",deps[i]);
				offset = (GINT)DATA_GET(object,tmpbuf);
				/*printf("offset %i\n",offset);*/
				g_free(tmpbuf);

				tmpbuf = g_strdup_printf("%s_canID",deps[i]);
				canID = (GINT)DATA_GET(object,tmpbuf);
				/*printf("canID %i\n",canID);*/
				g_free(tmpbuf);

				tmpbuf = g_strdup_printf("%s_size",deps[i]);
				size = (DataSize)DATA_GET(object,tmpbuf);
				/*printf("size %i\n",size); */
				g_free(tmpbuf);

				tmpbuf = g_strdup_printf("%s_bitmask",deps[i]);
				bitmask = (GINT)DATA_GET(object,tmpbuf);
				bitshift = get_bitshift_f(bitmask);
				g_free(tmpbuf);

				tmpbuf = g_strdup_printf("%s_bitval",deps[i]);
				bitval = (GINT)DATA_GET(object,tmpbuf);
				/*printf("bitval %i\n",bitval); */
				g_free(tmpbuf);

				if (!(((ms_get_ecu_data(canID,page,offset,size)) & bitmask) >> bitshift) == bitval)	
				{
					/*printf("dep_proc returning FALSE\n");*/
					return FALSE;
				}
				break;
			case ECU_VAR:

				tmpbuf = g_strdup_printf("%s_page",deps[i]);
				page = (GINT)DATA_GET(object,g_strdup_printf("%s_page",deps[i]));
				g_free(tmpbuf);

				tmpbuf = g_strdup_printf("%s_offset",deps[i]);
				offset = (GINT)DATA_GET(object,g_strdup_printf("%s_offset",deps[i]));
				g_free(tmpbuf);
				break;
			default:
				printf(_("CASE NOT HANDLED, dep_processor BUG!\n"));

		}
	}
	/*printf("dep_proc returning TRUE\n");*/
	return TRUE;
}