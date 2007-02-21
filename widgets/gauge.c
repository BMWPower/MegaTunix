/*
 * Copyright (C) 2006 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 * and Christopher Mire, 2006
 *
 * Megasquirt gauge widget
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 *  
 *
 * -------------------------------------------------------------------------
 *  Hacked and slashed to hell by David J. Andruczyk in order to bend and
 *  tweak it to my needs for MegaTunix.  Added in rendering ability using
 *  cairo and raw GDK callls for those less fortunate (OS-X)
 *  Added a HUGE number of functions to get/set every gauge attribute
 *
 *
 *  Was offered a fine contribution by Ari Karhu 
 *  "ari <at> ultimatevw <dot> com" from the msefi.com forums.
 *  His contribution made the gauges look, ohh so much nicer than I could 
 *  have come up with!
 */


#include <config.h>
#ifdef HAVE_CAIRO
#include <cairo/cairo.h>
#endif
#include <gauge.h>
#include <gauge-private.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib-object.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <string.h>



/*!
 \brief sets the color for the index passed.  The index to use used is an opaque enum
 inside of gauge.h
 \param gauge (MtxGaugeFace *) pointer to gauge
 \param index (gint) index of color to set.
 \param color (GdkColor) new color to use for the specified index
 */
void mtx_gauge_face_set_color (MtxGaugeFace *gauge, ColorIndex index, GdkColor color)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	gauge->colors[index].red = color.red;
	gauge->colors[index].green = color.green;
	gauge->colors[index].blue = color.blue;
	gauge->colors[index].pixel = color.pixel;
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
}


/*!
 \brief gets the color for the index passed.  The index to use used is an opaque enum
 inside of gauge.h
 \param gauge (MtxGaugeFace *) pointer to gauge
 \param index (gint) index of color to set.
 \returns a pointer to the internal GdkColor struct WHICH MUST NOT be FREED!!
 */
GdkColor* mtx_gauge_face_get_color (MtxGaugeFace *gauge, ColorIndex index)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge),NULL);
	return (&gauge->colors[index]);
}


/*!
 \brief gets the current value 
 \param gauge (MtxGaugeFace *) pointer to gauge
 */
gfloat mtx_gauge_face_get_value (MtxGaugeFace *gauge)
{
	g_return_val_if_fail ((MTX_IS_GAUGE_FACE (gauge)),0.0);
	return	gauge->value;
}


/*!
 \brief sets the current value 
 \param gauge (MtxGaugeFace *) pointer to gauge
 \param value (gfloat) new value
 */
void mtx_gauge_face_set_value (MtxGaugeFace *gauge, gfloat value)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	if (value > gauge->ubound)
		gauge->value = gauge->ubound;
	else if (value < gauge->lbound)
		gauge->value = gauge->lbound;
	else
		gauge->value = value;
	g_object_thaw_notify (G_OBJECT (gauge));
	mtx_gauge_face_redraw_canvas (gauge);
}


/*!
 \brief adds a new color range between the limits specified in the struct passed
 \param gauge (MtxGaugeFace *) pointer to gauge
 \param range (MtxColorRange*) pointer to color range struct to copy
 \returns index of where this range is stored...
 */
gint mtx_gauge_face_set_color_range_struct(MtxGaugeFace *gauge, MtxColorRange *range)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge),-1);

	g_object_freeze_notify (G_OBJECT (gauge));
	MtxColorRange * newrange = g_new0(MtxColorRange, 1);
	newrange = g_memdup(range,sizeof(MtxColorRange)); 
	g_array_append_val(gauge->c_ranges,newrange);
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return gauge->c_ranges->len-1;
}


/*!
 \brief adds a new text block  from the struct passed
 \param gauge, MtxGaugeFace * pointer to gauge
 \param tblock, MtxTextBlock * pointer to text block struct to copy
 \returns index of where this text block is stored...
 */
gint mtx_gauge_face_set_text_block_struct(MtxGaugeFace *gauge, MtxTextBlock *tblock)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge),-1);

	g_object_freeze_notify (G_OBJECT (gauge));
	MtxTextBlock * new_tblock = g_new0(MtxTextBlock, 1);
	new_tblock->font = g_strdup(tblock->font);
	new_tblock->text = g_strdup(tblock->text);
	new_tblock->color = tblock->color;
	new_tblock->font_scale = tblock->font_scale;
	new_tblock->x_pos = tblock->x_pos;
	new_tblock->y_pos = tblock->y_pos;
	g_array_append_val(gauge->t_blocks,new_tblock);
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return gauge->t_blocks->len-1;
}


