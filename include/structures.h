/*!
  \author David Andruczyk

 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute, etc. this as long as all the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

/* Constants/Enrichments Gui Adjustment Structures */

#ifndef __STRUCTURES_H__
#define __STRUCTURES_H__

#include <configfile.h>
#include <defines.h>
#include <enums.h>
#include <gtk/gtk.h>
#include <termios.h>
#include <unistd.h>


/*! 
 \brief Serial_Params holds all variables related to the state of the serial
 port being used by MegaTunix
 */
struct Serial_Params
{
        gint fd;		/*! File descriptor */
        gchar *port_name;	/*! textual name of comm port */
        gboolean open;		/*! flag, TRUE for open FALSE for closed */
        gint read_wait;		/*! time delay between each read */
        gint errcount;		/*! Serial I/O errors read error count */
        struct termios oldtio;	/*! serial port settings before we touch it */
        struct termios newtio;	/*! serial port settings we use when running */
};


/*! 
 \brief Firmware_Details stores all attributes about the firmware being used
 after detection (\see interrogate_ecu ) including lists of tabs to be loaded
 by the glade loader (\see load_gui_tabs ), the configuration for the realtime
 variables map (\see rtv_map_loader) and the sliderss_map_file (\see 
 load_runtime_sliders )
 */
struct Firmware_Details
{
	gchar *name;		/*! textual name */
	gchar **tab_list;	/*! vector string of tabs to load */
	gchar **tab_confs;	/*! Tab configuration files */
	gchar *rtv_map_file;	/*! realtime vars map filename */
	gchar *sliders_map_file;/*! runtime sliders map filename */
	gchar *rtt_map_file;	/*! runtime text map filename */
	gchar *status_map_file;	/*! runtime status map filename */
	gchar *signature_str;	/*! ECU Signature String */
        gint rtvars_size;       /*! Size of Realtime vars datablock */
        gint ignvars_size;      /*! Size of Realtime vars datablock */
        gint memblock_size;     /*! Size of Raw_Memory datablock */
	gboolean multi_page;	/*! Multi-page firmware */
	gboolean chunk_support;	/*! Supports Chunk Write */
	gint total_pages;	/*! How many pages do we handle? */
	gint total_tables;	/*! How many tables do we handle? */
	gint ro_above;		/*! Read Only debug pages above this one */
	gint trigmon_page;	/*! Trigger Monitor RO Page */
	gint toothmon_page;	/*! Tooth Monitor RO Page */
	gchar *chunk_write_cmd;	/*! Command to send to chunk write data... */
	gchar *write_cmd;	/*! Command to send to write data... */
	gchar *burn_cmd;	/*! Command to send to burn data... */
	gchar *page_cmd;	/*! Command to send to change pages ... */
	struct Page_Params **page_params;/*! special vars per page */
	struct Table_Params **table_params;/*! details each table */
	struct Req_Fuel_Params **rf_params;/*! req_fuel params */
};


/*! 
 Controls for the Required Fuel Calculator... 
 \brief The Req_Fuel struct contains jsut about all widgets for the rewuired
 fuel popup.  most of the values are loaded/saved from the main config file
 when used.
 */
struct Reqd_Fuel
{
	GtkWidget *popup;		/*! the popup window */
	GtkWidget *calcd_val_spin;	/*! Preliminary value */
	GtkWidget *reqd_fuel_spin;	/*! Used value */
	gfloat calcd_reqd_fuel;		/*! calculated value... */
        gint disp;			/*! Engine size  1-1000 Cu-in */
        gint cyls;			/*! # of Cylinders  1-12 */
        gfloat rated_inj_flow;		/*! Rated injector flow */
        gfloat rated_pressure;		/*! Rated fuel pressure */
        gfloat actual_pressure;		/*! Actual fuel pressure */
        gfloat actual_inj_flow;		/*! injector flow rate (lbs/hr) */
        gfloat target_afr;		/*! Air fuel ratio 10-25.5 */
        gint page;			/*! Which page is this for */
	gint table_num;			/*! Which table this refers to */
	gboolean visible;		/*! Is it visible? */
};


