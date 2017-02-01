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
#include "../raylib/release/rpi/raylib.h"

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
char debug_txt[100];	/* buffer for debug text messages */
char input_txt[100];    /* buffer for capturing user input */
int prog_running=0;		/* Setting this to 0 will force the prog to quit*/
int run_stop=STOP;		/* 0=Stop 1=Run: Halts the main step generator */
int clock_counter = 95;	/* Main clock counter, tracks the 96 pulses per step */
int clock_level = 0;	/* Master clock phase */
int clock_source = INT_CLK;	/* INT_CLK = Internal, EXT_CLK = External Clock Source */
int clock_freq=192;		/* speed of the main internal clock in Hz */
uint8_t PCF8574_state=0xF0; /* current state of the PCF8574 Ports on the Europi */
int led_on = 0;
int step_one = TRUE;	/* used for resetting the sequence to start from the first step */
int step_one_state = LOW; /*records whether the Step 1 pulse needs to be turned off */
int selected_step = -1;	/* records the step that is currently selected. -1=Nothing selected */
uint32_t step_tick = 0;	/* used to record the start point of each step in ticks */
uint32_t step_ticks = 250000;	/* Records the length of each step in ticks (used to limit slew length) Init value of 250000 is so it doesn't go nuts */
uint32_t slew_interval = 1000; /* number of microseconds between each sucessive level change during a slew */
/* global variables used by the touchscreen interface */
Vector2 touchPosition = { 0, 0 };
int currentGesture1;
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
int sample = 0;
int touched = 0;
int encoder_level_A;
int encoder_level_B;
int encoder_lastGpio;
int encoder_vel;
int disp_menu = 0;
int touchStream = -1;                    // Touch device file descriptor
int touchReady = FALSE;                 // Flag to know if touchscreen is present
pthread_t touchThreadId;                 


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
                        {" "," ","z","x","c","v","b","n","m",",","]"}};
int kbd_char_selected = 0;

/* Raylib-related stuff */
SpriteFont font1;
Texture2D Splash;	        // Splash screen texture
Texture2D KeyboardTexture;  // Keyboard Overlay texture
Texture2D DialogTexture;    // Common dialog control
Texture2D TextInputTexture; // text input dialog box
Texture2D TopBarTexture;    // Top (menu) bar texture
Texture2D MainScreenTexture; // Main screen
Texture2D ButtonBarTexture; // Soft-button graphic bar

/* declare and populate the menu structure */ 
menu mnu_file_open = 	{0,0,dir_none,"Open",&file_open,NULL};
menu mnu_file_save = 	{0,0,dir_none,"Save",&file_save,NULL};
menu mnu_file_saveas = 	{0,0,dir_none,"Save As",&file_saveas,NULL};
menu mnu_file_new = 	{0,0,dir_none,"New",NULL,NULL};
menu mnu_file_quit = 	{0,0,dir_none,"Quit",&file_quit,NULL};

menu mnu_seq_new = 		{0,0,dir_none,"New",&seq_new,NULL};
menu mnu_seq_setloop =	{0,0,dir_none,"Set Loop Points",&seq_setloop,NULL};
menu mnu_seq_setpitch = {0,0,dir_none,"Set pitch for step",&seq_setpitch,NULL};
menu mnu_seq_quantise = {0,0,dir_none,"Set Quantization",&seq_quantise,NULL};
menu mnu_seq_singlechnl = {0,0,dir_none,"Single Channel View",&seq_singlechnl,NULL};
menu mnu_seq_gridview = {0,0,dir_none,"Grid View",&seq_gridview,NULL};

menu mnu_config_setzero = {0,0,dir_left,"Set Zero",&config_setzero,NULL};
menu mnu_config_set10v = {0,0,dir_left,"Set 10 Volt",&config_setten,NULL};

menu mnu_test_scalevalue = {0,0,dir_left,"Test scale value",&test_scalevalue,NULL};
menu mnu_test_keyboard = {0,0,dir_left,"Test Keyboard",&test_keyboard,NULL};

menu sub_end = {0,0,dir_none,NULL,NULL,NULL}; //set of NULLs to mark the end of a sub menu

menu Menu[]={
	{0,1,dir_down,"File",NULL,{&mnu_file_open,&mnu_file_save,&mnu_file_saveas,&mnu_file_new,&mnu_file_quit,&sub_end}},
	{0,0,dir_down,"Sequence",NULL,{&mnu_seq_setloop,&mnu_seq_setpitch,&mnu_seq_quantise,&mnu_seq_gridview,&mnu_seq_singlechnl,&mnu_seq_new,&sub_end}},
	{0,0,dir_down,"Config",NULL,{&mnu_config_setzero,&mnu_config_set10v,&sub_end}},
	{0,0,dir_down,"Test",NULL,{&mnu_test_scalevalue,&mnu_config_setzero,&mnu_test_keyboard,&sub_end}},
	{0,0,dir_down,"Play",NULL,{&sub_end}},
	{0,0,dir_down,NULL,NULL,NULL}
	};


