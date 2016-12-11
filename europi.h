// Copyright 2016 Richard R. Goodwin / Audio Morphology
//
// Author: Richard R. Goodwin (richard.goodwin@morphology.co.uk)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.

#ifndef EUROPI_H
#define EUROPI_H

#include <pigpio.h>
#include "quantizer_scales.h"

/* GPIO Port Assignments 		*/
/* RPi Header pins in comments	*/
#define I2C_SDA		2	/* PIN 3	*/
#define I2C_SCL		3	/* PIN 5	*/
#define MASTER_CLK	18	/* PIN 12	*/
#define BUTTON1_IN	5	/* PIN 29	*/
#define BUTTON2_IN	6	/* PIN 31	*/
#define BUTTON3_IN	13	/* PIN 33	*/
#define BUTTON4_IN	19	/* PIN 35	*/
#define ENCODERB_IN	4	/* PIN 7	*/
#define ENCODERA_IN	14  /* PIN 8	*/
#define ENCODER_BTN 15	/* PIN 10 	*/
#define CLOCK_IN	12	/* PIN 32	*/
#define INTEXT_IN	20	/* PIN 38	*/
#define RUNSTOP_IN	26	/* PIN 37	*/
#define RESET_IN	16	/* PIN 36	*/
#define TOUCH_INT	17	/* PIN 11	*/

/* Hardware Address Constants */
#define DAC_BASE_ADDR 	0x4C	/* Base i2c address	of DAC8574 */
#define MCP_BASE_ADDR	0x20	/* Base i2c address of MCP23008 GPIO Expander */
#define PCF_BASE_ADDR	0x38	/* Base i2c address of PCF8574 GPIO Expander */

/* Channels */
#define CV_OUT		0x00		/* Europi and Minion */
#define GATE_OUT	0x01		/* Europi and Minion */
#define CLOCK_OUT	0x02		/* Europi only */
#define STEP1_OUT	0x03		/* Europi only */

/* Useful Logicals */
#define RUN			1
#define STOP		0
#define HIGH		1
#define LOW			0
#define INT_CLK		0
#define EXT_CLK		1
#define TRUE		1
#define FALSE		0

/* Typedefs */
typedef unsigned char     uint8_t;  		/*unsigned 8 bit definition */
enum encoder_focus_t {
	none,
	pitch_cv,
	slew_type,
	gate_on_off,
	repeat,
	quantise,
	menu_on,
	track_select,
	step_select,
	set_zerolevel,
	set_maxlevel,
	set_loop,
	set_pitch,
	set_quantise
};
enum slew_t {
	Off,
	Linear,
	Logarithmic,
	Exponential,
	ADSR,
	AD
};

enum slew_shape_t {
	Both,
	Rising,
	Falling
};

enum gate_type_t {
	GateOff,
	Trigger,
	ReTrigger,
	Gate
};

enum shot_type_t {
	OneShot,
	Repeat
};

/* Function Prototypes in europi_func1 */
int startup(void);
int shutdown(void);
void paint_menu(void);
void log_msg(const char*, ...);
void controlled_exit(int gpio, int level, uint32_t tick);
void master_clock(int gpio, int level, uint32_t tick);
void encoder_callback(int gpio, int level, uint32_t tick);
void encoder_button(int gpio, int level, uint32_t tick);
void button_1(int gpio, int level, uint32_t tick);
void button_2(int gpio, int level, uint32_t tick);
void button_3(int gpio, int level, uint32_t tick);
void button_4(int gpio, int level, uint32_t tick);
void next_step(void);
void next_step_old(void);
void splash_screen(void);
void paint_front_panel(void); 
void step_button_grid(void);
void draw_button(int x, int y, unsigned short c);
void draw_button_number(int button_number, unsigned short c);
void select_button(int button_number, unsigned short c);
void channel_level(int channel, int level);
void gate_state(int channel, int state);
void touch_interrupt(int gpio, int level, uint32_t tick);
void button_touched(int x, int y);
int MinonFinder(unsigned address);
int EuropiFinder(void);
void DACSingleChannelWrite(unsigned handle, uint8_t address, uint8_t channel, uint16_t voltage);
void GATESingleOutput(unsigned handle, uint8_t channel,int Device,int Value);
void hardware_init(void);
int quantize(int raw, int scale);
static void *SlewThread(void *arg);
static void *GateThread(void *arg);
static void *AdThread(void *arg);