/*!
 \brief Io_File is the datastructure used for file I/O in the inport/export
 routines.
 \see vetable_import
 \see vetable_export
 */
struct Io_File
{
	GIOChannel *iochannel;
	gchar *filename;
	FileIoType iotype;
};


/*!
 \brief Viewable_Value is the datastructure bound 
 to every trace viewed in the logviewer. 
 */
struct Viewable_Value
{
	GdkGC *font_gc;			/*! GC used for the fonts */
	GdkGC *trace_gc;		/*! GC used for the trace */
	PangoRectangle *log_rect;	/*! Logcial rectangle around text */
	PangoRectangle *ink_rect;	/*! Ink rectangle around text */
	GObject *object;		/*! object */
	gchar *vname;			/*! Name of widget being logged */
	gboolean is_float;		/*! TRUE or FALSE */
	gboolean force_update;		/*! flag to force update on addition */
	gboolean highlight;		/*! flag it highlight it.. */
	gint last_y;			/*! Last point on screen of trace */
	gint last_index;		/*! latest entryu into data array */
	gchar *data_source;		/*! Textual name of source */
	gfloat min;			/*! for auto-scaling */
	gfloat max;			/*! for auto-scaling */
	gfloat lower;			/*! hard limits to use for scaling */
	gfloat upper;			/*! hard limits to use for scaling */
	gfloat cur_low;			/*! User limits to use for scaling */
	gfloat cur_high;		/*! User limits to use for scaling */
	GArray *data_array;		/*! History of all values recorded */
	struct Log_Info *log_info;	/*! important */
};
	

struct Dash_Gauge
{
	GObject *object;		/* Data stroage object for RT vars */
	gchar * source;			/* Source name (unused) */
	GtkWidget *gauge;		/* pointer to gaugeitself */
	GtkWidget *dash;		/* pointer to gauge parent */
};


/*! 
 \brief The Rt_Slider struct contains info on the runtime display tab sliders
 as they are now stored in the config file and adjustable in position
 and placement and such..
 */
struct Rt_Slider
{
	gchar *ctrl_name;	/*! Ctrl name in config file (key in hash) */
	GtkWidget *parent;	/*! Parent of the table below  */
	GtkWidget *label;	/*! Label in runtime display */
	GtkWidget *textval;	/*! Label in runtime display */
	GtkWidget *pbar;	/*! progress bar for the data */
	gint table_num;		/*! Refers to the table number in the profile*/
	gint tbl;		/*! Table number (0-3) */
	gint row;		/*! Starting row */
	gchar *friendly_name;	/*! text for Label above */
	gint lower;		/*! Lower limit */
	gint upper;		/*! Upper limit */
	GArray *history;	/*! where the data is from */
	gfloat last_percentage;	/*! last percentage of on screen slider */
	GObject *object;	/*! object of obsession.... */
	gboolean enabled;	/*! Pretty obvious */
	gint count;		/*! used to making sure things update */
	gint rate;		/*! used to making sure things update */
	gint last_upd;		/*! used to making sure things update */
	SliderType class;	/*! Slider type... */
};


/*! 
 \brief The Rt_Text struct contains info on the floating runtime var text 
 display
 */
struct Rt_Text
{
	gchar *ctrl_name;	/*! Ctrl name in config file (key in hash) */
	GtkWidget *parent;	/*! Parent of the table below  */
	GtkWidget *name_label;	/*! Label in runtime display */
	GtkWidget *textval;	/*! Label in runtime display */
	gchar *friendly_name;	/*! text for Label above */
	GArray *history;	/*! where the data is from */
	GObject *object;	/*! object of obsession.... */
	gint count;		/*! used to making sure things update */
	gint rate;		/*! used to making sure things update */
	gint last_upd;		/*! used to making sure things update */
};


/*! 
 \brief The Log_Info datastructure is populated when a datalog file is opened
 for viewing in the Datalog viewer.
 */
struct Log_Info
{
	gint field_count;	/*! How many fields in the logfile */
	gchar *delimiter;	/*! delimiter between fields for this logfile */
	GArray *log_list;	/*! List of objects */
};


