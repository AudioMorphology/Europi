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
#include "slew_profiles.h"
#include "quantizer_scales.h"
#include "bjorklund.h"

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
//#define TOUCH_INT	17	/* PIN 11	*/

/* SC16IS750 UART register definitions */
#define SC16IS750_RHR	0x00 << 3
#define SC16IS750_THR	0x00 << 3
#define SC16IS750_IER	0x01 << 3
#define SC16IS750_FCR	0x02 << 3
#define SC16IS750_IIR	0x02 << 3
#define SC16IS750_LCR	0x03 << 3
#define SC16IS750_MCR	0x04 << 3
#define SC16IS750_LSR	0x05 << 3
#define SC16IS750_MSR	0x06 << 3
#define SC16IS750_SPR	0x07 << 3
#define SC16IS750_TCR	0x06 << 3
#define SC16IS750_TLR	0x07 << 3
#define SC16IS750_TXLVL	0x08 << 3
#define SC16IS750_RXLVL	0x09 << 3
#define SC16IS750_IODIR	0x0A << 3
#define SC16IS750_IOSTATE	0x0B << 3
#define SC16IS750_IOINTENA	0x0C << 3
#define SC16IS750_IOCONTROL	0x0E << 3
#define SC16IS750_EFCR	0x0F << 3
#define SC16IS750_DLL	0x00 << 3	//Divisor Latch low
#define SC16IS750_DLH	0x01 << 3	//Divisor Latch high

/* Hardware Address Constants */
#define DAC_BASE_ADDR 	0x4C	/* Base i2c address	of DAC8574 */
#define MCP_BASE_ADDR	0x20	/* Base i2c address of MCP23008 GPIO Expander */
#define PCF_BASE_ADDR	0x38	/* Base i2c address of PCF8574 GPIO Expander */
#define MID_BASE_ADDR	0x50	/* Base i2c address of MIDI Minion SC16IS750 UART */

/* Channels */
#define CV_OUT		0x00		/* Europi and Minion */
#define GATE_OUT	0x01		/* Europi and Minion */
#define CLOCK_OUT	0x02		/* Europi only */
#define STEP1_OUT	0x03		/* Europi only */
#define MIDI_OUT    0x04        /* MIDI Minion only */

/* Useful Logicals */
#define RUN			1
#define STOP		0
#define HIGH		1
#define LOW			0
#define INT_CLK		0
#define EXT_CLK		1
#define MIDI_CLK    2
#define TRUE		1
#define FALSE		0
#define UP          1
#define DOWN        -1
#define ON          1
#define OFF         0

/* Menu Constants */
#define MENU_FONT_SIZE      20
#define MENU_TOP_MARGIN     3   // How many pixels above & below (Top & Bottom) the Menu Text
#define MENU_TB_MARGIN      1   // How many pixels above & below (Top & Bottom) the Menu Text
#define MENU_LR_MARGIN      3   // How many pixels to Left and Right of the Menu Text
#define MENU_HORIZ_SPACE    2   // Horizontal Gap between top-level menus

/* Typedefs */
typedef unsigned char     uint8_t;  		/*unsigned 8 bit definition */

enum track_dir_t {
    Forwards,
    Backwards,
    Pendulum_F,
    Pendulum_B,
    Random
};

enum direction_t {
    dir_none,
    dir_up,
    dir_down,
    dir_left,
    dir_right
};

enum device_t {
//	CV_OUT,
//	GATE_OUT,
	MIDI_IN,
//	MIDI_OUT,
//	CLOCK_OUT,
//		STEP1_OUT
};

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
	set_quantise,
    set_direction,
    keyboard_input,
    file_open_focus
};

enum btnA_func_t {
    btnA_none,
    btnA_quit,
    btnA_select
};

enum btnB_func_t {
    btnB_none,
    btnB_menu,
    btnB_open,
    btnB_save,
    btnB_cancel,
    btnB_val_down,
    btnB_prev,
    btnB_tr_minus
};

enum btnC_func_t {
    btnC_none,
    btnC_bpm_dn,
    btnC_OK,
    btnC_cancel,
    btnC_val_up,
    btnC_next,
    btnC_tr_plus
};

enum btnD_func_t {
    btnD_none,
    btnD_bpm_up,
    btnD_done
};

enum slew_t {
	Off,
	Linear,
    Exponential,
	RevExp,         // Reverse Exponential
	ADSR,
	AD
};

enum slew_shape_t {
	Both,
	Rising,
	Falling
};