/*!
 \brief adds a new tick group from the struct passed
 \param gauge, MtxGaugeFace * pointer to gauge
 \param tblock, MtxTickGroup * pointer to tick group struct to copy
 \returns index of where this text block is stored...
 */
gint mtx_gauge_face_set_tick_group_struct(MtxGaugeFace *gauge, MtxTickGroup *tgroup)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge),-1);

	g_object_freeze_notify (G_OBJECT (gauge));
	MtxTickGroup * new_tgroup = g_new0(MtxTickGroup, 1);
	new_tgroup->text = g_strdup(tgroup->text);
	new_tgroup->text_color = tgroup->text_color;
	new_tgroup->text_inset = tgroup->text_inset;
	new_tgroup->font = g_strdup(tgroup->font);
	new_tgroup->font_scale = tgroup->font_scale;
	new_tgroup->num_maj_ticks = tgroup->num_maj_ticks;
	new_tgroup->maj_tick_color = tgroup->maj_tick_color;
	new_tgroup->maj_tick_inset = tgroup->maj_tick_inset;
	new_tgroup->maj_tick_length = tgroup->maj_tick_length;
	new_tgroup->maj_tick_width = tgroup->maj_tick_width;
	new_tgroup->num_min_ticks = tgroup->num_min_ticks;
	new_tgroup->min_tick_color = tgroup->min_tick_color;
	new_tgroup->min_tick_inset = tgroup->min_tick_inset;
	new_tgroup->min_tick_length = tgroup->min_tick_length;
	new_tgroup->min_tick_width = tgroup->min_tick_width;
	new_tgroup->start_angle = tgroup->start_angle;
	new_tgroup->sweep_angle = tgroup->sweep_angle;
	g_array_append_val(gauge->tick_groups,new_tgroup);
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return gauge->tick_groups->len-1;
}



/*!
 \brief adds a polygon from the struct passed
 \param gauge, MtxGaugeFace * pointer to gauge
 \param tblock, MtxPolygon * pointer to polygon struct to copy
 \returns index of where this text block is stored...
 */
gint mtx_gauge_face_set_polygon_struct(MtxGaugeFace *gauge, MtxPolygon *poly)
{
	gint size = 0;
	MtxGenPoly * new = NULL;
	MtxGenPoly * temp = NULL;
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge),-1);

	g_object_freeze_notify (G_OBJECT (gauge));
	MtxPolygon * new_poly = g_new0(MtxPolygon, 1);
	new_poly->type = poly->type;
	new_poly->color = poly->color;
	new_poly->filled = poly->filled;
	/* Get size of struct to copy */
	switch(poly->type)
	{
		case MTX_CIRCLE:
			size = sizeof(MtxCircle);
			break;
		case MTX_ARC:
			size = sizeof(MtxArc);
			break;
		case MTX_RECTANGLE:
			size = sizeof(MtxRectangle);
			break;
		case MTX_GENPOLY:
			size = sizeof(MtxGenPoly);
			break;
		default:
			break;
	}
	new_poly->data = g_memdup(poly->data,size);
	/* Generic polygons have a dynmically allocated part,  copy it */
	if (poly->type == MTX_GENPOLY)
	{
		new = (MtxGenPoly *)new_poly->data;
		temp = (MtxGenPoly *)poly->data;
		new->points = g_memdup(temp->points,sizeof(MtxPoint)*temp->num_points);
		//printf("copied over %i MTxPoints (%i bytes) \n",temp->num_points,sizeof(MtxPoint)*temp->num_points);
	}
	g_array_append_val(gauge->polygons,new_poly);
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return gauge->polygons->len-1;
}


/*!
 \brief returns the array of ranges to the requestor
 \param gauge (MtxGaugeFace *), pointer to gauge object
 \returns GArray * of the ranges,  DO NOT FREE THIS.
 */
GArray * mtx_gauge_face_get_color_ranges(MtxGaugeFace *gauge)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge),NULL);
	return gauge->c_ranges;
}


/*!
 \brief returns the array of textblocks to the requestor
 \param gauge (MtxGaugeFace *), pointer to gauge object
 \returns GArray * of the textblocks,  DO NOT FREE THIS.
 */