/*! 
 \brief The Page_Params structure contains fields defining the size of the 
 page returned from the ECU, the VEtable, RPm and Load table base offsets and
 sizes, along with a flag signifying if it's a spark table
 */
struct Page_Params
{
	gint length;		/*! How big this page is... */
	gint truepgnum;		/*! True pagenumber to send */
	gint is_spark;		/*! does this require alt write cmd? */
	gint spconfig_offset;	/*! Where spconfig value is located */
};


/*! 
 \brief The Table_Params structure contains fields defining table parameters
 One struct is allocated per table, and multiple tables per page are allowed
 */
struct Table_Params
{
	gboolean is_fuel;	/*! If true next 7 params must exist */
	gint dtmode_offset;	/*! DT mode offset (msns-e ONLY) */
	gint dtmode_page;	/*! DT mode page (msns-e ONLY) */
	gint cfg11_offset;	/*! Where config11 value is located */
	gint cfg12_offset;	/*! Where config12 value is located */
	gint cfg13_offset;	/*! Where config13 value is located */
	gint alternate_offset;	/*! Where alternate value is located */
	gint divider_offset;	/*! Where divider value is located */
	gint rpmk_offset;	/*! Where rpmk value is located */
	gint reqfuel_offset;	/*! Where reqfuel value is located */
	gint x_page;		/*! what page the rpm (X axis) resides in */
	gint x_base;		/*! where rpm table starts (X axis) */
	gint x_bincount;	/*! how many RPM bins (X axis) */
	gboolean x_multi_source;/*! uses multiple keyed sources? */
	gchar *x_source_key;	/*! this is hte key to sources_hash to 
				 *  get the search key for x_multi_hash
				 */
	gchar *x_multi_expr_keys;/*! keys to x_multi_hash */
	gchar *x_sources;	/*! comma sep list of sources */
	gchar *x_suffixes;	/*! comma sep list of suffixes */
	gchar *x_conv_exprs;	/*! comma sep list of x conv. expressions */
	gchar *x_precisions;	/*! comma sep list of precisions */
	GHashTable *x_multi_hash;/*! Hash table to store the above */
	gchar *x_source;	/*! X datasource for 3d displays */
	gchar *x_suffix;	/*! text suffix used on 3D view */
	gchar *x_conv_expr;	/*! x conversion expression */
	void *x_eval;		/*! evaluator for x variable */
	gint x_precision;	/*! how many decimal places */

	gint y_page;		/*! what page the load (Y axis) resides in */
	gint y_base;		/*! where load table starts  (Y Axis) */
	gint y_bincount;	/*! how many load bins (Y axis) */
	gboolean y_multi_source;/*! uses multiple keyed sources? */
	gchar *y_source_key;	/* text name of variable we find to determine
				 *  the correct key for y_multi_hash
				 */
	gchar *y_multi_expr_keys;/*! keys to x_multi_hash */
	gchar *y_sources;	/*! comma sep list of sources */
	gchar *y_suffixes;	/*! comma sep list of suffixes */
	gchar *y_conv_exprs;	/*! comma sep list of x conv. expressions */
	gchar *y_precisions;	/*! comma sep list of precisions */
	GHashTable *y_multi_hash;/*! Hash table to store the above */
	gchar *y_source;	/*! Y datasource for 3d displays */
	gchar *y_suffix;	/*! text suffix used on 3D view */
	gchar *y_conv_expr;	/*! y conversion expression */
	void *y_eval;		/*! evaluator for y variable */
	gint y_precision;	/*! how many decimal places */

	gint z_page;		/*! what page the vetable resides in */
	gint z_base;		/*! where the vetable starts */
	gboolean z_multi_source;/*! uses multiple keyed sources? */
	gchar *z_source_key;	/* text name of variable we find to determine
				 * the correct key for z_multi_hash
				 */
	gchar *z_multi_expr_keys;/*! keys to x_multi_hash */
	gchar *z_sources;	/*! comma sep list of sources */
	gchar *z_suffixes;	/*! comma sep list of suffixes */
	gchar *z_conv_exprs;	/*! comma sep list of x conv. expressions */
	gchar *z_precisions;	/*! comma sep list of precisions */
	GHashTable *z_multi_hash;/*! Hash table to store the above */
	gchar *z_source;	/*! Z datasource for 3d displays */
	gchar *z_suffix;	/*! text suffix used on 3D view */
	gchar *z_conv_expr;	/*! z conversion expression */
	void *z_eval;		/*! evaluator for z variable */
	gint z_precision;	/*! how many decimal places */
	gchar *table_name;	/*! Name for the 3D Table editor title */
};