/* Function Prototypes in europi_func2 */
void select_first_track(void);
void seq_new(void);
void clear_screen_elements(void);
void seq_quantise(void);
void seq_setpitch(void);
void seq_setloop(void);
void test_scalevalue(void);
void file_save(void);
void config_setzero(void);
void config_setten(void);
void set_zero(int Track, long ZeroVal);
void file_quit(void);
void pitch_adjust(int dir, int vel);
void gate_onoff(int dir, int vel);
void step_repeat(int dir, int vel);
void init_sequence(void);
void quantize_track(int track, int scale);
uint16_t scale_value(int track,uint16_t raw_value);

 
/* Function Prototypes for europi_framebuffer_utils */
void put_pixel(char *, int x, int y, int c);
void put_pixel_RGB24(char *, int x, int y, int r, int g, int b);
void put_pixel_RGB565(char *, int x, int y, unsigned short c);
void draw_line(char *, int x0, int y0, int x1, int y1, int c);
void draw_line_RGB565(char *, int x0, int y0, int x1, int y1, unsigned short c);
void draw_rect(char *, int x0, int y0, int w, int h, int c);
void draw_rect_RGB565(char *, int x0, int y0, int w, int h, unsigned short c);
void fill_rect(char *, int x0, int y0, int w, int h, int c);
void fill_rect_RGB565(char *, int x0, int y0, int w, int h, unsigned short c);
void draw_circle(char *, int x0, int y0, int r, int c);
void draw_circle_RGB565(char *, int x0, int y0, int rad,  unsigned short c);
void fill_circle(char *, int x0, int y0, int r, int c);
void fill_circle_RGB565(char *, int x0, int y0, int rad,  unsigned short c);
unsigned short RGB2565(int r, int g, int b);
unsigned short HEX2565(long c);
void put_string(char *, int x, int y, char *s, unsigned short fgc, unsigned short bgc);
void put_char(char *, int x, int y, int c, unsigned short fgc, unsigned short bgc);

/* Screen Colours */
#define SCR_BACK_CLR		0xD936CF	/* main background colour */
#define SCR_FRAMEBORDER_CLR	0xFFFFFF	/* border around main frames */
#define LED_OFF				0xFF00FF	/* Unlit LED */
#define LED_ON				0x00FF00	/* Lit LED */
#define MENU_BACK_CLR		4			/* Menu Panel background colour */
#define BTN_BACK_CLR		0xf8ea53	/* Default background colour for the grid of sequence buttons */
#define BTN_STEP_CLR		0x59ef10	/* Colour of the current step  */
#define BTN_SELECT_CLR		0xFF0000	/* Colour of the box drawn around a button to indicate that it is currently selected */
#define CHNL_BACK_CLR		0x000000	/* Background (non-illuminated) level for each channel indicator */
#define CHNL_FORE_CLR		0x32e30f	/* Foreground (illuminated) colour for each channel indicator */
#define GATE_BACK_CLR		0x000000	/* Background (non-illuminated) of each Gate State indicator */
#define GATE_FORE_CLR		0xec0718	/* Foreground (illuminate) colour for each Gate State indicator */

/* Screen Object Sizes */
#define LED_SIZE			8
#define	MENU_WIDTH			317
#define MENU_HEIGHT			30
#define	MENU_X				1
#define	MENU_Y				209
#define BTN_STEP_WIDTH		16			/* width of each step button */
#define BTN_STEP_HEIGHT		32			/* height of each step button */
#define BTN_STEP_HGAP		13			/* gap between each step button horizontally */
#define BTN_STEP_VGAP		14			/* gap between each step button vertically */
#define BTN_STEP_COLS		8			/* how many Step buttons to display horizontally */
#define BTN_STEP_ROWS		4			/* how many rows of buttons */
#define BTN_STEP_TLX		15			/* X coordinate of Top Left of 1st button */
#define BTN_STEP_TLY		14			/* Y coordinate of Top Left of 1st button */
#define CHNL_LEVEL_WIDTH	4			/* Width of each channel level indicator */
#define CHNL_LEVEL_HEIGHT	160			/* Height of each channel level indicator */
#define CHNL_LEVEL_HGAP		6			/* Gap between each channel level indicator */
#define CHNL_LEVEL_TLX		252			/* X coordinate of Top Left of 1st Channel Level indicator */
#define CHNL_LEVEL_TLY		14			/* Y coordinate of Top Left of 1st Channel Level indicator */
#define CHANNELS			2			/* How many CV / Gate Channels */
#define GATE_STATE_WIDTH	4			/* Width of the Gate State indicator */
#define GATE_STATE_HEIGHT	4			/* Height of the Gate State indicators */
#define GATE_STATE_HGAP		6			/* Gap between each Gate State indicator */
#define GATE_STATE_TLX		252			/* X Coordinate of Top Left of 1st channel Gate State indicator */
#define GATE_STATE_TLY		183			/* Y Coordinate of Top Left of 1st channel Gate State indicator */