GArray * mtx_gauge_face_get_text_blocks(MtxGaugeFace *gauge)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge),NULL);
	return gauge->t_blocks;
}


/*!
 \brief returns the array of tickgroups to the requestor
 \param gauge (MtxGaugeFace *), pointer to gauge object
 \returns GArray * of the tickgroups,  DO NOT FREE THIS.
 */
GArray * mtx_gauge_face_get_tick_groups(MtxGaugeFace *gauge)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge),NULL);
	return gauge->tick_groups;
}


/*!
 \brief returns the array of polygons to the requestor
 \param gauge (MtxGaugeFace *), pointer to gauge object
 \returns GArray * of the polygons,  DO NOT FREE THIS.
 */
GArray * mtx_gauge_face_get_polygons(MtxGaugeFace *gauge)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge),NULL);
	return gauge->polygons;
}


/*!
 \brief changes an attribute based on passed enum
 \param gauge  pointer to gauge object
 \param field,  enumeration of the field to change
 \param value, new value
 */
void mtx_gauge_face_set_attribute(MtxGaugeFace *gauge,MtxGenAttr field, gfloat value)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_return_if_fail (field < NUM_ATTRIBUTES);
	g_return_if_fail (field >= 0);
	g_object_freeze_notify (G_OBJECT (gauge));

	switch (field)
	{
		case START_ANGLE:
			gauge->start_angle = value;
			break;
		case SWEEP_ANGLE:
			gauge->sweep_angle = value;
			break;
		case LBOUND:
			gauge->lbound = value;
			break;
		case UBOUND:
			gauge->ubound = value;
			break;
		case NEEDLE_TAIL:
			gauge->needle_tail = value;
			break;
		case NEEDLE_LEN:
			gauge->needle_length = value;
			break;
		case NEEDLE_TIP_WIDTH:
			gauge->needle_tip_width = value;
			break;
		case NEEDLE_TAIL_WIDTH:
			gauge->needle_tail_width =value;
			break;
		case VALUE_FONTSCALE:
			gauge->value_font_scale = value;
			break;
		case VALUE_XPOS:
			gauge->value_xpos = value;
			break;
		case VALUE_YPOS:
			gauge->value_ypos = value;
			break;
			/*
		case VALUE_FONT:
			g_free(gauge->value_font);
			gauge->value_font = g_strdup(value);
			break;
			*/
		case PRECISION:
			gauge->precision = (gint)value;
			break;
		case ANTIALIAS:
			gauge->antialias = (gint)value;
			break;
		case SHOW_VALUE:
			gauge->show_value = (gint)value;
			break;
		default:
			break;
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;
}


/*!
 \brief gets and attribute based on passed enum
 \param gauge  pointer to gauge object
 \param field,  enumeration of the field to change
 \param value, new value
 */
gboolean mtx_gauge_face_get_attribute(MtxGaugeFace *gauge,MtxGenAttr field, gfloat * value)
{
	g_return_val_if_fail ((MTX_IS_GAUGE_FACE (gauge)),FALSE);
	g_return_val_if_fail ((field < NUM_ATTRIBUTES),FALSE);
	g_return_val_if_fail ((field >= 0),FALSE);
	g_object_freeze_notify (G_OBJECT (gauge));

	switch (field)
	{
		case START_ANGLE:
			*value = gauge->start_angle;
			break;
		case SWEEP_ANGLE:
			*value = gauge->sweep_angle;
			break;
		case LBOUND:
			*value = gauge->lbound;
			break;
		case UBOUND:
			*value = gauge->ubound;
			break;
		case NEEDLE_TAIL:
			*value = gauge->needle_tail;
			break;
		case NEEDLE_LEN:
			*value = gauge->needle_length;
			break;
		case NEEDLE_TIP_WIDTH:
			*value = gauge->needle_tip_width;
			break;
		case NEEDLE_TAIL_WIDTH:
			*value = gauge->needle_tail_width;
			break;
		case VALUE_FONTSCALE:
			*value = gauge->value_font_scale;
			break;
		case VALUE_XPOS:
			*value = gauge->value_xpos;
			break;
		case VALUE_YPOS:
			*value = gauge->value_ypos;
			break;
			/*
		case VALUE_FONT:
			value = g_strdup(gauge->value_font);
			break;
			*/
		case PRECISION:
			*value = (gfloat)gauge->precision;
			break;
		case ANTIALIAS:
			*value = (gfloat)gauge->antialias;
			break;
		case SHOW_VALUE:
			*value = (gfloat)gauge->show_value;
			break;
		default:
			return FALSE;
			break;
	}
	return TRUE;
}


