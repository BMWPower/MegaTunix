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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <defines.h>
#include <protos.h>
#include <globals.h>
#include <structures.h>
#include <runtime_gui.h>


struct v1_2_Runtime_Gui runtime_data;
extern struct Counts counts;
const gchar *status_msgs[] = {	"CONNECTED","CRANKING","RUNNING","WARMUP",
				"AS_ENRICH","ACCEL","DECEL"};
gboolean force_status_update = TRUE;
extern gboolean connected;
extern gboolean fahrenheit;
extern gboolean forced_update;
extern GdkColor white;
extern GdkColor black;
extern struct Labels labels;
gfloat ego_pbar_divisor = 5.0;	/* Initially assume a Wideband Sensor */
gfloat map_pbar_divisor = 255.0;/* Initially assume a Turbo MAP Sensor */

int build_runtime(GtkWidget *parent_frame)
{
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *label;
	GtkWidget *table;
	GtkWidget *pbar;
	GtkWidget *button;
	GtkWidget *entry;
	GtkWidget *ebox;
	extern GtkTooltips *tip;
	gint i=0;

	vbox = gtk_vbox_new(FALSE,0);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_container_add(GTK_CONTAINER(parent_frame),vbox);

	frame = gtk_frame_new("Real-Time Variables");
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);

	hbox = gtk_hbox_new(TRUE,0);
	gtk_container_add(GTK_CONTAINER(frame),hbox);

	table = gtk_table_new(6,3,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),5);
        gtk_table_set_col_spacings(GTK_TABLE(table),5);
        gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_box_pack_start(GTK_BOX(hbox),table,TRUE,TRUE,0);

	/* Seconds Counter Label */
	label = gtk_label_new("Seconds");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	/* Seconds Counter from MS */
	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.secl_lab = label;

	/* O2 Voltage Label*/
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),"O<sub>2</sub> (Volts)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	/* O2 Voltage value*/
	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.ego_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 1, 2,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.ego_pbar = pbar;

	/* Coolant */
	label = gtk_label_new("Coolant (F)");
	labels.runtime_clt_lab = label;
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 2, 3,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.clt_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 2, 3,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.clt_pbar = pbar;
	
	
	/* Battery Voltage */
	label = gtk_label_new("Batt (Volts)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 3, 4,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.batt_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);

        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 3, 4,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.batt_pbar = pbar;
	

	/* TPS */
	label = gtk_label_new("TPS (%)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 4, 5,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.tps_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 4, 5,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.tps_pbar = pbar;
	
	
	/* GammaE (sum of enrichments) */
	label = gtk_label_new("GammaE (%)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 5, 6,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.gammae_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 5, 6,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.gammae_pbar = pbar;
	
	table = gtk_table_new(6,3,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),5);
        gtk_table_set_col_spacings(GTK_TABLE(table),5);
        gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_box_pack_start(GTK_BOX(hbox),table,TRUE,TRUE,0);

	/* NULL label to make spacing line up... */
	label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
	
	/* MAP Sensor */	
	label = gtk_label_new("MAP (kPa)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.map_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 1, 2,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.map_pbar = pbar;
	
	/* Manifold Air Temp */
	label = gtk_label_new("MAT (F)");
	labels.runtime_mat_lab = label;
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 2, 3,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.mat_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 2, 3,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.mat_pbar = pbar;
	
	/* RPM */
	label = gtk_label_new("RPM");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 3, 4,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.rpm_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 3, 4,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.rpm_pbar = pbar;

	/* PW */
	label = gtk_label_new("PW (ms)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 4, 5,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.pw_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 4, 5,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.pw_pbar = pbar;

	/* Duty Cycle */
	label = gtk_label_new("Duty Cycle (%)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 5, 6,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.dcycle_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 5, 6,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.dcycle_pbar = pbar;
	
	/* Corrections/Enrichments frame */

	frame = gtk_frame_new("Corrections/Enrichments (Percent)");
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);

	hbox = gtk_hbox_new(TRUE,0);
	gtk_container_add(GTK_CONTAINER(frame),hbox);

	table = gtk_table_new(3,3,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),5);
        gtk_table_set_col_spacings(GTK_TABLE(table),5);
        gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_box_pack_start(GTK_BOX(hbox),table,TRUE,TRUE,0);

	/* EGOcorr  */
	label = gtk_label_new("EGO (%)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.egocorr_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 0, 1,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.egocorr_pbar = pbar;
	
	
	/* Baro Correction  */
	label = gtk_label_new("Baro (%)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.barocorr_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 1, 2,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.barocorr_pbar = pbar;
	
	
	/* Warmup Correction  */
	label = gtk_label_new("Warmup (%)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 2, 3,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.warmcorr_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 2, 3,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.warmcorr_pbar = pbar;
	
	table = gtk_table_new(3,3,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),5);
        gtk_table_set_col_spacings(GTK_TABLE(table),5);
        gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_box_pack_start(GTK_BOX(hbox),table,TRUE,TRUE,0);

	/* Air Den Cycle */
	label = gtk_label_new("Air Density (%)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.aircorr_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 0, 1,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.aircorr_pbar = pbar;
	
	/* Ve  */
	label = gtk_label_new("VE (%)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.vecurr_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 1, 2,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.vecurr_pbar = pbar;
	
	/* Accel Boost  */
	label = gtk_label_new("Accel (ms)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, 2, 3,
                        (GtkAttachOptions) (GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.tpsaccel_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
        gtk_table_attach (GTK_TABLE (table), pbar, 2, 3, 2, 3,
                        (GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
                        (GtkAttachOptions) (0), 0, 0);
	runtime_data.tpsaccel_pbar = pbar;

	ebox = gtk_event_box_new();
        gtk_box_pack_start(GTK_BOX(vbox),ebox,FALSE,TRUE,0);
        gtk_tooltips_set_tip(tip,ebox,
        "This block shows you statistics on the number of good reads of the VE/Constants datablocks, RealTime datablocks and the MegaSquirt hard reset and Serial I/O error counts.  Hard resets are indicative of power problems or excessive electrical noise to the MS (causing cpu resets).  Serial I/O errors are indicative of a poor cable connection between this host computer and the MS.",NULL);

	frame = gtk_frame_new("MegaSquirt I/O Status");
        gtk_container_add(GTK_CONTAINER(ebox),frame);

        hbox = gtk_hbox_new(TRUE,0);
        gtk_container_add(GTK_CONTAINER(frame),hbox);

        table = gtk_table_new(3,4,FALSE);
        gtk_table_set_row_spacings(GTK_TABLE(table),7);
        gtk_table_set_col_spacings(GTK_TABLE(table),5);
        gtk_container_set_border_width(GTK_CONTAINER(table),5);
        gtk_box_pack_start(GTK_BOX(hbox),table,FALSE,TRUE,20);

        label = gtk_label_new("Good VE/Constants Reads");
        gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

        entry = gtk_entry_new();
        counts.runtime_ve_readcount_entry = entry;
        gtk_entry_set_width_chars (GTK_ENTRY (entry), 8);
        gtk_widget_set_sensitive(entry,FALSE);
        gtk_widget_modify_text(entry,GTK_STATE_INSENSITIVE,&black);
        gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);


        label = gtk_label_new("Good RealTime Reads");
        gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
        gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (GTK_FILL), 0, 0);

        entry = gtk_entry_new();
        counts.runtime_readcount_entry = entry;
        gtk_entry_set_width_chars (GTK_ENTRY (entry), 8);
        gtk_widget_set_sensitive(entry,FALSE);
        gtk_widget_modify_text(entry,GTK_STATE_INSENSITIVE,&black);
        gtk_table_attach (GTK_TABLE (table), entry, 3, 4, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

        label = gtk_label_new("Hard Reset Count");
        gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (GTK_FILL), 0, 0);

        entry = gtk_entry_new();
        counts.runtime_reset_entry = entry;
        gtk_entry_set_width_chars (GTK_ENTRY (entry), 8);
        gtk_widget_set_sensitive(entry,FALSE);
        gtk_widget_modify_text(entry,GTK_STATE_INSENSITIVE,&black);
        gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

        label = gtk_label_new("Serial I/O Error Count");
        gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
        gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (GTK_FILL), 0, 0);

        entry = gtk_entry_new();
        counts.runtime_sioerr_entry = entry;
        gtk_entry_set_width_chars (GTK_ENTRY (entry), 8);
        gtk_widget_set_sensitive(entry,FALSE);
        gtk_widget_modify_text(entry,GTK_STATE_INSENSITIVE,&black);
        gtk_table_attach (GTK_TABLE (table), entry, 3, 4, 1, 2,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

	button = gtk_button_new_with_label("Reset Status Counters...");
	g_signal_connect(G_OBJECT (button), "clicked",
			G_CALLBACK (update_errcounts), \
			GINT_TO_POINTER(TRUE));
	gtk_table_attach (GTK_TABLE (table), button, 0, 4, 2, 3,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (GTK_FILL), 0, 0);

	frame = gtk_frame_new("Commands");
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);

	table = gtk_table_new(1,2,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),5);
        gtk_table_set_col_spacings(GTK_TABLE(table),5);
        gtk_container_set_border_width (GTK_CONTAINER (table), 10);
	gtk_container_add(GTK_CONTAINER(frame),table);

	button = gtk_button_new_with_label("Start Reading RT Vars");
        g_signal_connect(G_OBJECT (button), "clicked",
                        G_CALLBACK (std_button_handler), \
                        GINT_TO_POINTER(START_REALTIME));
        gtk_table_attach (GTK_TABLE (table), button, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

	button = gtk_button_new_with_label("Stop Reading RT vars");
        g_signal_connect(G_OBJECT (button), "clicked",
                        G_CALLBACK (std_button_handler), \
                        GINT_TO_POINTER(STOP_REALTIME));
        gtk_table_attach (GTK_TABLE (table), button, 3, 4, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

	frame = gtk_frame_new("Runtime Status");
        gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);

        vbox2 = gtk_vbox_new(FALSE,0);
        gtk_container_add(GTK_CONTAINER(frame),vbox2);
        gtk_container_set_border_width(GTK_CONTAINER(vbox2),5);
	hbox = gtk_hbox_new(TRUE,3);
	gtk_box_pack_start(GTK_BOX(vbox2),hbox,TRUE,TRUE,0);
	for (i=0;i<7;i++)
	{
		frame = gtk_frame_new(NULL);
		gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_IN);
		runtime_data.status[i] = gtk_label_new(status_msgs[i]);
		gtk_widget_set_sensitive(runtime_data.status[i],FALSE);
		gtk_container_add(GTK_CONTAINER(frame),runtime_data.status[i]);
		gtk_box_pack_start(GTK_BOX(hbox),frame,TRUE,TRUE,0);
	}
	
	return TRUE;
}

void update_runtime_vars()
{
	char buff[120];
	gfloat tmpf;
	gint tmpi;
	extern struct runtime_std *runtime;
	extern struct runtime_std *runtime_last;
	/* test to see if data changed 
	 * Why bother wasting CPU to update the GUI when 
	 * you'd just print the same damn thing?
	 * Makes the code a little uglier, but the gui won't
	 * flicker the text widgets at high update rates
	 */

	gdk_threads_enter();
	if (runtime->secl != runtime_last->secl)
	{
		g_snprintf(buff,10,"%i",runtime->secl);
		gtk_label_set_text(GTK_LABEL(runtime_data.secl_lab),buff);
	}
	if (runtime->ego != runtime_last->ego)
	{
		g_snprintf(buff,10,"%.2f",runtime->ego);
		gtk_label_set_text(GTK_LABEL(runtime_data.ego_lab),buff);
		tmpf = runtime->ego/ego_pbar_divisor <= 1.0 
				? runtime->ego/ego_pbar_divisor: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.ego_pbar),
				tmpf);
	}
	if (runtime->map != runtime_last->map)
	{
		g_snprintf(buff,10,"%i",runtime->map);
		gtk_label_set_text(GTK_LABEL(runtime_data.map_lab),buff);
		tmpf = runtime->map/map_pbar_divisor <= 1.0 
				? runtime->map/map_pbar_divisor: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.map_pbar),
				tmpf);
	}
	if ((runtime->clt != runtime_last->clt) || (forced_update))
	{
		if (fahrenheit)/* subtract 40 from MS value */
			tmpi = runtime->clt-40;
		else	/* subtract 40 and then to F->C conversion */
			tmpi = (runtime->clt-40-32)*(5.0/9.0);
	
		g_snprintf(buff,10,"%i",tmpi);
		gtk_label_set_text(GTK_LABEL(runtime_data.clt_lab),buff);
		tmpf = runtime->clt/255.0 <= 1.0 ? runtime->clt/255.0 : 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.clt_pbar),
				tmpf);
	}
	if (runtime->batt != runtime_last->batt)
	{
		g_snprintf(buff,10,"%.2f",runtime->batt);
		gtk_label_set_text(GTK_LABEL(runtime_data.batt_lab),buff);
		tmpf = runtime->batt/18.0 <= 1.0 ? runtime->batt/18.0: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.batt_pbar),
				tmpf);
	}
	if (runtime->gammae != runtime_last->gammae)
	{
		g_snprintf(buff,10,"%i",runtime->gammae);
		gtk_label_set_text(GTK_LABEL(runtime_data.gammae_lab),buff);
		tmpf = runtime->gammae/200.0 <= 1.0 ? runtime->gammae/200.0: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.gammae_pbar),
				tmpf);
	}
	if ((runtime->mat != runtime_last->mat) || (forced_update))
	{
		if (fahrenheit)	/* subtract 40 from MS value */
			tmpi = runtime->mat-40;
		else	/* subtract 40 and then to F->C conversion */
			tmpi = (runtime->mat-40-32)*(5.0/9.0);
			
		g_snprintf(buff,10,"%i",tmpi);
		gtk_label_set_text(GTK_LABEL(runtime_data.mat_lab),buff);
		tmpf = runtime->mat/255.0 <= 1.0 ? runtime->mat/255.0 : 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.mat_pbar),
				tmpf);
	}
	if (runtime->tps != runtime_last->tps)
	{
		g_snprintf(buff,10,"%i",runtime->tps);
		gtk_label_set_text(GTK_LABEL(runtime_data.tps_lab),buff);
		tmpf = runtime->tps/100.0 <= 1.0 ? runtime->tps/100.0: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.tps_pbar),
				tmpf);
	}
	if (runtime->rpm != runtime_last->rpm)
	{
		g_snprintf(buff,10,"%i",runtime->rpm);
		gtk_label_set_text(GTK_LABEL(runtime_data.rpm_lab),buff);
		tmpf = runtime->rpm/8000.0 <= 1.0 ? runtime->rpm/8000.0: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.rpm_pbar),
				tmpf);
	}
	if (runtime->pw != runtime_last->pw)
	{
		g_snprintf(buff,10,"%.1f",runtime->pw);
		gtk_label_set_text(GTK_LABEL(runtime_data.pw_lab),buff);
		tmpf = runtime->pw/25.5 <= 1.0 ? runtime->pw/25.5: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.pw_pbar),
				tmpf);
	}
	if (runtime->egocorr != runtime_last->egocorr)
	{
		g_snprintf(buff,10,"%i",runtime->egocorr);
		gtk_label_set_text(GTK_LABEL(runtime_data.egocorr_lab),buff);
		tmpf = runtime->egocorr/200.0 <= 1.0 ? runtime->egocorr/200.0: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.egocorr_pbar),
				tmpf);
	}
	if (runtime->barocorr != runtime_last->barocorr)
	{
		g_snprintf(buff,10,"%i",runtime->barocorr);
		gtk_label_set_text(GTK_LABEL(runtime_data.barocorr_lab),buff);
		tmpf = runtime->barocorr/200.0 <= 1.0 ? runtime->barocorr/200.0: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.barocorr_pbar),
				tmpf);
	}
	if (runtime->warmcorr != runtime_last->warmcorr)
	{
		g_snprintf(buff,10,"%i",runtime->warmcorr);
		gtk_label_set_text(GTK_LABEL(runtime_data.warmcorr_lab),buff);
		tmpf = runtime->warmcorr/200.0 <= 1.0 ? runtime->warmcorr/200.0: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.warmcorr_pbar),
				tmpf);
	}
	if (runtime->aircorr != runtime_last->aircorr)
	{
		g_snprintf(buff,10,"%i",runtime->aircorr);
		gtk_label_set_text(GTK_LABEL(runtime_data.aircorr_lab),buff);
		tmpf = runtime->aircorr/200.0 <= 1.0 ? runtime->aircorr/200.0: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.aircorr_pbar),
				tmpf);
	}
	if (runtime->vecurr != runtime_last->vecurr)
	{
		g_snprintf(buff,10,"%i",runtime->vecurr);
		gtk_label_set_text(GTK_LABEL(runtime_data.vecurr_lab),buff);
		tmpf = runtime->vecurr/200.0 <= 1.0 ? runtime->vecurr/200.0: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.vecurr_pbar),
				tmpf);
	}
	if (runtime->tpsaccel != runtime_last->tpsaccel)
	{
		g_snprintf(buff,10,"%.1f",runtime->tpsaccel/10.0);
		gtk_label_set_text(GTK_LABEL(runtime_data.tpsaccel_lab),buff);
		tmpf = (runtime->tpsaccel/10.0)/25.5 <= 1.0 
				? (runtime->tpsaccel/10.0)/25.5: 1.0;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.tpsaccel_pbar),
				tmpf);
	}
	if (runtime->dcycle != runtime_last->dcycle)
	{
		g_snprintf(buff,10,"%.1f",runtime->dcycle);
		tmpf = runtime->dcycle/100.0 <= 1.0 ? runtime->dcycle/100.0: 1.0;
		gtk_label_set_text(GTK_LABEL(runtime_data.dcycle_lab),buff);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
				(runtime_data.dcycle_pbar),
				tmpf);
	}

	/* Connected */
	gtk_widget_set_sensitive(runtime_data.status[CONNECTED],
			connected);
		
	if ((force_status_update) || (runtime->engine.value != runtime_last->engine.value))
	{
		force_status_update = FALSE;
		/* Cranking */
		gtk_widget_set_sensitive(runtime_data.status[CRANKING],
				runtime->engine.bit.crank);
		/* Running */
		gtk_widget_set_sensitive(runtime_data.status[RUNNING],
				runtime->engine.bit.running);
		/* Warmup */
		gtk_widget_set_sensitive(runtime_data.status[WARMUP],
				runtime->engine.bit.warmup);
		/* Afterstart Enrichment */
		gtk_widget_set_sensitive(runtime_data.status[AS_ENRICH],
				runtime->engine.bit.startw);
		/* Accel Enrichment */
		gtk_widget_set_sensitive(runtime_data.status[ACCEL],
				runtime->engine.bit.tpsaen);
		/* Decel Enleanment */
		gtk_widget_set_sensitive(runtime_data.status[DECEL],
				runtime->engine.bit.tpsden);
		
	}
	if (forced_update)
		forced_update = FALSE;
	gdk_threads_leave();
}

void reset_runtime_status()
{
	gint i;
	for(i=1;i<7;i++)
	{
		gtk_widget_set_sensitive(runtime_data.status[i],
				FALSE);
	}
}
	