enum gate_type_t {
	Gate_Off,    // no gate On
    Gate_On,     // no gate Off
	Trigger,    // 10ms Pulse
    Gate_25,    // Gate on for 25% of step length
    Gate_50,
    Gate_75,
    Gate_95
};

enum shot_type_t {
	OneShot,
	Repeat
};

enum MidiType {
    InvalidType           = 0x00,    ///< For notifying errors
    NoteOff               = 0x80,    ///< Note Off
    NoteOn                = 0x90,    ///< Note On
    AfterTouchPoly        = 0xA0,    ///< Polyphonic AfterTouch
    ControlChange         = 0xB0,    ///< Control Change / Channel Mode
    ProgramChange         = 0xC0,    ///< Program Change
    AfterTouchChannel     = 0xD0,    ///< Channel (monophonic) AfterTouch
    PitchBend             = 0xE0,    ///< Pitch Bend
    SystemExclusive       = 0xF0,    ///< System Exclusive
    TimeCodeQuarterFrame  = 0xF1,    ///< System Common - MIDI Time Code Quarter Frame
    SongPosition          = 0xF2,    ///< System Common - Song Position Pointer
    SongSelect            = 0xF3,    ///< System Common - Song Select
    TuneRequest           = 0xF6,    ///< System Common - Tune Request
    Clock                 = 0xF8,    ///< System Real Time - Timing Clock
    Start                 = 0xFA,    ///< System Real Time - Start
    Continue              = 0xFB,    ///< System Real Time - Continue
    Stop                  = 0xFC,    ///< System Real Time - Stop
    ActiveSensing         = 0xFE,    ///< System Real Time - Active Sensing
    SystemReset           = 0xFF,    ///< System Real Time - System Reset
};

/* Function Prototypes in europi_func1 */
int startup(void);
int shutdown(void);
void log_msg(const char*, ...);
void controlled_exit(int gpio, int level, uint32_t tick);
void master_clock(int gpio, int level, uint32_t tick);
void encoder_callback(int gpio, int level, uint32_t tick);
void encoder_button(int gpio, int level, uint32_t tick);
void toggle_menu(void);
void ClearMenus(void);
void MenuSelectItem(int Parent, int Child);
void button_1(int gpio, int level, uint32_t tick);
void button_2(int gpio, int level, uint32_t tick);
void button_3(int gpio, int level, uint32_t tick);
void button_4(int gpio, int level, uint32_t tick); 
void next_step(void);
int MidiMinonFinder(unsigned address);
int MinonFinder(unsigned address);
int EuropiFinder(void);
void MIDISingleChannelWrite(unsigned handle, uint8_t channel, uint8_t velocity, uint16_t voltage);
void DACSingleChannelWrite(unsigned handle, uint8_t address, uint8_t channel, uint16_t voltage);
void GATESingleOutput(unsigned handle, uint8_t channel,int Device,int Value);
void hardware_init(void); 
int quantize(int raw, int scale);
int pitch2midi(uint16_t voltage);
void *SlewThread(void *arg); 
void *GateThread(void *arg);
void *AdThread(void *arg); 
void *AdsrThread(void *arg); 
void *MidiThread(void *arg); 

/* Function Prototypes in europi_func2 */ 
void seq_singlechnl(void);
void seq_gridview(void);
void select_first_track(void);
void select_track(int track);
void select_next_track(int dir);
void select_next_step(int dir);
void set_loop_point(int dir);
void select_next_quantisation(int dir);
void select_next_direction(int dir);
void set_step_pitch(int dir,int vel);
void seq_new(void);
void ClearScreenOverlays(void);
int OverlayActive(void);
void buttonsDefault(void);
void seq_quantise(void);
void seq_setdir(void);
void seq_setpitch(void);
void seq_setloop(void);
void seq_setslew(void);
void test_scalevalue(void);
void test_keyboard(void);
void file_save(void);
void file_saveas(void);
void file_open(void); 
void config_setzero(void);
void config_setten(void);
void config_debug(void);
//void config_calibtouch(void);
void set_zero(int Track, long ZeroVal);
void file_quit(void);
void load_sequence(const char *filename);
void slew_adjust(int dir, int vel);
void step_repeat(int dir, int vel);
void init_sequence(void);
void quantize_track(int track, int scale);
uint16_t scale_value(int track,uint16_t raw_value);
size_t file_list(const char *path, char ***ls);
int cstring_cmp(const void *a, const void *b);
int polyrhythm(uint32_t Steps, uint32_t Fill, uint32_t ThisStep);
void hardware_config(void);