/*!
 \brief changes a text block field by passing in an index 
 to the test block and the field name to change
 \param gauge  pointer to gauge object
 \param index, index of the textblock
 \param field,  enumeration of the field to change
 \param value, new value
 */
void mtx_gauge_face_alter_text_block(MtxGaugeFace *gauge, gint index,TbField field, void * value)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_return_if_fail (field < TB_NUM_FIELDS);
	g_return_if_fail (field >= 0);
	g_object_freeze_notify (G_OBJECT (gauge));

	MtxTextBlock *tblock = NULL;
	tblock = g_array_index(gauge->t_blocks,MtxTextBlock *,index);
	g_return_if_fail (tblock != NULL);
	switch (field)
	{
		case TB_FONT_SCALE:
			tblock->font_scale = *(gfloat *)value;
			break;
		case TB_X_POS:
			tblock->x_pos = *(gfloat *)value;
			break;
		case TB_Y_POS:
			tblock->y_pos = *(gfloat *)value;
			break;
		case TB_COLOR:
			tblock->color = *(GdkColor *)value;
			break;
		case TB_FONT:
			g_free(tblock->font);
			tblock->font = g_strdup(value);
			break;
		case TB_TEXT:
			g_free(tblock->text);
			tblock->text = g_strdup(value);
			break;
		default:
			break;
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;
}


/*!
 \brief changes a tick group field by passing in an index 
 to the tick group and the field name to change
 \param gauge  pointer to gauge object
 \param index, index of the tickgroup
 \param field,  enumeration of the field to change
 \param value, new value
 */
void mtx_gauge_face_alter_tick_group(MtxGaugeFace *gauge, gint index,TgField field, void * value)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_return_if_fail (field < TG_NUM_FIELDS);
	g_return_if_fail (field >= 0);
	g_object_freeze_notify (G_OBJECT (gauge));

	MtxTickGroup *tgroup = NULL;
	tgroup = g_array_index(gauge->tick_groups,MtxTickGroup *,index);
	g_return_if_fail (tgroup != NULL);
	switch (field)
	{
		case TG_FONT:
			g_free(tgroup->font);
			tgroup->font = g_strdup(value);
			break;
		case TG_TEXT:
			g_free(tgroup->text);
			tgroup->text = g_strdup(value);
			break;
		case TG_TEXT_COLOR:
			tgroup->text_color = *(GdkColor *)value;
			break;
		case TG_TEXT_INSET:
			tgroup->text_inset = *(gfloat *)value;
			break;
		case TG_FONT_SCALE:
			tgroup->font_scale = *(gfloat *)value;
			break;
		case TG_NUM_MAJ_TICKS:
			tgroup->num_maj_ticks = (gint)(*(gfloat *)value);
			break;
		case TG_MAJ_TICK_COLOR:
			tgroup->maj_tick_color = *(GdkColor *)value;
			break;
		case TG_MAJ_TICK_INSET:
			tgroup->maj_tick_inset = *(gfloat *)value;
			break;
		case TG_MAJ_TICK_WIDTH:
			tgroup->maj_tick_width = *(gfloat *)value;
			break;
		case TG_MAJ_TICK_LENGTH:
			tgroup->maj_tick_length = *(gfloat *)value;
			break;
		case TG_NUM_MIN_TICKS:
			tgroup->num_min_ticks = (gint)(*(gfloat *)value);
			break;
		case TG_MIN_TICK_COLOR:
			tgroup->min_tick_color = *(GdkColor *)value;
			break;
		case TG_MIN_TICK_INSET:
			tgroup->min_tick_inset = *(gfloat *)value;
			break;
		case TG_MIN_TICK_WIDTH:
			tgroup->min_tick_width = *(gfloat *)value;
			break;
		case TG_MIN_TICK_LENGTH:
			tgroup->min_tick_length = *(gfloat *)value;
			break;
		case TG_START_ANGLE:
			tgroup->start_angle = *(gfloat *)value;
			break;
		case TG_SWEEP_ANGLE:
			tgroup->sweep_angle = *(gfloat *)value;
			break;
		default:
			break;
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;
}



/*!
 \brief changes a polygon field by passing in an index 
 to the polygon and the field name to change
 \param gauge  pointer to gauge object
 \param index, index of the tickgroup
 \param field,  enumeration of the field to change
 \param value, new value
 */