/* Global Constants */
// Some global variables that we'll need all over the place
#define X_MAX 320 
#define Y_MAX 240
#define TOUCH_EVENT_HANDLER "/dev/input/event2"	/* need to set this to match the event handler, use cat /proc/bus/input/devices to find which event hsandler the touchscreen is using */
#define SAMPLE_AMOUNT 2					/* number of times to sample touchscreen (to reduce jitter) */
/* min and max values returned by from touchscreen - need to measure
 * these from the actual screen used, and set these values accordingly */
#define rawXmin				231
#define rawXmax				3700
#define rawYmin				380
#define rawYmax				3775

/* 
 * The Main Europi Structures, which are used to construct
 * a single Structure that holds all of the details for
 * the hardware configuration, running sequence, timing
 * loop points etc.
 *
 * Sequencer array sizes - using static sizes
 * rather than completely flexible dynamically
 * allocated sizes, though this could be changed 
 * if more flexibility is needed
 */ 
#define MAX_SEQUENCES 64	/* Song can contain up to 64 sequences */
#define MAX_TRACKS 2+(4*8)	/* 2 Tracks on Europi, plus 4 per minion, with total of 8 Minions */ 
#define MAX_CHANNELS 2		/* 2 channels per track (CV + GATE) */
#define MAX_STEPS 32		/* Up to 32 steps in an individual sequence */
/* CHANNEL TYPE */
#define CHNL_TYPE_CV 0
#define CHNL_TYPE_GATE 1
#define CHNL_TYPE_TRIGGER 2
/* Voltage control type */
#define VOCT 0				/* Volts per Octave */
#define VHZ 1				/* Hertz per Volt */
/* device Types */
#define DEV_MCP23008 0
#define DEV_DAC8574 1
#define DEV_RPI 2
#define DEV_PCF8574 3

/* Menu structures - Menus are defined using 
 * Arrays of structures containing the
 * Menu Tree & leaf element names, Functions
 * to call etc.
 */
typedef struct MENU{
	int expanded;					// Whether this node is expanded or not
	int highlight;					// whether to highlight this leaf
	const char *name;				// Menu display text
	void (*funcPtr)();				// Handler for this leaf node (Optionally NULL)
	struct MENU *child[8];			// Pointer to child submenu (Optionally NULL)
}menu;

/* Screen element structures - track
 * whether particular screen elements
 * are visible or not
 */
 struct screen_elements{
	 int GridView;
	 int MainMenu;
	 int SetZero;
	 int SetTen;
	 int ScaleValue;
	 int SetLoop;
	 int SetPitch;
	 int SetQuantise;
 };
 
/*
 * SLEW structure is instantiated and filled with
 * details about the slew for the current step - 
 * Start & End values, Type, Length etc
 */
 struct slew {
	int track;				/* which track spawned the thread */
	int i2c_handle;			/* Handle to the i2c device that outputs this track */
	int i2c_address;		/* Address of this device on the i2c Bus - address need to match the physical A3-A0 pins */
	int i2c_channel;		/* Individual channel (on multi-channel i2c devices) */
	int i2c_device;			/* Type of device (needed for Gate / Trigger outputs */
	uint16_t start_value; 	/* Value for start of slew */
	uint16_t end_value; 	/* Value to reach at end of slew */
	uint32_t slew_length;	/* How long we've got to get there (in microseconds) */
	enum slew_t slew_type;	/* Off, Linear, Logarithmic, Exponential */	
	enum slew_shape_t slew_shape; /* Both, Rising, Falling */
};

struct gate {
	int track;				/* which track spawned the thread */
	int i2c_handle;			/* Handle to the i2c device that outputs this track */
	int i2c_address;		/* Address of this device on the i2c Bus - address need to match the physical A3-A0 pins */
	int i2c_channel;		/* Individual channel (on multi-channel i2c devices) */
	int i2c_device;			/* Type of device (needed for Gate / Trigger outputs */
	uint32_t gate_length;	/* Gate ON period (in microseconds) */
	enum gate_type_t gate_type;   /* Off, Trigger, Gate */
	int retrigger_count;	/* How many times to re-trigger during the step */
};