/* This is the main structure that holds info about the running sequence */
struct europi Europi; 
struct hardware Hardware;
enum display_page_t DisplayPage = GridView;
struct screen_overlays ScreenOverlays;


// application entry point 
int main(int argc, char* argv[])
{
	unsigned int iseed = (unsigned int)time(NULL);
	srand (iseed);
	/* things to do when prog first starts */
	impersonate_hw = FALSE; //TRUE;	/* !!! uncomment this line if testing software without full hardware present */
	startup();
	/* Read and set the states of the run/stop and int/ext switches */
	log_msg("Run/stop: %d, Int/ext: %d\n",gpioRead(RUNSTOP_IN),gpioRead(INTEXT_IN));
	run_stop = gpioRead(RUNSTOP_IN);
	clock_source = gpioRead(INTEXT_IN);
	//Temp for testing
	//run_stop = STOP; 
	//clock_source = INT_CLK;
    
    currentGesture1 = GESTURE_NONE;
    lastGesture = GESTURE_NONE;
    //SetGesturesEnabled(0b0000000011100011);   //None, tap & DoubleTap 
 
while (prog_running == 1){

    lastGesture = currentGesture1;
    currentGesture1 = GetGestureDetected();
    touchPosition = GetTouchPosition(0);
    switch(DisplayPage){
        case GridView:
            gui_8x8();
        break;
        case SingleChannel:
            gui_SingleChannel();
        break;
    }
 
/*
    if(ScreenElements.SetZero == 1){
        int track = 0;
        char str[80];
        for(track = 0; track < MAX_TRACKS; track++) {
            if (Europi.tracks[track].selected == TRUE){
                if(encoder_focus == track_select){
                    sprintf(str,"Track: [%02d] Value for Zero output: %05d\0",track+1,Europi.tracks[track].channels[CV_OUT].scale_zero);
                }
                else if (encoder_focus == set_zerolevel) {
                    sprintf(str,"Track: %02d Value for Zero output: [%05d]\0",track+1,Europi.tracks[track].channels[CV_OUT].scale_zero);
                }
                DACSingleChannelWrite(Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].scale_zero);
                DrawRectangle(20, 30, 250, 20, BLACK);
                DrawText(str,25,35,10,WHITE);
                
            }
        }
    }
    if(ScreenElements.SetTen == 1){
        int track = 0;
        char str[80];
        for(track = 0; track < MAX_TRACKS; track++) {
            if (Europi.tracks[track].selected == TRUE){
                if (encoder_focus == track_select) {
                    sprintf(str,"Track: [%02d] Value for 10v output: %05d\0",track+1,Europi.tracks[track].channels[CV_OUT].scale_max);
                }
                else if (encoder_focus == set_maxlevel) {
                    sprintf(str,"Track: %02d Value for 10v output: [%05d]\0",track+1,Europi.tracks[track].channels[CV_OUT].scale_max);
                }
                DACSingleChannelWrite(Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].scale_max);
                DrawRectangle(20, 30, 250, 20, BLACK);
                DrawText(str,25,35,10,WHITE);
                
            }
        }
    }
    if(ScreenElements.ScaleValue == 1){
        int track = 0;
        char str[80];
        char str1[80];
        for(track = 0; track < MAX_TRACKS; track++) {
            if (Europi.tracks[track].selected == TRUE){
                if (encoder_focus == track_select) {
                    sprintf(str,"Trk: [%02d] Zero: %05d Max: %05d\0",track+1,Europi.tracks[track].channels[CV_OUT].scale_zero,Europi.tracks[track].channels[CV_OUT].scale_max);
                    sprintf(str1,"Raw: %05d Scaled: %05d\0",Europi.tracks[track].channels[CV_OUT].steps[12].raw_value,scale_value(track,Europi.tracks[track].channels[CV_OUT].steps[12].raw_value));
                }
                else if (encoder_focus == set_maxlevel) {
                    sprintf(str,"Track: %02d Value for 10v output: [%05d]\0",track+1,Europi.tracks[track].channels[CV_OUT].scale_max);
                }
                DACSingleChannelWrite(Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].scale_max);
                DrawRectangle(20, 30, 250, 40, BLACK);
                DrawText(str,25,35,10,WHITE);
                DrawText(str1,25,55,10,WHITE);
                
            }
        }
    }
     */
    if(debug == TRUE){
        DrawRectangle(0,200,320,20,BLACK);
        DrawText(debug_txt,5,205,10,WHITE);
    }
    
    if (currentGesture1 != GESTURE_NONE){ 
        DrawCircleV(touchPosition, 2, BLACK);
    }  
    //UpdateGestures();
    usleep(100); 
}

	pthread_join(touchThreadId, NULL);
	shutdown();
	return 0;
  
}