void mtx_gauge_face_alter_polygon(MtxGaugeFace *gauge, gint index,PolyField field, void * value)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_return_if_fail (field < POLY_NUM_FIELDS);
	g_return_if_fail (field >= 0);
	g_object_freeze_notify (G_OBJECT (gauge));

	MtxPolygon *poly = NULL;
	void *data = NULL;
	gint i = 0;
	gint num_points = 0;

	poly = g_array_index(gauge->polygons,MtxPolygon *,index);
	g_return_if_fail (poly != NULL);
	data = poly->data;
	switch (field)
	{
		case POLY_COLOR:
			poly->color = *(GdkColor *)value;
			break;
		case POLY_FILLED:
			poly->filled = (gint)(*(gfloat *)value);
			break;
		case POLY_LINESTYLE:
			poly->line_style = (gint)(*(gfloat *)value);
			break;
		case POLY_JOINSTYLE:
			poly->join_style = (gint)(*(gfloat *)value);
			break;
		case POLY_LINEWIDTH:
			poly->line_width = *(gfloat *)value;
			break;
		case POLY_X:
			if (poly->type == MTX_CIRCLE)
				((MtxCircle *)data)->x = *(gfloat *)value;
			if (poly->type == MTX_ARC)
				((MtxArc *)data)->x = *(gfloat *)value;
			if (poly->type == MTX_RECTANGLE)
				((MtxRectangle *)data)->x = *(gfloat *)value;
			break;
		case POLY_Y:
			if (poly->type == MTX_CIRCLE)
				((MtxCircle *)data)->y = *(gfloat *)value;
			if (poly->type == MTX_ARC)
				((MtxArc *)data)->y = *(gfloat *)value;
			if (poly->type == MTX_RECTANGLE)
				((MtxRectangle *)data)->y = *(gfloat *)value;
			break;
		case POLY_WIDTH:
			if (poly->type == MTX_ARC)
				((MtxArc *)data)->width = *(gfloat *)value;
			if (poly->type == MTX_RECTANGLE)
				((MtxRectangle *)data)->width = *(gfloat *)value;
			break;
		case POLY_HEIGHT:
			if (poly->type == MTX_ARC)
				((MtxArc *)data)->height = *(gfloat *)value;
			if (poly->type == MTX_RECTANGLE)
				((MtxRectangle *)data)->height = *(gfloat *)value;
			break;
		case POLY_RADIUS:
			if (poly->type == MTX_CIRCLE)
				((MtxCircle *)data)->radius = *(gfloat *)value;
			break;
		case POLY_START_ANGLE:
			if (poly->type == MTX_ARC)
				((MtxArc *)data)->start_angle = *(gfloat *)value;
			break;
		case POLY_SWEEP_ANGLE:
			if (poly->type == MTX_ARC)
				((MtxArc *)data)->sweep_angle = *(gfloat *)value;
			break;
		case POLY_NUM_POINTS:
			if (poly->type == MTX_GENPOLY)
			{
				num_points = ((MtxGenPoly *)data)->num_points;
				((MtxGenPoly *)data)->num_points = (gint)(*(gfloat *)value);
				((MtxGenPoly *)data)->points = g_renew(MtxPoint,((MtxGenPoly *)data)->points,((MtxGenPoly *)data)->num_points);
				for (i=num_points;i<((MtxGenPoly *)data)->num_points;i++)
				{
					((MtxGenPoly *)data)->points[i].x = 0.0;
					((MtxGenPoly *)data)->points[i].y = 0.0;
				}
			}
			break;
		case POLY_POINTS:
			if (poly->type == MTX_GENPOLY)
			{
				if (((MtxGenPoly *)data)->points)
					g_free(((MtxGenPoly *)data)->points);
				((MtxGenPoly *)data)->points = g_memdup(value,((MtxGenPoly *)data)->num_points *sizeof(MtxPoint));
			}
			break;
		default:
			break;
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;
}


/*!
 \brief changes a color range field by passing in an index 
 to the test block and the field name to change
 \param gauge  pointer to gauge object
 \param index, index of the textblock
 \param field,  enumeration of the field to change
 \param value, new value
 */
