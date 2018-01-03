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
 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <linux/kd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <pigpio.h>

#include "europi.h"
//#include "quantizer_scales.h"
//#include "../raylib/release/libs/rpi/raylib.h"
#include "../raylib/src/raylib.h"
unsigned hw_version;			/* Type 1: 2,3 Type 2: 4,5,6 & 15 Type 3: 16 or Greater */
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo; 
struct fb_var_screeninfo orig_vinfo; 
char *fbp = 0;   
int fbfd = 0; 
int kbfd = 0;
int is_europi = FALSE;	/* whether we are running on Europi hardware - set to True in hardware_init() */
int print_messages = TRUE; /* controls whether log_msg outputs to std_err or not */
int debug = FALSE;		/* controls whether debug messages are printed to the main screen */
int impersonate_hw = FALSE;	/* allows the software to bypass hardware checks (useful when testing sw without full hw ) */ 
char input_txt[100];    /* buffer for capturing user input */
char current_filename[100]; /* The File we have Open, which is used in File-Save */
char modal_dialog_txt1[50]; /* 1st Line of text for display in Modal Dialog box */
char modal_dialog_txt2[50]; /* 2nd Line of text for display in Modal Dialog box */
char modal_dialog_txt3[50]; /* 2nd Line of text for display in Modal Dialog box */
char modal_dialog_txt4[50]; /* 2nd Line of text for display in Modal Dialog box */
int prog_running=0;		/* Setting this to 0 will force the prog to quit*/
int ThreadEnd = FALSE; /* semaphore to shut down autonomous threads when the prog ends */
int run_stop=STOP;		/* 0=Stop 1=Run: Halts the main step generator */
int save_run_stop;      /* somewhere to store the stop/run state */
int midi_clock_counter = 0; /* divides down the MIDI Clock into pulses per step */
int midi_clock_divisor = 24; /* MIDI Clock pulses per step */
int clock_counter = 95;	/* Main clock counter, tracks the 96 pulses per step */
int clock_level = 0;	/* Master clock phase */
int clock_source = INT_CLK;	/* INT_CLK = Internal, EXT_CLK = External Clock Source */
int clock_freq=192;		/* speed of the main internal clock in Hz */
int TuningOn = FALSE;    //FALSE;   /* when True, all CV ports will output the same Raw voltage */
uint16_t TuningVoltage = 0;   /* raw value that is output when Tuning flag is Set */
uint8_t PCF8574_state=0xF0; /* current state of the PCF8574 Ports on the Europi */
int led_on = 0;
int last_track;         /* used to trim the for() loops when looping round all tracks */
int step_one = TRUE;	/* used for resetting the sequence to start from the first step */
int step_one_state = LOW; /*records whether the Step 1 pulse needs to be turned off */
int selected_step = -1;	/* records the step that is currently selected. -1=Nothing selected */
int edit_track = 1;     /* Track being edited */
int edit_step = 1;      /* step being edited */
int SingleChannelOffset = 0; /* first step displayed in Single Channel View */
uint32_t step_tick = 0;	/* used to record the start point of each step in ticks */
uint32_t step_ticks = 250000;	/* Records the length of each step in ticks (used to limit slew length) Init value of 250000 is so it doesn't go nuts */
uint32_t slew_interval = 1000; /* number of microseconds between each sucessive level change during a slew */
/* global variables used by the touchscreen interface */
Vector2 touchPosition = { 0, 0 };
int currentGesture;
int lastGesture;

int  xres,yres,x;
int Xsamples[20];
int Ysamples[20];
int screenXmax, screenXmin;
int screenYmax, screenYmin;
float scaleXvalue, scaleYvalue;
int rawX, rawY, rawPressure, scaledX, scaledY;
int Xaverage = 0;
int Yaverage = 0;
int VerticalScrollPercent = 0;          // How far down the vertical scroll bar the handle is
int sample = 0;
int touched = 0;
int encoder_level_A;
int encoder_level_B;
int encoder_lastGpio;
int encoder_vel;
int disp_menu = 0;
int touchStream = -1;                   // Touch device file descriptor
int touchReady = FALSE;                 // Flag to know if touchscreen is present
pthread_t touchThreadId;  
pthread_t midiThreadId[4];              // 4 x Threads for potential MIDI Listeners 
int midiThreadLaunched[4];             // flags to show whether the listener has launched or not              

