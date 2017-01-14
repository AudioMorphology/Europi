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
int debug = TRUE;		/* controls whether debug messages are printed to the main screen */
int impersonate_hw = FALSE;	/* allows the software to bypass hardware checks (useful when testing sw without full hw ) */ 
char debug_txt[320];	/* buffer for debug text messages */
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
int CalibCross;			/* which calibration cross we're displaying */
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
uint32_t encoder_tick;
pthread_attr_t detached_attr;		/* Single detached thread attribute used by any /all detached threads */
pthread_mutex_t mcp23008_lock;
pthread_mutex_t pcf8574_lock;
uint8_t mcp23008_state[16];
char *kbfds = "/dev/tty";

/* Raylib-related stuff */
SpriteFont font1;
Texture2D Splash;	// Splash screen texture

/* declare and populate the menu structure */ 
menu mnu_file_open = 	{0,0,"Open",NULL,NULL};
menu mnu_file_save = 	{0,0,"Save",&file_save,NULL};
menu mnu_file_saveas = 	{0,0,"Save As",NULL,NULL};
menu mnu_file_new = 	{0,0,"New",NULL,NULL};
menu mnu_file_quit = 	{0,0,"Quit",&file_quit,NULL};

menu mnu_seq_new = 		{0,0,"New",&seq_new,NULL};
menu mnu_seq_setloop =	{0,0,"Set Loop Points",&seq_setloop,NULL};
menu mnu_seq_setpitch = {0,0,"Set pitch for step",&seq_setpitch,NULL};
menu mnu_seq_quantise = {0,0,"Set Quantization",&seq_quantise,NULL};
menu mnu_seq_singlechnl = {0,0,"Single Channel View",&seq_singlechnl,NULL};

menu mnu_config_setzero = {0,0,"Set Zero",&config_setzero,NULL};
menu mnu_config_set10v = {0,0,"Set 10 Volt",&config_setten,NULL};
menu mnu_config_calibtouch = {0,0,"Calibrate Touchscreen",&config_calibtouch,NULL};

menu mnu_test_scalevalue = {0,0,"Test scale value",&test_scalevalue,NULL};

menu sub_end = {0,0,NULL,NULL,NULL}; //set of NULLs to mark the end of a sub menu

menu Menu[]={
	{0,1,"File",NULL,{&mnu_file_open,&mnu_file_save,&mnu_file_saveas,&mnu_file_new,&mnu_file_quit,&sub_end}},
	{0,0,"Sequence",NULL,{&mnu_seq_new,&mnu_seq_setloop,&mnu_seq_setpitch,&mnu_seq_quantise,&mnu_seq_singlechnl,&sub_end}},
	{0,0,"Config",NULL,{&mnu_config_setzero,&mnu_config_set10v,&mnu_config_calibtouch,&sub_end}},
	{0,0,"Test",NULL,{&mnu_test_scalevalue,&mnu_config_setzero,&sub_end}},
	{0,0,"Play",NULL,{&sub_end}},
	{0,0,NULL,NULL,NULL}
	};


/* This is the main structure that holds info about the running sequence */
struct europi Europi; 
struct hardware Hardware;
struct screen_elements ScreenElements;