/* function prototypes in europi_gui.c */
void gui_8x8(void);
void gui_singlestep(void);
void gui_grid(void);
void gui_SingleChannel(void);
void gui_ButtonBar(void);
void gui_MainMenu(void);
void ShowScreenOverlays(void);
void gui_debug(void);

/* function prototypes in touch.c */
//void *TouchThread(void *arg);
//void InitTouch(void);

/* Global Constants */
#define X_MAX 320 
#define Y_MAX 240
#define KBD_GRID_TL_X 0
#define KBD_GRID_TL_Y 95
#define KBD_BTN_TL_X 5
#define KBD_BTN_TL_Y 5
#define KBD_BTN_WIDTH 29
#define KBD_BTN_HEIGHT 29
#define KBD_COL_WIDTH 28
#define KBD_ROW_HEIGHT 28
#define KBD_ROWS 4
#define KBD_COLS 11
#define DLG_ROWS 9
#define VERTICAL_SCROLL_MAX 187
#define VERTICAL_SCROLL_MIN 18

// Two shades of BLUE used in menus etc.
#define CLR_DARKBLUE (Color){ 153, 181, 208, 255 } 
#define CLR_LIGHTBLUE (Color){ 186, 212, 238, 255 } 
//#define DLG_BTN1_X 18
//#define DLG_BTN1_Y 189
//#define DLG_BTN1_W 121
//#define DLG_BTN1_H 39
//#define DLG_BTN2_X 181
//#define DLG_BTN2_Y 189
//#define DLG_BTN2_W 121
//#define DLG_BTN2_H 39


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
#define CHNL_TYPE_MIDI 3
/* Voltage control type */
#define VOCT 0				/* Volts per Octave */
#define VHZ 1				/* Hertz per Volt */
/* device Types */
#define DEV_MCP23008 0
#define DEV_DAC8574 1
#define DEV_RPI 2
#define DEV_PCF8574 3
#define DEV_SC16IS750 4

/* Menu structures - Menus are defined using 
 * Arrays of structures containing the
 * Menu Tree & leaf element names, Functions
 * to call etc.
 */
typedef struct MENU{
	int expanded;					// Whether this node is expanded or not
	int highlight;					// whether to highlight this leaf
    enum direction_t direction;     // which direction branches from this leaf open
	const char *name;				// Menu display text
	void (*funcPtr)();				// Handler for this leaf node (Optionally NULL)
	struct MENU *child[9];			// Pointer to child submenu (Optionally NULL)
}menu;

/* Display Pages - only one sort of 
 * page can be displayed at any point
 * in time, so an Enumerated Type is
 * used to record what Page is being 
 * displayed.
 */
enum display_page_t {
	GridView,
	SingleChannel,
    SingleStep
};