void mtx_gauge_face_alter_color_range(MtxGaugeFace *gauge, gint index,CrField field, void * value)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_return_if_fail (field < CR_NUM_FIELDS);
	g_return_if_fail (field >= 0);
	g_object_freeze_notify (G_OBJECT (gauge));

	MtxColorRange *c_range = NULL;
	c_range = g_array_index(gauge->c_ranges,MtxColorRange *,index);
	g_return_if_fail (c_range != NULL);
	switch (field)
	{
		case CR_LOWPOINT:
			c_range->lowpoint = *(gfloat *)value;
			break;
		case CR_HIGHPOINT:
			c_range->highpoint = *(gfloat *)value;
			break;
		case CR_INSET:
			c_range->inset = *(gfloat *)value;
			break;
		case CR_LWIDTH:
			c_range->lwidth = *(gfloat *)value;
			break;
		case CR_COLOR:
			c_range->color = *(GdkColor *)value;
			break;
		default:
			break;
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;
}


/*!
 \brief clears all color ranges from the gauge
 \param gauge (MtxGaugeFace *), pointer to gauge object
 */
void mtx_gauge_face_remove_all_color_ranges(MtxGaugeFace *gauge)
{
	gint i = 0;
	MtxColorRange *c_range = NULL;
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	for (i=gauge->c_ranges->len-1;i>=0;i--)
	{
		c_range = g_array_index(gauge->c_ranges,MtxColorRange *, i);
		gauge->c_ranges = g_array_remove_index(gauge->c_ranges,i);
		g_free(c_range);
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;
}


/*!
 \brief clears all text blocks from the gauge
 \param gauge (MtxGaugeFace *), pointer to gauge object
 */
void mtx_gauge_face_remove_all_text_blocks(MtxGaugeFace *gauge)
{
	gint i = 0;
	MtxTextBlock *tblock = NULL;
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	for (i=gauge->t_blocks->len-1;i>=0;i--)
	{
		tblock = g_array_index(gauge->t_blocks,MtxTextBlock *, i);
		gauge->t_blocks = g_array_remove_index(gauge->t_blocks,i);
		g_free(tblock->font);
		g_free(tblock->text);
		g_free(tblock);
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;
}



/*!
 \brief clears all tick groups from the gauge
 \param gauge (MtxGaugeFace *), pointer to gauge object
 */
void mtx_gauge_face_remove_all_tick_groups(MtxGaugeFace *gauge)
{
	gint i = 0;
	MtxTextBlock *tgroup = NULL;
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	for (i=gauge->tick_groups->len-1;i>=0;i--)
	{
		tgroup = g_array_index(gauge->tick_groups,MtxTextBlock *, i);
		gauge->tick_groups = g_array_remove_index(gauge->tick_groups,i);
		g_free(tgroup->font);
		g_free(tgroup->text);
		g_free(tgroup);
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;
}


/*!
 \brief clears all polygons from the gauge
 \param gauge (MtxGaugeFace *), pointer to gauge object
 */
void mtx_gauge_face_remove_all_polygons(MtxGaugeFace *gauge)
{
	gint i = 0;
	MtxPolygon *poly = NULL;
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	for (i=gauge->polygons->len-1;i>=0;i--)
	{
		poly = g_array_index(gauge->polygons,MtxPolygon *, i);
		gauge->polygons = g_array_remove_index(gauge->polygons,i);
		if (poly->type == MTX_GENPOLY)
		{
			if (((MtxGenPoly *)poly->data)->points)
				g_free(((MtxGenPoly *)poly->data)->points);
		}
		g_free(poly->data);
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;
}


/*!
 \brief clears a specific color range based on index passed
 \param gauge (MtxGaugeFace *), pointer to gauge object
 \param index gint index of the one we want to remove.
 */
void mtx_gauge_face_remove_color_range(MtxGaugeFace *gauge, gint index)
{
	MtxColorRange *c_range = NULL;
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	if ((index <= gauge->c_ranges->len) && (index >= 0 ))
	{
		c_range = g_array_index(gauge->c_ranges,MtxColorRange *, index);
		g_array_remove_index(gauge->c_ranges,index);
		if (c_range)
			g_free(c_range);
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;

}


/*!
 \brief clears a specific text_block based on index passed
 \param gauge (MtxGaugeFace *), pointer to gauge object
 \param index gint index of the one we want to remove.
 */
void mtx_gauge_face_remove_text_block(MtxGaugeFace *gauge, gint index)
{
	MtxTextBlock *tblock = NULL;
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	if ((index <= gauge->t_blocks->len) && (index >= 0 ))
	{
		tblock = g_array_index(gauge->t_blocks,MtxTextBlock *, index);
		g_array_remove_index(gauge->t_blocks,index);
		if (tblock)
		{
			g_free(tblock->font);
			g_free(tblock->text);
			g_free(tblock);
		}
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;

}


/*!
 \brief clears a specific tick_group based on index passed
 \param gauge (MtxGaugeFace *), pointer to gauge object
 \param index gint index of the one we want to remove.
 */
void mtx_gauge_face_remove_tick_group(MtxGaugeFace *gauge, gint index)
{
	MtxTextBlock *tgroup = NULL;
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	if ((index <= gauge->tick_groups->len) && (index >= 0 ))
	{
		tgroup = g_array_index(gauge->tick_groups,MtxTextBlock *, index);
		g_array_remove_index(gauge->tick_groups,index);
		if (tgroup)
		{
			g_free(tgroup->font);
			g_free(tgroup->text);
			g_free(tgroup);
		}
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;

}


/*!
 \brief clears a specific polygon based on index passed
 \param gauge (MtxGaugeFace *), pointer to gauge object
 \param index gint index of the one we want to remove.
 */
void mtx_gauge_face_remove_polygon(MtxGaugeFace *gauge, gint index)
{
	MtxPolygon *poly = NULL;
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	if ((index <= gauge->polygons->len) && (index >= 0 ))
	{
		poly = g_array_index(gauge->polygons,MtxPolygon *, index);
		g_array_remove_index(gauge->polygons,index);
		if (poly)
		{
			if (poly->type == MTX_GENPOLY)
			{
				if (((MtxGenPoly *)poly->data)->points)
					g_free(((MtxGenPoly *)poly->data)->points);
			}
			g_free(poly);
		}
	}
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	return;

}


/*!
 \brief updates the gauge position,  This is a wrapper function conditionally
 compiled to call a corresponsing GDK or cairo function.
 \param widget (GtkWidget *) pointer to the gauge object
 */
void update_gauge_position(GtkWidget *widget)
{
#ifdef HAVE_CAIRO
	cairo_update_gauge_position (widget);
#else
	gdk_update_gauge_position (widget);
#endif
}



/*!
 \brief generates the gauge background, This is a wrapper function 
 conditionally compiled to call a corresponsing GDK or cairo function.
 \param widget (GtkWidget *) pointer to the gauge object
 */
void generate_gauge_background(GtkWidget *widget)
{
#ifdef HAVE_CAIRO
	cairo_generate_gauge_background(widget);
#else
	gdk_generate_gauge_background(widget);
#endif
}


/*!
 \brief gets called when  a user wants a new gauge
 \returns a pointer to a newly created gauge widget
 */
GtkWidget *mtx_gauge_face_new ()
{
	return GTK_WIDGET (g_object_new (MTX_TYPE_GAUGE_FACE, NULL));
}


/*!
 \brief enables showing the drag border boxes 
 \param gauge, pointer to gauge
 \param state, state to set it to
 */
void mtx_gauge_face_set_show_drag_border(MtxGaugeFace *gauge, gboolean state)
{
	g_return_if_fail (MTX_IS_GAUGE_FACE (gauge));
	g_object_freeze_notify (G_OBJECT (gauge));
	gauge->show_drag_border = state;
	g_object_thaw_notify (G_OBJECT (gauge));
	generate_gauge_background(GTK_WIDGET(gauge));
	mtx_gauge_face_redraw_canvas (gauge);
	mtx_gauge_face_configure(GTK_WIDGET(gauge),NULL);
	gdk_window_clear_area_e(GTK_WIDGET(gauge)->window,0,0,gauge->w, gauge->h);
}


/*!
 \brief gets the state of the drag border
 \param gauge, pointer to gauge
 \returns the state of whether the gauge shows the drag border
 */
gboolean mtx_gauge_face_get_show_drag_border(MtxGaugeFace *gauge)
{
	g_return_val_if_fail (MTX_IS_GAUGE_FACE (gauge), FALSE);
	return gauge->show_drag_border;
}


/*!
 \brief gets called to redraw the entire display manually
 \param gauge (MtxGaugeFace *) pointer to the gauge object
 */
void mtx_gauge_face_redraw_canvas (MtxGaugeFace *gauge)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (gauge);

	if (!widget->window) return;

	update_gauge_position(widget);
	gdk_window_clear(widget->window);
}


