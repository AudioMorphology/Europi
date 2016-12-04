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
menu mnu_file_save = 	{0,0,"Save",NULL,NULL};
menu mnu_file_saveas = 	{0,0,"Save As",NULL,NULL};
menu mnu_file_new = 	{0,0,"New",NULL,NULL};
menu mnu_file_quit = 	{0,0,"Quit",&file_quit,NULL};

menu mnu_config_setzero = {0,0,"Set Zero",&config_setzero,NULL};
menu mnu_config_set10v = {0,0,"Set 10 Volt",NULL,NULL};

menu sub_end = {0,0,NULL,NULL,NULL}; //set of NULLs to mark the end of a sub menu

menu Menu[]={
	{0,1,"File",NULL,{&mnu_file_open,&mnu_file_save,&mnu_file_saveas,&mnu_file_new,&mnu_file_quit,&sub_end}},
	{0,0,"Config",NULL,{&mnu_config_setzero,&mnu_config_set10v,&sub_end}},
	{0,0,"Play",NULL,{&sub_end}},
	{0,0,"A long menu",NULL,{&sub_end}},
	{0,0,NULL,NULL,NULL}
	};


/* This is the main structure that holds info about the running sequence */
struct europi Europi; 



// application entry point
int main(int argc, char* argv[])
{
	unsigned int iseed = (unsigned int)time(NULL);			//Seed srand() 
	srand (iseed);
	/* things to do when prog first starts */
	startup();
	/* draw the main front screen */
	//paint_main();
	/* Read and set the states of the run/stop and int/ext switches */
	log_msg("Run/stop: %d, Int/ext: %d\n",gpioRead(RUNSTOP_IN),gpioRead(INTEXT_IN));
	run_stop = gpioRead(RUNSTOP_IN);
	clock_source = gpioRead(INTEXT_IN);
	//Temp for testing
	//run_stop = RUN; 
	//clock_source = INT_CLK;

while (prog_running == 1){
        //----------------------------------------------------------------------------------
        // Draw 
        //----------------------------------------------------------------------------------
        BeginDrawing();

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
			// Draw the menu
			if(disp_menu == 1){
				int i = 0;
				int x = 5;
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
        EndDrawing(); 
        //----------------------------------------------------------------------------------
	
	/* check for touchscreen input */
	if (touched == 1){
	int x;
	touched = 0; 
	getTouchSample(&rawX, &rawY, &rawPressure); 
	Xsamples[sample] = rawX;
	Ysamples[sample] = rawY;
	if (sample++ >= SAMPLE_AMOUNT){
		sample = 0;
		Xaverage  = 0;
		Yaverage  = 0;

		for ( x = 0; x < SAMPLE_AMOUNT; x++ ){
			Xaverage += Xsamples[x];
			Yaverage += Ysamples[x];
		}

	Xaverage = Xaverage/SAMPLE_AMOUNT;
	Yaverage = Yaverage/SAMPLE_AMOUNT;
	/* scale these values by the known min & max raw values (obtained by
	 * experimentation, and noted in europi.h
	*/ 
	if (Xaverage <= rawXmin) Xaverage = rawXmin;
	if (Xaverage >= rawXmax) Xaverage = rawXmax;
	if (Yaverage <= rawYmin) Yaverage = rawYmin;
	if (Yaverage >= rawYmax) Yaverage = rawYmax;
	/* Er., X is upside down */
	scaledX = ((float)(Xaverage - rawXmin) / (float)(rawXmax-rawXmin))*X_MAX;
	scaledY = Y_MAX-((float)(Yaverage - rawYmin) / (float)(rawYmax-rawYmin))*Y_MAX;
	/* see whether one of the buttons has been touched. If so, select it */
	button_touched(scaledX,scaledY);

	}

	
	}
    usleep(100);
}

	shutdown();
	return 0;
  
}