enum encoder_focus_t encoder_focus;
enum btnA_func_t btnA_func;
enum btnB_func_t btnB_func;
enum btnC_func_t btnC_func;
enum btnD_func_t btnD_func;
int btnA_state = 0;
int btnB_state = 0;
int btnC_state = 0;
int btnD_state = 0;
uint32_t encoder_tick;
pthread_attr_t detached_attr;		/* Single detached thread attribute used by any /all detached threads */
pthread_mutex_t mcp23008_lock;
pthread_mutex_t pcf8574_lock;
uint8_t mcp23008_state[16];
char **files;                       // Filled with a list of filenames in a directory by file_list( )
size_t file_count;                      
int file_selected;
int first_file;
char *kbfds = "/dev/tty";
char *kbd_chars[4][11] = {{"1","2","3","4","5","6","7","8","9","0","_"},
                        {"/","q","w","e","r","t","y","u","i","o","p"},
                        {"/","a","s","d","f","g","h","j","k","l","."},
                        {" "," ","z","x","c","v","b","n","m","~","]"}};		// Note ~ = Backspace, and ] is Return
int kbd_char_selected = 0;
char debug_messages[10][80];
int next_debug_slot = 0;

/* Raylib-related stuff */
SpriteFont font1;
Texture2D Splash;	                // Splash screen texture
Texture2D KeyboardTexture;          // Keyboard Overlay texture
Texture2D DialogTexture;            // Common dialog control
Texture2D TextInputTexture;         // text input dialog box
Texture2D Text2chTexture;           // 2 Character text box (used for Channel number, step number etc)
Texture2D Text5chTexture;           // 5 Character text box 
Texture2D TopBarTexture;            // Top (menu) bar texture
Texture2D MainScreenTexture;        // Main screen
Texture2D ButtonBarTexture;         // Soft-button graphic bar
Texture2D VerticalScrollBarTexture; // Vertical scroll bar for RHS of screen
Texture2D VerticalScrollBarShortTexture; // Short Vertical scroll bar for RHS of screen
Texture2D ScrollHandleTexture;      // Scroll handle for Vewrtical Scroll Bar


/* declare and populate the menu structure */ 
menu mnu_file_open = 	{0,0,dir_none,"Open",&file_open,{NULL}};
menu mnu_file_save = 	{0,0,dir_none,"Save",&file_save,{NULL}};
menu mnu_file_saveas = 	{0,0,dir_none,"Save As",&file_saveas,{NULL}};
menu mnu_file_new = 	{0,0,dir_none,"New",NULL,{NULL}};
menu mnu_file_quit = 	{0,0,dir_none,"Quit",&file_quit,{NULL}};

menu mnu_seq_new = 		{0,0,dir_none,"New",&seq_new,{NULL}};
menu mnu_seq_setslew =  {0,0,dir_none,"Set Slew for Step",&seq_setslew,{NULL}};
menu mnu_seq_setloop =	{0,0,dir_none,"Set Track Length",&seq_setloop,{NULL}};
menu mnu_seq_setpitch = {0,0,dir_none,"Set Pitch for step",&seq_setpitch,{NULL}};
menu mnu_seq_setdir =   {0,0,dir_none,"Set Track Direction",&seq_setdir,{NULL}};
menu mnu_seq_quantise = {0,0,dir_none,"Set Quantization",&seq_quantise,{NULL}};
menu mnu_seq_singlechnl = {0,0,dir_none,"Single Channel View",&seq_singlechnl,{NULL}};
menu mnu_seq_gridview = {0,0,dir_none,"Grid View",&seq_gridview,{NULL}};

menu mnu_config_setzero = {0,0,dir_left,"Set Zero",&config_setzero,{NULL}};
menu mnu_config_set10v = {0,0,dir_left,"Set 10 Volt",&config_setten,{NULL}};
menu mnu_config_debug = {0,0,dir_left,"Debug on/off",&config_debug,{NULL}};
menu mnu_config_tune = {0,0,dir_left,"Tuning on/off",&config_tune,{NULL}};

menu mnu_test_scalevalue = {0,0,dir_left,"Test scale value",&test_scalevalue,{NULL}};
menu mnu_test_keyboard = {0,0,dir_left,"Test Keyboard",&test_keyboard,{NULL}};

menu sub_end = {0,0,dir_none,NULL,NULL,{NULL}}; //set of NULLs to mark the end of a sub menu