/*! 
 \brief The Canidate structure is used for interrogation, each potential
 firmware is interrogated and as it is the data is fed into this structure
 for comparison against know values (interrogation profiles), upon a match
 the needed data is copied into a Firmware_Details structure
 \see Firmware_Details
 */
struct Canidate
{
	gchar *name;		/*! Name of this firmware */
	gchar *filename;	/*! absolute path to this canidate profile */
	GHashTable *bytecounts;	/*! byte count for each of the 10 test cmds */
	gchar *sig_str;		/*! Signature string to search for */
	gchar *text_version_str;/*! Ext Version string to search for */
	gint ver_num;		/*! Version number to search for */
	gchar *load_tabs;	/*! list of tabs to load into the gui */
	gchar *tab_confs;	/*! Tab configuration files */
	gchar *rtv_map_file;	/*! name of realtime vars map file */
	gchar *sliders_map_file;/*! runtime sliders map filename */
	gchar *rtt_map_file;	/*! runtime text map filename */
	gchar *status_map_file;	/*! runtime status map filename */
	Capability capabilities;/*! Bitmask of capabilities.... */
	gchar *rt_cmd_key;	/*! string key to hashtable for RT command */
	gchar *ve_cmd_key;	/*! string key to hashtable for VE command */
	gchar *ign_cmd_key;	/*! string key to hashtable for Ign command */
	gchar *raw_mem_cmd_key;	/*! string key to hashtable for RAW command */
	gchar *chunk_write_cmd;	/*! Command to send to write data... */
	gchar *write_cmd;	/*! Command to send to write data... */
	gchar *burn_cmd;	/*! Command to send to burn data... */
	gchar *page_cmd;	/*! Command to send to change pages... */
	gboolean multi_page;	/*! Multi-page firmware */
	gboolean chunk_support;	/*! Chunk Write supported firmware */
	gint ro_above;		/*! Debug pages above this one */
	gint trigmon_page;	/*! Trigger Monitor RO Page */
	gint toothmon_page;	/*! Tooth Monitor RO Page */
	gint total_pages;	/*! how many pages do we handle? */
	gint total_tables;	/*! how many tables do we handle? */
	GHashTable *lookuptables;/*! Lookuptables hashtable... */
	struct Page_Params **page_params;/*! special vars per page */
	struct Table_Params **table_params;/*! details on ve/rpm/load tables*/
};


/*!
 \brief The Req_Fuel_Params structure is used to store the current and last
 values of the interdependant required fuel parameters for the function
 that verifies req_fuel status and downloads it.  There is one structure
 allocated PER Table (even if the table's aren't fuel..)
 */
struct Req_Fuel_Params
{
	gint num_squirts;
	gint num_cyls;
	gint num_inj;
	gint divider;
	gint alternate;
	gint last_num_squirts;
	gint last_num_cyls;
	gint last_num_inj;
	gint last_divider;
	gint last_alternate;
	gfloat req_fuel_total;
	gfloat last_req_fuel_total;
};


/*!
 \brief the Command struct is used to store details on the commands that
 are valid for the ECU, they are loaded from a config file "tests" normally
 installed in /usr/local/share/MegaTunix/Interrogator/tests. There will be
 one Command struct created per command, and they are used to interrogate the
 target ECU.
 */
struct Command
{
	gchar *string;		/*! command to get the data */
	gchar *desc;		/*! command description */
	gchar *key;		/*! key into cmd_details hashtable */
	gint len;		/*! Command length in chars to send */
	gboolean multipart;	/*! Multipart command? (raw_memory) */
	gint cmd_int_arg;	/*! multipart arg, integer */
	gboolean store_data;	/*! Store returned data ? */
	StoreType store_type;	/*! Store data where */
};