// application entry point
int main(int argc, char* argv[])
{
	unsigned int iseed = (unsigned int)time(NULL);
	srand (iseed);
	/* things to do when prog first starts */
	impersonate_hw = TRUE;	/* !!! uncomment this line if testing software without full hardware present */
	startup();
	/* Read and set the states of the run/stop and int/ext switches */
	log_msg("Run/stop: %d, Int/ext: %d\n",gpioRead(RUNSTOP_IN),gpioRead(INTEXT_IN));
	run_stop = gpioRead(RUNSTOP_IN);
	clock_source = gpioRead(INTEXT_IN);
	//Temp for testing
	//run_stop = RUN; 
	//clock_source = INT_CLK;
	int currentGesture = GESTURE_NONE;
	int lastGesture = GESTURE_NONE;
	Vector2 ballPosition = { -100.0f, -100.0f };
    Color ballColor = DARKBLUE;

	Vector2 touchPosition = { 0, 0 };

while (prog_running == 1){
	
        ballPosition = GetMousePosition();
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) ballColor = MAROON; 
        else if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) ballColor = LIME;
        else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) ballColor = DARKBLUE;	
	
		currentGesture = GetGestureDetected();
        touchPosition = GetTouchPosition(0);	
		if (currentGesture != lastGesture)
            {
/*                // Store gesture string
                switch (currentGesture)
                { 
                    case GESTURE_TAP: log_msg("GESTURE TAP\n"); break;
                    case GESTURE_DOUBLETAP: log_msg("GESTURE DOUBLETAP\n"); break;
                    case GESTURE_HOLD: log_msg("GESTURE HOLD\n"); break;
                    case GESTURE_DRAG: log_msg("GESTURE DRAG\n"); break;
                    case GESTURE_SWIPE_RIGHT: log_msg("GESTURE SWIPE RIGHT\n"); break;
                    case GESTURE_SWIPE_LEFT: log_msg("GESTURE SWIPE LEFT\n"); break;
                    case GESTURE_SWIPE_UP: log_msg("GESTURE SWIPE UP\n"); break;
                    case GESTURE_SWIPE_DOWN: log_msg("GESTURE SWIPE DOWN\n"); break;
                    case GESTURE_PINCH_IN: log_msg("GESTURE PINCH IN\n"); break;
                    case GESTURE_PINCH_OUT: log_msg("GESTURE PINCH OUT\n"); break;
                    default: break;
                }
  */          }
        //----------------------------------------------------------------------------------
        // Draw 
        //----------------------------------------------------------------------------------
        BeginDrawing();
			if(ScreenElements.GridView == 1){
				ClearBackground(RAYWHITE);
				int track;
				int step;
				char track_no[20];
				int txt_len;
				for(track=0;track<24;track++){
					// Track Number
					sprintf(track_no,"%d",track+1);
					txt_len = MeasureText(track_no,10);
					DrawText(track_no,12-txt_len,track * 10,10,DARKGRAY);
					for(step=0;step<32;step++){
						if(step == Europi.tracks[track].last_step){
							// Paint last step
							DrawRectangle(15 + (step * 9), track * 10, 8, 9, BLACK); 
						}
						else if(step == Europi.tracks[track].current_step){
							// Paint current step
							DrawRectangle(15 + (step * 9), track * 10, 8, 9, LIME); 
							// Gate state for current step
							if (Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].gate_value == 1){
								if (Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].retrigger > 0) {
									DrawRectangle(15 + (32 * 9), track * 10, 8, 9, BLACK);	
								}
								else {
									DrawRectangle(15 + (32 * 9), track * 10, 8, 9, VIOLET);	
								}	
							}
							else {
								DrawRectangle(15 + (32 * 9), track * 10, 8, 9, WHITE);	
							}
						}
						else {
							// paint blank step
							DrawRectangle(15 + (step * 9), track * 10, 8, 9, MAROON); 
						}
					}
				}
			}	
			// Display a Single Channel
			if(ScreenElements.SingleChannel == 1){
				ClearBackground(RAYWHITE);
				int track;
				int step;
				int val;
				char track_no[20];
				int txt_len;
				for (track = 0; track < MAX_TRACKS; track++){
					if (Europi.tracks[track].selected == TRUE){
						sprintf(track_no,"%d",track+1);
						txt_len = MeasureText(track_no,10);
						DrawText(track_no,12-txt_len,220,10,DARKGRAY);
						for (step = 0; step < MAX_STEPS; step++){
							if(step < Europi.tracks[track].last_step){
								val = (int)(((float)Europi.tracks[track].channels[CV_OUT].steps[step].scaled_value / (float)60000) * 220);
								if(step == Europi.tracks[track].current_step){
									DrawRectangle(15 + (step*9),220-val,8,val,MAROON);
								}
								else{
									DrawRectangle(15 + (step*9),220-val,8,val,LIME);
								}
								// Gate State
								if (Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].gate_value == 1){
									sprintf(track_no,"%d",Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger);
									DrawText(track_no,15 + (step*9),220,10,DARKGRAY);
								}
								// Slew
								if (Europi.tracks[track].channels[CV_OUT].steps[step].slew_type != Off){
									switch (Europi.tracks[track].channels[CV_OUT].steps[step].slew_shape){
										case Both:
											DrawText("V",15 + (step*9),230,10,DARKGRAY);
										break;
										case Rising:
											DrawText("/",15 + (step*9),230,10,DARKGRAY);
										break;
										case Falling:
											DrawText("\\",15 + (step*9),230,10,DARKGRAY);
										break;
									}
								}
							}
						}
					}
				}

			}
			// Draw the menu
			if(ScreenElements.MainMenu == 1){
				int i = 0;
				int x = 5;
				int txt_len;
				Color menu_colour;
				while(Menu[i].name != NULL){
					txt_len = MeasureText(Menu[i].name,10);
					//Draw a box a bit bigger than this
					if (Menu[i].highlight == 1) menu_colour = BLUE; else menu_colour = BLACK;
					DrawRectangle(x,0,txt_len+6,12,menu_colour);
					DrawText(Menu[i].name,x+3,1,10,WHITE);
					if(Menu[i].expanded == 1){
						// Draw sub-menus
						int j = 0;
						int y = 12;
						int sub_len = 0;
						int tmp_len;
						// Measure the length of the longest sub menu
						while(Menu[i].child[j]->name != NULL){
							tmp_len = MeasureText(Menu[i].child[j]->name,10);
							if(tmp_len > sub_len) sub_len = tmp_len;
							j++;
						}
						j = 0;
						while(Menu[i].child[j]->name != NULL){
							if (Menu[i].child[j]->highlight == 1) menu_colour = BLUE; else menu_colour = BLACK;
							DrawRectangle(x,y,sub_len+6,12,menu_colour);
							DrawText(Menu[i].child[j]->name,x+3,y+1,10,WHITE);
							y+=12;
							j++;
						}
					}
					x+=txt_len+6+2;
					i++;
				}
			}
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
			if(ScreenElements.CalibTouch == 1){
				DrawText("Touch each Cross as it appears",50,110,10,DARKGRAY);
				switch(CalibCross){
					case 0:
						DrawRectangle(10,5,1,11, BLACK);
						DrawRectangle(5,10,11,1,BLACK);
						touchPosition = GetTouchPosition(0);
						CalibCross = 1;
					break;
					case 1:
						DrawRectangle(310,5,1,11, BLACK);
						DrawRectangle(305,10,11,1,BLACK);
					break;
					case 2:
						DrawRectangle(10,225,1,11, BLACK);
						DrawRectangle(5,230,11,1,BLACK);
					break;
					case 3:
						DrawRectangle(310,225,1,11, BLACK);
						DrawRectangle(305,230,11,1,BLACK);
					
					break;
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
			if(ScreenElements.SetLoop == 1){
				int track = 0;
				char str[80];
				for(track = 0; track < MAX_TRACKS; track++) {
					if (Europi.tracks[track].selected == TRUE){
						if(encoder_focus == track_select){
							sprintf(str,"Track: [%02d] Loop Point: %02d\0",track+1,Europi.tracks[track].last_step);
						}
						else if (encoder_focus == set_loop) {
							sprintf(str,"Track: %02d Loop Point: [%02d]\0",track+1,Europi.tracks[track].last_step);
						}
						DrawRectangle(20, 30, 250, 20, BLACK);
						DrawText(str,25,35,10,WHITE);
					}
				}
			}
			if(ScreenElements.SetPitch == 1){
				int track = 0;
				char str[80];
				for(track = 0; track < MAX_TRACKS; track++) {
					if (Europi.tracks[track].selected == TRUE){
						if(encoder_focus == track_select){
							sprintf(str,"Track: [%02d] Step: %02d Pitch: %05d\0",track+1,Europi.tracks[track].current_step+1,Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value);
						}
						else if (encoder_focus == step_select) {
							sprintf(str,"Track: %02d Step: [%02d] Pitch: %05d\0",track+1,Europi.tracks[track].current_step+1,Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value);
						}
						else if (encoder_focus == set_pitch){
							sprintf(str,"Track: %02d Step: %02d Pitch: [%05d]\0",track+1,Europi.tracks[track].current_step+1,Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value);
						}
						DrawRectangle(20, 30, 250, 20, BLACK);
						DrawText(str,25,35,10,WHITE);
					}
				}
			}
			if(ScreenElements.SetQuantise == 1){
				int track = 0;
				char str[80];
				for(track = 0; track < MAX_TRACKS; track++) {
					if (Europi.tracks[track].selected == TRUE){
						if(encoder_focus == track_select){
							sprintf(str,"Track: [%02d] Quantisation: %s\0",track+1,scale_names[Europi.tracks[track].channels[CV_OUT].quantise]);
						}
						else if (encoder_focus == set_quantise) {
							sprintf(str,"Track: %02d Quantisation: [%s]\0",track+1,scale_names[Europi.tracks[track].channels[CV_OUT].quantise]);
						}
						DrawRectangle(20, 30, 250, 20, BLACK);
						DrawText(str,25,35,10,WHITE);
					}
				}
			}
			
			DrawCircleV(ballPosition, 5, ballColor);			
			if(debug == TRUE){
				DrawRectangle(0,200,320,20,BLACK);
				DrawText(debug_txt,5,205,10,WHITE);
			}
			
			if (currentGesture != GESTURE_NONE){ 
                log_msg("Ball: %d,%d Touch: %d,%d\n",ballPosition.x,ballPosition.y,touchPosition.x,touchPosition.y);
                DrawCircleV(touchPosition, 10, BLUE);
            }        
            EndDrawing(); 
        //----------------------------------------------------------------------------------

    usleep(100);
}

	pthread_join(touchThreadId, NULL);
	shutdown();
	return 0;
  
}