menu Menu[]={
	{0,1,dir_down,"File",NULL,{&mnu_file_open,&mnu_file_save,&mnu_file_saveas,&mnu_file_new,&mnu_file_quit,&sub_end}},
	{0,0,dir_down,"Sequence",NULL,{&mnu_seq_setslew,&mnu_seq_setloop,&mnu_seq_setpitch,&mnu_seq_setdir,&mnu_seq_quantise,&mnu_seq_gridview,&mnu_seq_singlechnl,&mnu_seq_new,&sub_end}},
	{0,0,dir_down,"Config",NULL,{&mnu_config_setzero,&mnu_config_set10v,&mnu_config_debug,&mnu_config_tune,&sub_end}},
	{0,0,dir_down,"Test",NULL,{&mnu_test_scalevalue,&mnu_config_setzero,&mnu_test_keyboard,&sub_end}},
	{0,0,dir_down,"Play",NULL,{&sub_end}},
	{0,0,dir_down,NULL,NULL,{NULL}}
	};


/* This is the main structure that holds info about the running sequence */
struct europi Europi; 
struct europi_hw Europi_hw;
enum display_page_t DisplayPage = GridView;
uint32_t ActiveOverlays;

#define MAX_GESTURE_STRINGS   20

// application entry point 
int main(int argc, char* argv[])
{
	unsigned int iseed = (unsigned int)time(NULL);
	srand (iseed);
	/* things to do when prog first starts */
	impersonate_hw =  FALSE; //TRUE;	/* !!! uncomment this line if testing software without full hardware present */
	startup();
	/* Read and set the states of the run/stop and int/ext switches */
	log_msg("Run/stop: %d, Int/ext: %d\n",gpioRead(RUNSTOP_IN),gpioRead(INTEXT_IN));
	run_stop = gpioRead(RUNSTOP_IN);
	clock_source = gpioRead(INTEXT_IN);
	//Temp for testing
	//run_stop = RUN; //STOP; 
	//clock_source = INT_CLK;
    //debug = TRUE;
    currentGesture = GESTURE_NONE;
    lastGesture = GESTURE_NONE;
    //SetGesturesEnabled(0b0000000011100011);   //None, tap & DoubleTap 

    SetTargetFPS(60);		// Seems this is necessary, or GESTURE_TAP & SWIPE aren't detected!!
 
while (!WindowShouldClose() && (prog_running ==1)) {
	
    lastGesture = currentGesture;
    currentGesture = GetGestureDetected();
    touchPosition = GetTouchPosition(0);
/*
	if (currentGesture != lastGesture)
		{

		  switch (currentGesture)
			{
				case GESTURE_TAP: log_msg("GESTURE TAP"); break;
				case GESTURE_DOUBLETAP: log_msg( "GESTURE DOUBLETAP"); break;
				case GESTURE_HOLD: log_msg("GESTURE HOLD"); break;
				case GESTURE_DRAG: log_msg("GESTURE DRAG"); break;
				case GESTURE_SWIPE_RIGHT: log_msg("GESTURE SWIPE RIGHT"); break;
				case GESTURE_SWIPE_LEFT: log_msg("GESTURE SWIPE LEFT"); break;
				case GESTURE_SWIPE_UP: log_msg("GESTURE SWIPE UP"); break;
				case GESTURE_SWIPE_DOWN: log_msg("GESTURE SWIPE DOWN"); break;
				case GESTURE_PINCH_IN: log_msg("GESTURE PINCH IN"); break;
				case GESTURE_PINCH_OUT: log_msg("GESTURE PINCH OUT"); break;
				default: break;
			}
		}
*/
    switch(DisplayPage){
        case GridView:
            gui_8x8();
        break;
        case SingleStep:
            gui_singlestep();
        break;
        case SingleChannel:
            gui_SingleChannel();
        break;
        case SingleAD:
            gui_SingleAD();
        break;
        case SingleADSR:
            gui_SingleADSR();
        break;
    }
 
    usleep(100); 
}
    ThreadEnd = TRUE;
    // Wait for various joinable threads to end
	pthread_join(touchThreadId, NULL);
    int i;
    for(i=0;i<4;i++) {
        if(midiThreadLaunched[i] == TRUE) pthread_join(midiThreadId[i], NULL);
    }
	shutdown();
	return 0;
  
}