/*!
 \brief Io_Message structure is used for passing data around in threads.c for
 kicking off commands to send data to/from the ECU or run specified handlers.
 messages and postfunctiosn can be bound into this strucutre to do some complex
 things with a simple subcommand.
 \see Io_Cmds
 */
struct Io_Message
{
	Io_Command cmd;		/*! Source command (initiator)*/
	CmdType command;	/*! Command type */
	gchar *out_str;		/*! Data sent to the ECU  for READ_CMD's */
	gint page;		/*! Virtual Page number */
	gint truepgnum;		/*! Physical page number */
	gint out_len;		/*! number of bytes in out_str */
	gint offset;		/*! used for RAW_MEMORY and more */
	GArray *funcs;		/*! List of functiosn to be dispatched... */
	InputHandler handler;	/*! Command handler for inbound data */
	void *payload;		/*! data passed along, arbritrary size.. */
	gboolean need_page_change; /*! flag to set if we need to change page */
};


/*
 \brief Text_Message strcture is used for a thread to pass messages up
 a GAsyncQueue to the main gui thread for updating a textview in a thread
 safe manner. A dispatch queue runs 5 times per second checking for messages
 to dispatch...
 */
struct Text_Message
{
	gchar *view_name;	/*! Textview name */
	gchar *tagname;		/*! Texttag to use */
	gchar *msg;		/*! message to display */
	gboolean count;		/*! display a counter */
	gboolean clear;		/*! Clear the window? */
};


/*
 \brief QFunction strcture is used for a thread to pass messages up
 a GAsyncQueue to the main gui thread for running any arbritrary function
 by name.
 */
struct QFunction
{
	gchar *func_name;	/*! Function Name */
	gint  dummy;		/*! filler for more shit later.. */
};


/*
 \brief Widget_Update strcture is used for a thread to pass a widget update
 call up a GAsyncQueue to the main gui thread for updating a widget in 
 a thread safe manner. A dispatch queue runs 5 times per second checking 
 for messages to dispatch...
 */
struct Widget_Update
{
	gchar *widget_name;	/*! Widget name */
	WidgetType type;	/*! what type of widget are we updating */
	gchar *msg;		/*! message to display */
};


/*!
 \brief Io_Cmds stores the basic data for the critical megasquirt command
 codes. (realtime, VE, ign and spark) including the length of each of those
 commands.  
 \warning This really should be done a better way...
 */
struct Io_Cmds
{
	gchar *realtime_cmd;	/*! Command sent to get RT vars.... */
	gint rt_cmd_len;	/*! length in bytes of rt_cmd_len */
	gchar *veconst_cmd;	/*! Command sent to get VE/Const vars.... */
	gint ve_cmd_len;	/*! length in bytes of veconst_cmd */
	gchar *ignition_cmd;	/*! Command sent to get Ignition vars.... */
	gint ign_cmd_len;	/*! length in bytes of ignition_cmd */
	gchar *raw_mem_cmd;	/*! Command sent to get raw_mem vars.... */
	gint raw_mem_cmd_len;	/*! length in bytes of raw_mem_cmd */
};


/*! 
 \brief Output_Data A simple wrapper struct to pass data to the output 
 function which makes the function a lot simpler.
 */
struct Output_Data
{
	gint page;		/*! Page in ECU */
	gint offset;		/*! Offset in block */
	gint value;		/*! Value to send */
	gint len;		/*! Length of chunk write block */
	guchar *data;		/*! Block of data for chunk write */
	gboolean ign_parm;	/*! Ignition parameter, True or False */
	gboolean queue_update;	/*! If true queues a member widget update */
	WriteMode mode;		/*! Write mode enum */
};


/*! 
 \brief RtvMap is the RealTime Variables Map structure, containing fields to
 access the realtime derived data via a hashtable, and via raw index. Stores
 timestamps of each incoming data byte for advanced future use.
 */
