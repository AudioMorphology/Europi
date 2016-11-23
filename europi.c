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
//int current_step = 0;
int step_one = TRUE;	/* used for resetting the sequence to start from the first step */
int step_one_state = LOW; /*records whether the Step 1 pulse needs to be turned off */
int selected_step = -1;	/* records the step that is currently selected. -1=Nothing selected */
//int last_step = 32;		/* last step in the sequence - can be changed to shorten the loop point */
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
enum encoder_focus_t encoder_focus;
uint32_t encoder_tick;
pthread_attr_t detached_attr;		/* Single detached thread attribute used by any /all detached threads */
pthread_mutex_t mcp23008_lock;
pthread_mutex_t pcf8574_lock;
uint8_t mcp23008_state[16];
/* Raylib-related stuff */
SpriteFont fonts[1];

/* This is the main structure that holds info about the running sequence */
struct europi Europi; 

int test_v = 2000;

/* sequence array:
 * Holds details of our Sequence [CHNL][STEP][PARAMETER]
 * [CHNL] = Channel 0-5
 * [STEP] = Sequence step 0-31
 * where the parameters are as follows:
 * [0] = cv value
 * [1] = gate (0=off, 1=gate, 2=trigger)
 * [2] = active (1 = step Active, 0 = step inactive)
 */
unsigned int sequence[6][32][3];	
unsigned int button_grid[32][1][1];	/* X & Y coordinates of Top Left of each button in the grid */

char *kbfds = "/dev/tty";


// application entry point
int main(int argc, char* argv[])
{
	unsigned int iseed = (unsigned int)time(NULL);			//Seed srand() 
	srand (iseed);
	/* things to do when prog first starts */
	startup();
	/* draw the main front screen */
	//paint_main();
	/* scribble some text */
	//put_string(fbp, 20, 215, "Menu       ", HEX2565(0x000000), HEX2565(0xFFFFFF));
	/* Read and set the states of the run/stop and int/ext switches */
	log_msg("Run/stop: %d, Int/ext: %d\n",gpioRead(RUNSTOP_IN),gpioRead(INTEXT_IN));
	run_stop = gpioRead(RUNSTOP_IN);
	clock_source = gpioRead(INTEXT_IN);
	//Temp for testing
	//run_stop = RUN; 
	//clock_source = INT_CLK;

    Vector2 ballPosition = { -100.0f, -100.0f };
    Color ballColor = DARKBLUE;

while (prog_running == 1){
        // Update
        //----------------------------------------------------------------------------------
        ballPosition = GetMousePosition();
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) ballColor = MAROON;
        else if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) ballColor = LIME;
        else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) ballColor = DARKBLUE;
        //----------------------------------------------------------------------------------
        // Draw 
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);
			int track;
			int step;
			for(track=0;track<24;track++){
				for(step=0;step<32;step++){
					if(step == Europi.tracks[track].last_step){
						// Paint last step
						DrawRectangle(step * 10, track * 10, 9, 9, BLACK); 
					}
					else if(step == Europi.tracks[track].current_step){
						// Paint current step
						DrawRectangle(step * 10, track * 10, 9, 9, LIME); 
						
					}
					else {
						// paint blank step
						DrawRectangle(step * 10, track * 10, 9, 9, MAROON); 
					}
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