/* Overlays are things such as Menus, 
 * which can be overlaid on top of whatever
 * page is currently being displayed
 */ 
 
 //To Do: This needs to be a bitwise field, rather than a sstructure of Ints !!!
 struct screen_overlays{
	 int MainMenu;
	 int SetZero;
	 int SetTen;
	 int ScaleValue;
	 int SetLoop;
	 int SetPitch;
     int SetSlew;
	 int SetQuantise;
     int SetDirection;
     int Keyboard;
     int FileOpen;
     int TextInput;
     int FileSaveAs;
     int VerticalScrollBar;
     int SingleStep;
     int SingleChannel;
     int ModalDialog;
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
	enum gate_type_t gate_type;   /* Off, Trigger, Gate */
	int ratchets;	        /* How many times to re-trigger during the step */
    int fill;               /* Euclidian fill value - if this is greater or equal to the ratchets, then every ratches will sound */
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
	uint32_t d_length;		/* Length of Decay Ramp */
    uint32_t s_level;       /* Sustain Level */
	uint32_t s_length;		/* Length of Sustain */
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

struct midiChnl {
    int i2c_handle;
};
/* 
 * DEVICE records the physical configuration of 
 * the Europi system, including device handles,
 * Output scaling parameteres Etc. Keeping this
 * information separate to the Sequence itself
 * enables existing sequences to be run on differing
 * base hardware configurations
 */
struct device {
	enum device_t device_type;	/* CV_OUT, GATE_OUT etc. */
	int i2c_handle;				/* Handle to the i2c device that outputs this track */
	int i2c_device;				/* Type of i2c device DAC8754, MCP23008 etc */
	int i2c_address;			/* Address of this device on the i2c Bus - address need to match the physical A3-A0 pins */
	int i2c_channel;			/* Individual channel (on multi-channel i2c devices) */
	long scale_zero;			/* Value required to generate zero volt output */
	long scale_max;				/* Value required to generate 10v output voltage */
	float scale_factor;			/* scaled_value = (raw * scale_factor) + scale_zero */
};

/*
 * STEP is an individual step in a sequence. Steps
 * are unique to a particular track & channel. Steps contain
 * the output voltage, gate state etc for a Step.
 * 
 */
struct step {
    // Applicable to steps within a CV Channel
	int raw_value;			/* Non-scaled value to output on a 6000 step/Octave scale*/
	uint16_t scaled_value; 	/* Scaled / Quantised value to output */
	enum slew_t slew_type;	/* Off, Linear, Logarithmic, Exponential, AD, ADSR */
	enum slew_shape_t slew_shape;	/* Both, Rising, Falling*/
	uint32_t slew_length; 	/* Slew length (in microseconds) */
	// Applicable to steps within a GATE Channel
    enum gate_type_t gate_type;
	int ratchets;		    /* Number or ratchets to fit into this Step */
    int fill;              /* Number of beats to fit within the number of Ratchets (Euclidian polyrhythm generator) */
    int repetitions;        /* Number of times to repeat this step */
    int repeat_counter;     /* Counter within the step itself to track step repeats */
};

/* 
 * CHANNEL is an set of parameters for a single output,
 * which can be a CV channel, Gate output etc. Usually
 * a CV and GATE output would be linked together in a 
 * single TRACK
 */
struct channel {
	struct step steps[MAX_STEPS];	/* Array of steps */
	int i2c_handle;				/* Handle to the i2c device that outputs this track */
	int i2c_device;				/* Type of i2c device DAC8754, MCP23008 etc */
	unsigned i2c_address;		/* Address of this device on the i2c Bus - address need to match the physical A3-A0 pins */
	unsigned i2c_channel;		/* Individual channel (on multi-channel i2c devices) */
	uint16_t scale_zero;			/* Value required to generate zero volt output */
	uint16_t scale_max;				/* Value required to generate 10v output voltage */
	int enabled;			/* Whether this channel is in use or not */
	int type;				/* Types include CV, GATE, TRIGGER */
	int quantise;			/* whether this channel is quantised to a particular scale 0=OFF*/
	long transpose;			/* fixed (transpose) voltage offset applied to this channel */
	int	octaves;			/* How many octaves are covered from scale_zero to scale_max */
	int vc_type;			/* Type of voltage control output V_OCT or V_HZ (ignored for gates) */
};

/*
 * a TRACK comprises two physical output channels linked 
 * together - a CV output plus its associated GATE output.
 */
struct track{
	struct channel channels[MAX_CHANNELS];	/* a TRACK contains an array of CHANNELs */
	int selected;			    /* Track is selected for some sort of operation */
	int track_busy;			    /* If TRUE then this Track won't advance to the next step */
	int current_step;		    /* Tracks where this track is going next */
	int last_step;			    /* sets the end step for a particular track */
    enum track_dir_t direction; /* Forwards, Backwards, Pendulum, Random */
};
/*
 * Europi is the main Container structure for the Hardware
 */
struct europi{
	struct track tracks[MAX_TRACKS];
};
/*
 * hw_channel holds the Hardware config for a Channel - forms
 * part of the Europi_hw structure, and is only used to save the
 * hardware config, and to compare the current config with the 
 * saved config during program initialisation.
 */
struct hw_channel{
	int i2c_handle;				/* Handle to the i2c device that outputs this track */
	int i2c_device;				/* Type of i2c device DAC8754, MCP23008 etc */
	unsigned i2c_address;		/* Address of this device on the i2c Bus - address need to match the physical A3-A0 pins */
	unsigned i2c_channel;		/* Individual channel (on multi-channel i2c devices) */
	uint16_t scale_zero;		/* Value required to generate zero volt output */
	uint16_t scale_max;			/* Value required to generate 10v output voltage */
    int enabled;
    int type;
};
/* 
 * hw_track holds the Hardware config for a track
 */
struct hw_track{
    struct hw_channel hw_channels[MAX_CHANNELS];
};
/*
 * Europi_hw is used to save the device-specific config
 * such as scale zero and 10v settings, and also tracks
 * if the hardware configuration (number of Minions etc)
 * has changed - and prompts the user to re-calibrate if
 * it has.
 */
struct europi_hw{
    struct hw_track hw_tracks[MAX_TRACKS];
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