struct Rtv_Map
{
	gint raw_total;		/*! Number of raw variables */
	gint derived_total;	/*! Number of derived variables */
	gchar **raw_list;	/*! Char List of raw variables by name */
	GArray *rtv_array;	/*! Realtime Values array of lists.. */
	GArray *ts_array;	/*! Timestamp array */
	GArray *rtv_list;	/*! List of derived vars IN ORDER */
	GHashTable *rtv_hash;	/*! Hashtable of rtv derived values indexed by
				 * it's internal name */
};


/*! 
 \brief DebugLevel stores the debugging name, handler, class (bitmask) and 
 shift (forgot why this is here) and a enable/disable flag. Used to make the
 debugging core a little more configurable
 */
struct DebugLevel
{
	gchar * name;		/*! Debugging name */
	gint	handler;	/*! Signal handler name */
	Dbg_Class dclass;	/*! Bit mask for this level (0-31) */
	Dbg_Shift dshift;	/*! Bit shift amount */
	gboolean enabled;	/*! Enabled or not? */
};


/*!
 \brief Group holds common settings from groups of control as defined in a 
 datamap file.  This should reduce redundancy and significantly shrink the 
 datamap files.
 */
struct Group
{
	gchar **keys;		/*! String array for key names */
	gint *keytypes;		/*! Int array of key types... */
	GObject *object;	/*! To hold the data cleanly */
	gint num_keys;		/* How many keys we hold */
	gint num_keytypes;	/* How many keytypes we hold */
	gint page;		/* page of this group of data */
};


/*!
 \brief BindGroup is a small container used to pass multiple params into
 a function that is limited to a certain number of arguments...
 */
struct BindGroup
{
	ConfigFile *cfgfile;	/*! where the configfile ptr goes... */
	GHashTable *groups;	/*! where the groups table goes */
};


/*!
 \brief the Ve_View_3D structure contains all the field to create and 
 manipulate a 3D view of a MegaSquirt VE/Spark table, and should work in
 theory for any sized table
 */
struct Ve_View_3D
{
	gint beginX;
	gint beginY;
	gint active_y;
	gint active_x;
	gfloat dt;
	gfloat sphi;
	gfloat stheta;
	gfloat sdepth;
	gfloat zNear;
	gfloat zFar;
	gfloat aspect;
	gfloat h_strafe;
	gfloat v_strafe;
	gfloat z_offset;
	gfloat x_trans;
	gfloat y_trans;
	gfloat z_trans;
	gfloat x_scale;
	gfloat y_scale;
	gfloat z_scale;
	gfloat x_div;
	gfloat y_div;
	gfloat z_div;
	gint x_precision;
	gint y_precision;
	gint z_precision;
	/* Simple sources*/
	gchar *x_source;
	gchar *x_suffix;
	gchar *x_conv_expr;
	void *x_eval;
	gchar *y_source;
	gchar *y_suffix;
	gchar *y_conv_expr;
	void *y_eval;
	gchar *z_source;
	gchar *z_suffix;
	gchar *z_conv_expr;
	void *z_eval;
	/* Multi-sources */
	gchar * x_source_key;
	gboolean x_multi_source;
	GHashTable *x_multi_hash;
	gchar * y_source_key;
	gboolean y_multi_source;
	GHashTable *y_multi_hash;
	gchar * z_source_key;
	gboolean z_multi_source;
	GHashTable *z_multi_hash;

	GtkWidget *drawing_area;
	GtkWidget *window;
	GtkWidget *burn_but;
	GObject *dep_obj;
	gint y_base;
	gint y_page;
	gint y_bincount;
	gint x_base;
	gint x_page;
	gint x_bincount;
	gint z_base;
	gint z_page;
	gchar *table_name;
	gint table_num;
};


/*!
 \brief The Vex_Import structure holds all fields (lots) needed to load and
 process a VEX (VEtabalt eXport file) and load it into megatunix.
 \see vetable_import
 \see vetable_export
 */