struct adsr {
	int track;				/* which track spawned the thread */
	int i2c_handle;			/* Handle to the i2c device that outputs this track */
	int i2c_address;		/* Address of this device on the i2c Bus - address need to match the physical A3-A0 pins */
	int i2c_channel;		/* Individual channel (on multi-channel i2c devices) */
	int i2c_device;			/* Type of device (needed for Gate / Trigger outputs */
	uint16_t a_start_value;	/* Start value for Attack Ramp */
	uint16_t a_end_value;	/* End value for Attack Ramp */
	uint32_t a_length;		/* Length of Attack Ramp */
	uint16_t d_start_value;	/* Start value for Decay Ramp */
	uint16_t d_end_value;	/* End value for Decay Ramp */
	uint32_t d_length;		/* Length of Decay Ramp */
	uint32_t s_length;		/* Length of Sustain */
	uint16_t r_start_value;	/* Start value for Release Ramp */
	uint16_t r_end_value;	/* End value for Release Ramp */
	uint32_t r_length;		/* Length of Release Ramp */
};

struct ad {
	int track;				/* which track spawned the thread */
	int i2c_handle;			/* Handle to the i2c device that outputs this track */
	int i2c_address;		/* Address of this device on the i2c Bus - address need to match the physical A3-A0 pins */
	int i2c_channel;		/* Individual channel (on multi-channel i2c devices) */
	int i2c_device;			/* Type of device (needed for Gate / Trigger outputs */
	uint16_t a_start_value;	/* Start value for Attack Ramp */
	uint16_t a_end_value;	/* End value for Attack Ramp */
	uint32_t a_length;		/* Length of Attack Ramp */
	uint16_t d_end_value;	/* End value for Decay Ramp */
	uint32_t d_length;		/* Length of Decay Ramp */
	enum shot_type_t shot_type; /* One-shot or Repeat */
};

/*
 * STEP is an individual step in a sequence. Steps
 * are unique to a particular track. Steps contain
 * the output voltage, gate state etc for a Step
 * 
 */
struct step {
	int raw_value;			/* Non-scaled value to output on a 6000 step/Octave scale*/
	uint16_t scaled_value; 	/* Scaled / Quantised value to output */
	enum slew_t slew_type;	/* Off, Linear, Logarithmic, Exponential, AD, ADSR */
	enum slew_shape_t slew_shape;	/* Both, Rising, Falling*/
	uint32_t slew_length; 	/* Slew length (in microseconds) */
	int retrigger;			/* Number of times to retrigger this step - permitted values are 0,2,3 or 4 */
	int ratchet;			/* Ratchet fits the repeats into this beat */
	int gate_type;			/* Type of gate output (GATE or TRIGGER) */
	int step_type;			/* ON, SKIP (missed out) or EXTEND (carries on the previous step) */
	int gate_value;			/* 0 = Off, 1 = ON */
};

/* 
 * CHANNEL is an set of parameters for a single output,
 * which can be a CV channel, Gate output etc. Usually
 * a CV and GATE output would be linked together in a 
 * single TRACK
 */
struct channel {
	int enabled;			/* Whether this channel is in use or not */
	int type;				/* Types include CV, GATE, TRIGGER */
	int quantise;			/* whether this channel is quantised to a particular scale 0=OFF*/
	int i2c_handle;			/* Handle to the i2c device that outputs this track */
	int i2c_device;			/* Type of i2c device DAC8754, MCP23008 etc */
	int i2c_address;		/* Address of this device on the i2c Bus - address need to match the physical A3-A0 pins */
	int i2c_channel;		/* Individual channel (on multi-channel i2c devices) */
	long scale_zero;		/* Value required to generate zero volt output */
	long scale_max;			/* Value required to generate 10v output voltage */
	long transpose;			/* fixed (transpose) voltage offset applied to this channel */
	int	octaves;			/* How many octaves are covered from scale_zero to scale_max */
	int vc_type;			/* Type of voltage control output V_OCT or V_HZ (ignored for gates) */
	struct step steps[MAX_STEPS];	/* Array of steps */ 	
};

/*
 * a TRACK comprises two physical output channels linked 
 * together - a CV output plus its associated GATE output.
 */
struct track{
	int selected;			/* Track is selected for some sort of operation */
	int track_busy;			/* If TRUE then this Track won't advance to the next step */
	int current_step;		/* Tracks where this track is going next */
	int last_step;			/* sets the end step for a particular track */
	struct channel channels[MAX_CHANNELS];	/* a TRACK contains an array of CHANNELs */
};
/*
 * Europi is the main Container structure for the Hardware
 */
struct europi{
	int track_enabled;
	struct track tracks[MAX_TRACKS];
};
/* 
 * PATTERN is one main loopable section, 
 * can contain many TRACKS 
 */
struct pattern {
	int sequence_index;
	struct track tracks[MAX_TRACKS];
};
/* SEQUENCE contains many Patterns chained together */
struct sequence {
	int sequence_index;
	int next_pattern;							/* used to control the way sequences are chained together in a Song */
	struct pattern patterns[MAX_SEQUENCES];	/* Array of Sequences */
};

#endif /* EUROPI_H */