struct Vex_Import
{	
	gchar *version;		/* String */
	gchar *revision;	/* String */
	gchar *comment;		/* String */
	gchar *date;		/* String */
	gchar *time;		/* String */
	gint page;		/* Int */
	gint table;		/* Int */
	gint total_x_bins;	/* Int */
	gint *x_bins;		/* Int Array, dynamic */
	gint total_y_bins;	/* Int */
	gint *y_bins;		/* Int Array, dynamic */
	gint total_tbl_bins;	/* Int */
	gint *tbl_bins;		/* Int Array, dynamic */
	gboolean got_page;	/* Flag */
	gboolean got_rpm;	/* Flag */
	gboolean got_load;	/* Flag */
	gboolean got_ve;	/* Flag */
	
};


/*!
 \brief The Logview_Data struct is a ontainer used within the logviewer_gui.c
 file used to store settigns specific to the logviewer including th pointer to
 the drawing area, and a hashtable and list of pointers to the trace 
 datastructures.
 */
struct Logview_Data
{
	GdkGC *highlight_gc;	/*! GC used for the highlight */
	GtkWidget *darea;	/*! Trace drawing area... */
	GdkPixmap *pixmap;	/*! pointer to backing pixmap... */
	GdkPixmap *pmap;	/*! pointer to Win32 pixmap hack!!! */
	GHashTable *traces;	/*! Hash table of v_values key'd by name */
	GList *tlist;		/*! Doubly linked lists of v_Values*/
	GList *used_colors;	/*! List of colors in use.... */
	gint active_traces;	/*! how many are active */
	gint spread;		/*! Pixel spread between trace info blocks */
	gint tselect;		/*! Trace that is currently selected */
	PangoFontDescription *font_desc; /*! Font used for text... */
};

/*!
 * \brief TrigToothMon_Data struct is a container used to hold private data
 * for the Trigger and Tooth Loggers (MSnS-E only)
 */
struct TTMon_Data
{
	gint page;		/*! page used to discern them apart */
	GdkPixmap *pixmap;	/*! Pixmap */
	GtkWidget *darea;	/*! Pointer to drawing area */
	gint min_time;		/*! Minimum, trigger/tooth time */
	gint num_maxes;		/*! Hot many long pips per block */
	gint mins_inbetween;	/*! How many normal teeth */
	gint max_time;		/*! Maximum, trigger/tooth time */
	gint midpoint_time;	/*! avg between min and max */
	gint est_teeth;		/*! Estimated number of teeth */
	gint units;		/*! Units multiplier */
	gfloat usable_begin;	/*! Usable begin point for bars */
	gfloat font_height;	/*! Font height needed for some calcs */
	GArray *current;	/*! Current block of times */
	GArray *last;		/*! Last block of times */
	gint wrap_pt;		/*! Wrap point */
	gint vdivisor;		/*! Vertical scaling divisor */
	gfloat peak;		/*! Vertical Peak Value */
	PangoFontDescription *font_desc;	/*! Pango Font Descr */
	PangoLayout *layout;	/*! Pango Layout */
	GdkGC *axis_gc;		/*! axis graphics context */
	GdkGC *trace_gc;	/*! axis graphics context */

};


/*!
 * \brief MultiExpr is a container struct used for Realtime var processing
 * for vars that depend on ECU state (MAP, need to know current sensor)
 */
struct MultiExpr
{
	gint lower_limit;	/* Lower limit */	
	gint upper_limit;	/* Upper limit */
	gchar *lookuptable;	/* textual lookuptable name */
	gchar *dl_conv_expr;	/* download (to ecu) conv expression */
	gchar *ul_conv_expr;	/* upload (from ecu) conv expression */
	void *dl_eval;		/* evaluator for download */
	void *ul_eval;		/* evalutator for upload */
};


/*!
 * \brief MultiSource is a container struct used for Table data handling 
 * for the x/y/z axis's for properly scaling and displaying things on the
 * 3D displays as well as the 2D table scaling.  This allows things to be
 * significantly more adaptable 
 */
struct MultiSource
{
	gchar *source;		/* name of rtvars datasource */
	gchar *conv_expr;	/* conversion expression ms units to real */
	void * evaluator;	/* evaluator pointer */
	gchar * suffix;		/* textual suffix for this evaluator*/
	gint precision;		/* Precision for floating point */
};

#endif
