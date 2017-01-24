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

#include <linux/input.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <linux/kd.h>
#include <pigpio.h>
#include <signal.h>

#include "europi.h"
//#include "front_panel.c"
//#include "touch.h"
//#include "touch.c"
//#include "quantizer_scales.h"
#include "../raylib/release/rpi/raylib.h"

//extern struct europi;
extern struct fb_var_screeninfo vinfo;
extern struct fb_fix_screeninfo finfo;
extern struct fb_var_screeninfo orig_vinfo;

extern unsigned hw_version;
extern int fbfd;
extern char *fbp;
extern char *kbfds;
extern int impersonate_hw;
extern int debug;
extern char debug_txt[];
extern int kbfd;
extern int prog_running;
extern int run_stop;
extern int is_europi;
extern int retrig_counter;
extern int extclk_counter;
extern int extclk_level;
extern int clock_counter;
extern int clock_level;
extern int clock_freq;
extern int clock_source;
extern uint8_t PCF8574_state;
extern int led_on;
extern int print_messages;
//extern unsigned int sequence[6][32][3];
//extern int current_step;
//extern int last_step;
extern int selected_step;
extern int step_one;
extern int step_one_state;
extern uint32_t step_tick;
extern uint32_t step_ticks;
extern uint32_t slew_interval;
/* globals used by touchscreen interface */
extern int  xres,yres,x;
extern int Xsamples[20];
extern int Ysamples[20];
extern int screenXmax, screenXmin;
extern int screenYmax, screenYmin;
extern float scaleXvalue, scaleYvalue;
extern int rawX, rawY, rawPressure, scaledX, scaledY;
extern int Xaverage;
extern int Yaverage;
extern int sample;
extern int touched;
extern int encoder_level_A;
extern int encoder_level_B;
extern int encoder_lastGpio;
extern int encoder_pos;
extern int encoder_vel;
extern uint32_t encoder_tick;
extern enum encoder_focus_t encoder_focus;
extern struct europi Europi;
extern enum display_page_t DisplayPage;
extern struct screen_overlays ScreenOverlays;
extern struct MENU Menu[];
extern pthread_attr_t detached_attr;		
extern pthread_mutex_t mcp23008_lock;
extern pthread_mutex_t pcf8574_lock;
extern uint8_t mcp23008_state[16];
extern int test_v;
pthread_t ThreadId; 		// Pointer to detatched Thread Ids (re-used by each/every detatched thread)
extern SpriteFont font1;
extern Texture2D Splash;
extern int disp_menu;	

/* Internal Clock
 * 
 * This is the main timing loop for the 
 * whole programme, and which runs continuously
 * from program start to end.
 * 
 * The Master Clock runs at 96 * The BPM step
 * frequency
 */
void master_clock(int gpio, int level, uint32_t tick)
{
	if ((run_stop == RUN) && (clock_source == INT_CLK)) {
		if (clock_counter++ > 95) {
			clock_counter = 0;
			GATESingleOutput(Europi.tracks[0].channels[GATE_OUT].i2c_handle,CLOCK_OUT,DEV_PCF8574,HIGH);
			next_step();
		}
		if (clock_counter == 48) GATESingleOutput(Europi.tracks[0].channels[GATE_OUT].i2c_handle,CLOCK_OUT,DEV_PCF8574,LOW);
	}
}

/* External clock - triggered by rising edge of External Clock input
 * If we are clocking from external clock, then it is this function
 * that advances the sequence to the next step
 */
void external_clock(int gpio, int level, uint32_t tick)
{
	if ((run_stop == RUN) && (clock_source == EXT_CLK)) {
		// Copy the external clock to the Clock Out port
		GATESingleOutput(Europi.tracks[0].channels[GATE_OUT].i2c_handle,CLOCK_OUT,DEV_PCF8574,level);
		if (level == 1) next_step();
	}
}

/* run/stop switch input
 * called whenever the switch changes state
 */
void runstop_input(int gpio, int level, uint32_t tick)
{
	run_stop = level;
}
/* int/ext clock source switch input
 * called whenever the switch changes state
 */
void clocksource_input(int gpio, int level, uint32_t tick)
{
	clock_source = level;
}
/* Reset input
 * Rising edge causes next step to be Step 1
 */
void reset_input(int gpio, int level, uint32_t tick)
{
	if (level == 1)	step_one = TRUE;
}
/* Function called to advance the sequence on to the next step */
void next_step(void)
{
	/* Note the time and length of the previous step
	 * this gives us a running approximation of how
	 * fast the sequence is running, and can be used to 
	 * prevent slews overrunning too badly, time triggers
	 * etc.
	 */
	uint32_t current_tick = gpioTick();
	// first ever time it's run, there will be
	// no value for step_tick, to the length of
	// the first step will be indeterminate. So,
	// set it to 200ms.
	if (step_tick == 0) step_ticks = 200000; else step_ticks = current_tick - step_tick;
	//log_msg("Step Ticks: %d\n",step_ticks);
	step_tick = current_tick;
	int previous_step, channel, track;
	/* look for something to do */
	//for (track = 0;track < MAX_TRACKS; track++){
	for (track = 0;track < MAX_TRACKS; track++){
		/* if this track is busy doing something else, then it won't advance to the next step */
		if (Europi.tracks[track].track_busy == FALSE){
			/* Each Track has its own end point */
			previous_step = Europi.tracks[track].current_step;
			if (++Europi.tracks[track].current_step >= Europi.tracks[track].last_step || step_one == TRUE){
				Europi.tracks[track].current_step = 0;
				/* IF we've got Europi hardware, trigger the Step 1 pulse as Track 0 passes through Step 0 */
				if ((is_europi == TRUE) && (track == 0)){
					/* Track 0 Channel 1 will have the GPIO Handle for the PCF8574 channel 3 is Step 1 Out*/
					struct gate sGate;
					sGate.track = track;
					sGate.i2c_handle = Europi.tracks[0].channels[GATE_OUT].i2c_handle;
					sGate.i2c_address = Europi.tracks[0].channels[GATE_OUT].i2c_address;
					sGate.i2c_channel = STEP1_OUT;
					sGate.i2c_device = DEV_PCF8574;
					sGate.gate_length = 10000;			/* 10 MS Pulse */
					sGate.gate_type = Trigger;
					sGate.retrigger_count = 1;
					struct gate *pGate = malloc(sizeof(struct gate));
					memcpy(pGate, &sGate, sizeof(struct gate));
					if(pthread_create(&ThreadId, &detached_attr, &GateThread, pGate)){
						log_msg("Gate thread creation error\n");
					}
					
				}
			}
			
			/* set the CV for each channel, BUT ONLY if slew is OFF */
			if ((Europi.tracks[track].channels[CV_OUT].enabled == TRUE ) && (Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type == Off)){
				DACSingleChannelWrite(Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value);
			}
			/* set the Gate State for each channel */
			if (Europi.tracks[track].channels[GATE_OUT].enabled == TRUE ){
				if (Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].gate_value == 1){
					struct gate sGate;
					sGate.track = track;
					sGate.i2c_handle = Europi.tracks[track].channels[GATE_OUT].i2c_handle;
					sGate.i2c_address = Europi.tracks[track].channels[GATE_OUT].i2c_address;
					sGate.i2c_channel =  Europi.tracks[track].channels[GATE_OUT].i2c_channel;
					sGate.i2c_device = Europi.tracks[track].channels[GATE_OUT].i2c_device;
					sGate.retrigger_count = Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].retrigger;
					sGate.gate_length = 10000;			/* 10 MS Pulse */
					sGate.gate_type = Gate;
					struct gate *pGate = malloc(sizeof(struct gate));
					memcpy(pGate, &sGate, sizeof(struct gate));
					if(pthread_create(&ThreadId, &detached_attr, &GateThread, pGate)){
						log_msg("Gate thread creation error\n");
					}
				}
			}
			/* Is there a Slew, AD, ADSR set on this step */
			switch (Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type){
				// launch a thread to handle the slew for this Channel / Step
				// Note that this is created as a Detached, rather than Joinable
				// thread, because otherwise the memory consumed by the thread
				// won't be recovered when the thread exits, leading to memory
				// leaks
				struct slew sSlew;
				struct ad sAD;
				struct adsr sADSR;
				
				case Linear:
				case Logarithmic:
					sAD.track = track;
					sSlew.i2c_handle = Europi.tracks[track].channels[CV_OUT].i2c_handle;
					sSlew.i2c_address = Europi.tracks[track].channels[CV_OUT].i2c_address;
					sSlew.i2c_channel = Europi.tracks[track].channels[CV_OUT].i2c_channel;
					sSlew.start_value = Europi.tracks[track].channels[CV_OUT].steps[previous_step].scaled_value;
					sSlew.end_value = Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value;
					sSlew.slew_length = Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_length;
					sSlew.slew_type = Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type;
					sSlew.slew_shape = Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_shape;
					struct slew *pSlew = malloc(sizeof(struct slew));
					memcpy(pSlew, &sSlew, sizeof(struct slew));
					if(pthread_create(&ThreadId, &detached_attr, &SlewThread, pSlew)){
						log_msg("Slew thread creation error\n");
					}
				break;
				case AD:
					// Attack - Decay ramp
					sAD.track = track;
					sAD.i2c_handle = Europi.tracks[track].channels[CV_OUT].i2c_handle;
					sAD.i2c_address = Europi.tracks[track].channels[CV_OUT].i2c_address;
					sAD.i2c_channel = Europi.tracks[track].channels[CV_OUT].i2c_channel;
					sAD.a_start_value = 0;	
					sAD.a_end_value = 30000;	//5v	
					sAD.a_length = 000;		
					sAD.d_end_value = 0;	
					sAD.d_length = 20000;
					sAD.shot_type = Repeat;
					struct ad *pAD = malloc(sizeof(struct ad));
					memcpy(pAD, &sAD, sizeof(struct ad));
					if(pthread_create(&ThreadId, &detached_attr, &AdThread, pAD)){
						log_msg("Attack-Decay thread creation error\n");
					}
				break;
				case ADSR:
					// Full ADSR ramp profile
					
				break;
			}
		}
	}
	/* anything that needed resetting back to step 1 will have done so */
	if (step_one == TRUE) step_one = FALSE;
}

/*
 * Attack-Decay thread.
 * Simple AD ramp, which can be set to OneShot or Repeat
 */
static void *AdThread(void *arg)
{
	struct ad *pAD = (struct ad *)arg;
	uint16_t this_value;
	uint32_t start_tick = gpioTick();
	int step_size;
	int num_steps;
	int i;
	// don't bother if it's anything other than a 'normal' AD profile
	if((pAD->a_end_value > pAD->a_start_value) && (pAD->d_end_value < pAD->a_end_value)){
		// Set Track Busy flag
		Europi.tracks[pAD->track].track_busy = TRUE;
		// A-ramp
		this_value = pAD->a_start_value;
		DACSingleChannelWrite(pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, this_value);
		if(pAD->a_length >= slew_interval){
			step_size = (pAD->a_end_value - pAD->a_start_value) / (pAD->a_length / slew_interval);
			num_steps = (pAD->a_end_value - pAD->a_start_value) / step_size;
			if (step_size == 0) {step_size = 1; num_steps = (pAD->a_end_value - pAD->a_start_value);}
			for(i = 0;i < num_steps; i++){
				usleep(slew_interval / 2);
				// Rail clamp
				if((this_value += step_size) <= 60000){
					DACSingleChannelWrite(pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, this_value);
				}
				else {
					DACSingleChannelWrite(pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, 60000);
				}
			}
		}
		// D-ramp
		this_value = pAD->a_end_value;
		DACSingleChannelWrite(pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, this_value);
		if(pAD->d_length >= slew_interval){
			step_size = (pAD->a_end_value - pAD->d_end_value) / (pAD->d_length / slew_interval);
			num_steps = (pAD->a_end_value - pAD->d_end_value) / step_size;
			if (step_size == 0) {step_size = 1; num_steps = (pAD->a_end_value - pAD->d_end_value);}
			for(i = 0;i < num_steps; i++){
				usleep(slew_interval / 2);
				// Rail clamp
				if((this_value -= step_size) >= 0){
					DACSingleChannelWrite(pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, this_value);
				}
				else {
					DACSingleChannelWrite(pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, 0);
				}
			}
		}
		DACSingleChannelWrite(pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, pAD->d_end_value);
		// Clear Track Busy flag
		Europi.tracks[2].track_busy = FALSE;
	}	
	free(pAD);
	return(0);
}

/*
 * Slew Thread - launched for each Track / Step that
 * has a slew value other than SLEW_OFF. This thread
 * function lives just to perform the slew, then
 * ends itself
 */
static void *SlewThread(void *arg)
{
	struct slew *pSlew = (struct slew *)arg;
	uint16_t this_value = pSlew->start_value;
	uint32_t start_tick = gpioTick();
	int step_size;
	if (pSlew->slew_length == 0) {
		// No slew length set, so just output this step and close the thread
		DACSingleChannelWrite(pSlew->i2c_handle, pSlew->i2c_address, pSlew->i2c_channel, pSlew->end_value);
		free(pSlew);
		return(0);
	}
	
	if ((pSlew->end_value > pSlew->start_value) && ((pSlew->slew_shape == Rising) || (pSlew->slew_shape == Both))) {
		// Glide UP
		step_size = (pSlew->end_value - pSlew->start_value) / (pSlew->slew_length / slew_interval);
		if (step_size == 0) step_size = 1;
		//log_msg("Step size up: %d\n",step_size);
		while ((this_value <= pSlew->end_value) && (this_value < (60000 - step_size))){
			DACSingleChannelWrite(pSlew->i2c_handle, pSlew->i2c_address, pSlew->i2c_channel, this_value);
			this_value += step_size;
			usleep(slew_interval / 2);		
			if ((gpioTick() - start_tick) > step_ticks) break;	//We've been here too long
		}
	}
	else if ((pSlew->end_value < pSlew->start_value) && ((pSlew->slew_shape == Falling) || (pSlew->slew_shape == Both))){
		// Glide Down
		step_size = (pSlew->start_value - pSlew->end_value) / (pSlew->slew_length / slew_interval);
		if (step_size == 0) step_size = 1;		
		//log_msg("Step size Down: %d\n",step_size);
		while ((this_value >= pSlew->end_value) && (this_value >= step_size)){
			DACSingleChannelWrite(pSlew->i2c_handle, pSlew->i2c_address, pSlew->i2c_channel, this_value);
			this_value -= step_size;
			usleep(slew_interval / 2);
			if ((gpioTick() - start_tick) > step_ticks) break;	//We've been here too long
		} 
	}
	else {
		// Slew set, but Rising or Falling are off, so just output the end value
		DACSingleChannelWrite(pSlew->i2c_handle, pSlew->i2c_address, pSlew->i2c_channel, pSlew->end_value);
	}
    free(pSlew);
	return(0);
}

/*
 * Gate Thread - launched for each Track / Step that
 * has a Gate/Trigger. Uses gate_length to determine the
 * length of the pulse. The function lives just to perform 
 * the trigger, then ends itself
 */
static void *GateThread(void *arg)
{
	struct gate *pGate = (struct gate *)arg;
	uint32_t start_tick = gpioTick();
	if(pGate->gate_type == Trigger){
		/* Gate On */
		GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,1);
		usleep(pGate->gate_length);
		/* Gate Off */
		GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,0);
	}
	else if(pGate->gate_type == Gate){
		/* Gate On */
		GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,1);
		if (pGate->retrigger_count == 1){
			/* Wait until approximately the end of the step */
			usleep((step_ticks * 80)/100);
			/* Gate Off */
			GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,0);
		}
		else if (pGate->retrigger_count > 1){
			/* this step is to be re-triggered, so work out the sleep length between triggers 
			 * Work on 95% of the the total step time, as this gives a bit of leeway for the
			 * function calling overhead
			 */
			int sleep_time = (((step_ticks * 80)/100) / pGate->retrigger_count);
			int i;
			for (i = 0; i < pGate->retrigger_count; i++){
				/* Gate On */
				GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,1);
				usleep(sleep_time);
				/* Gate Off */
				GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,0);
			}
		}
	}
    free(pGate);
	return(0);
}

 int startup(void)
 {
	 // Initial state of the Screen Elements (Menus etc)
	 ClearScreenOverlays();
	 DisplayPage = GridView;
	 // Initialise the Deatched pThread attribute
	int rc = pthread_attr_init(&detached_attr);
	rc = pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
	// establish a Mutex lock to protect the MCP23008
	if (pthread_mutex_init(&mcp23008_lock, NULL) != 0){
        log_msg("MCP23008 mutex init failed\n");
    }
	if (pthread_mutex_init(&pcf8574_lock, NULL) != 0){
        log_msg("PCF8574 mutex init failed\n");
    }
	 // Initialise the Europi structure 
	int channel;
	for(channel=0;channel < MAX_CHANNELS;channel++){
		
	}
	// Check and note the hardware revision - this is 
	// important because the I2C bus is Bus 0 on the 
	// older boards, and Bus 1 on the later ones
	hw_version = gpioHardwareRevision();
	log_msg("Running on hw_revision: %d\n",hw_version);
	// PIGPIO Function initialisation
	if (gpioInitialise()<0) return 1;
	// TEMP for testing with the K-Sharp screen
	// Use one of the buttons to quit the app
	gpioSetMode(BUTTON1_IN, PI_INPUT);
	gpioGlitchFilter(BUTTON1_IN,100);
	gpioSetPullUpDown(BUTTON1_IN, PI_PUD_UP);
	gpioSetMode(BUTTON2_IN, PI_INPUT);
	gpioGlitchFilter(BUTTON2_IN,100);
	gpioSetPullUpDown(BUTTON2_IN, PI_PUD_UP);
	gpioSetMode(BUTTON3_IN, PI_INPUT);
	gpioGlitchFilter(BUTTON4_IN,100);
	gpioSetPullUpDown(BUTTON3_IN, PI_PUD_UP);
	gpioSetMode(BUTTON4_IN, PI_INPUT);
	gpioGlitchFilter(BUTTON4_IN,100);
	gpioSetPullUpDown(BUTTON4_IN, PI_PUD_UP);
	gpioSetMode(ENCODER_BTN, PI_INPUT);
	gpioGlitchFilter(ENCODER_BTN,100);
	gpioSetPullUpDown(ENCODER_BTN, PI_PUD_UP);

	//gpioSetMode(TOUCH_INT, PI_INPUT);
	//gpioSetPullUpDown(TOUCH_INT, PI_PUD_UP);
	gpioSetMode(CLOCK_IN, PI_INPUT);
	//gpioGlitchFilter(CLOCK_IN,100);				/* EXT_CLK has to be at the new level for 100uS before it is registered */
	gpioSetMode(RUNSTOP_IN, PI_INPUT);
	gpioSetPullUpDown(RUNSTOP_IN, PI_PUD_UP);
	gpioGlitchFilter(RUNSTOP_IN,100);
	gpioSetMode(INTEXT_IN, PI_INPUT);
	gpioSetPullUpDown(INTEXT_IN, PI_PUD_UP);
	gpioGlitchFilter(INTEXT_IN,100);
	gpioSetMode(RESET_IN, PI_INPUT);
	gpioGlitchFilter(RESET_IN,100);
	// Register callback routines
	gpioSetAlertFunc(BUTTON1_IN, button_1);
	gpioSetAlertFunc(BUTTON2_IN, button_2);
	gpioSetAlertFunc(BUTTON3_IN, button_3);
	gpioSetAlertFunc(BUTTON4_IN, button_4);
	gpioSetAlertFunc(ENCODER_BTN, encoder_button);
	//gpioSetAlertFunc(TOUCH_INT, touch_interrupt);
	gpioSetAlertFunc(CLOCK_IN, external_clock);
	gpioSetAlertFunc(RUNSTOP_IN, runstop_input);
	gpioSetAlertFunc(INTEXT_IN, clocksource_input);
	gpioSetAlertFunc(RESET_IN, reset_input);
	/* Establish Rotary Encoder Callbacks */
	gpioSetMode(ENCODERA_IN, PI_INPUT);
	gpioSetMode(ENCODERB_IN, PI_INPUT);
	gpioSetPullUpDown(ENCODERA_IN, PI_PUD_UP);
	gpioSetPullUpDown(ENCODERB_IN, PI_PUD_UP);
	gpioSetAlertFunc(ENCODERA_IN, encoder_callback);
	gpioSetAlertFunc(ENCODERB_IN, encoder_callback);
	encoder_level_A = 0;
	encoder_level_B = 0;
	encoder_tick = gpioTick();
	encoder_focus = none;

	/* Raylib Initialisation */
	/* force main screen resolution to same as TFT */
	fbfd = open("/dev/fb0", O_RDWR);
	if (!fbfd) {
		log_msg("Error: cannot open framebuffer device.");
	}
	unsigned int screensize = 0;
	// Get current screen metrics
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		log_msg("Error reading variable information.");
	}
	// Store this for when the prog closes (copy vinfo to vinfo_orig)
	// because we'll need to re-instate all the existing parameters
	memcpy(&orig_vinfo, &vinfo, sizeof(struct fb_var_screeninfo));
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &orig_vinfo)) {
		log_msg("Error reading variable information.");
	}

	// Change variable info - force 16 bit and resolution
	//vinfo.bits_per_pixel = 16;
	vinfo.xres = X_MAX;
	vinfo.yres = Y_MAX;
//	vinfo.xres_virtual = vinfo.xres;
//	vinfo.yres_virtual = vinfo.yres;
  
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo)) {
		log_msg("Error setting variable information.");
	}
  
	// Get fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		log_msg("Error reading fixed information.");
	}
	
	//Raylib Initialisation
	InitWindow(X_MAX, Y_MAX, "Europi by Audio Morpholgy");
	ToggleFullscreen();
	DisableCursor();
	font1 = LoadSpriteFont("resources/fonts/mecha.rbmf");

	//Splash screen
	Splash = LoadTexture("resources/images/splash_screen.png");
	BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawTexture(Splash,0,0,WHITE);
	EndDrawing();

  	// Turn the cursor off
    kbfd = open(kbfds, O_WRONLY);
    if (kbfd >= 0) {
        ioctl(kbfd, KDSETMODE, KD_GRAPHICS);
    }
    else {
        log_msg("Could not open kbfds.");
    }
	
	/* Open all the hardware ports */
	hardware_init();

	//initialise the sequence for testing purposes
	init_sequence();
	/* Start the internal sequencer clock */
	run_stop = STOP;		/* master clock is running, but step generator is halted */
	gpioHardwarePWM(MASTER_CLK,clock_freq,500000);
	gpioSetAlertFunc(MASTER_CLK, master_clock);
	
	prog_running = 1;
    return(0);
}

/* Things to do as the prog closess */
int shutdown(void)
 {
	int track;
	// Splash screen
	BeginDrawing();
	DrawTexture(Splash,0,0,WHITE);
	EndDrawing();
	/* Slight pause to give some threads time to exist */
	sleep(2);
	/* clear down all CV / Gate outputs */
	for (track = 0;track < MAX_TRACKS; track++){
		/* set the CV for each channel to the Zero level*/
		if (Europi.tracks[track].channels[0].enabled == TRUE ){
			DACSingleChannelWrite(Europi.tracks[track].channels[0].i2c_handle, Europi.tracks[track].channels[0].i2c_address, Europi.tracks[track].channels[0].i2c_channel, Europi.tracks[track].channels[0].scale_zero);
		}
		/* set the Gate State for each channel to OFF*/
		if (Europi.tracks[track].channels[GATE_OUT].enabled == TRUE ){
			GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x00);
		}
	}

	/* Raylib de-initialisation */
	UnloadSpriteFont(font1);
	UnloadTexture(Splash);
	CloseWindow();        			// Close window and OpenGL context
	// Set screen resolution back to Original values
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo)) {
		log_msg("Error re-setting variable information.");
	}
	close(fbfd);
	/* unload the PIGPIO library */
	gpioTerminate();
//	unsigned int screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
//	if (kbfd >= 0) {
//		ioctl(kbfd, KDSETMODE, KD_TEXT);
//	}
//	else {
//		log_msg("Could not reset keyboard mode.");
//	}
	// destroy the Mutex locks for the mcp23008, pcf8574
	pthread_mutex_destroy(&mcp23008_lock);
	pthread_mutex_destroy(&pcf8574_lock);
	return(0);
 }

/* Deals with messages that need to be logged
 * This could be modified to write them to a 
 * log file. In the first instance, it simply
 * printf's them
 */
void log_msg(const char* format, ...)
{
   char buf[128];
	if (print_messages == TRUE){
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	
	fprintf(stderr, "%s", buf);
	if(debug == TRUE) sprintf(debug_txt, "%s\0", buf);
	va_end(args);
	}
}



/* Called to initiate a controlled shutdown and
 * exit of the prog
 */
void controlled_exit(int gpio, int level, uint32_t tick)
{
	prog_running = 0;
}

void encoder_callback(int gpio, int level, uint32_t tick){
	int dir = 0;
	int vel;
	uint32_t tick_diff;
	if (gpio == ENCODERA_IN) encoder_level_A = level; else encoder_level_B = level;
	if (gpio != encoder_lastGpio)	/* debounce */
	{
		encoder_lastGpio=gpio;
		if((gpio == ENCODERA_IN) && (level==1))
		{
			if (encoder_level_B) dir=1;
		}
		else if ((gpio == ENCODERB_IN) && (level==1))
		{
			if(encoder_level_A) dir = -1;
		}
	}
	if(dir != 0)
	{
		/* work out the speed */
		tick_diff = tick-encoder_tick;
		encoder_tick = tick;
		//log_msg("Ticks: %d\n",tick_diff);
		switch(tick_diff){
				case 0 ... 3000:
					vel = 10;
				break;
				case 3001 ... 5000:
					vel = 9;
				break;
				case 5001 ... 10000:
					vel = 8;
				break;
				case 10001 ... 20000:
					vel = 7;
				break;
				case 20001 ... 30000:
					vel = 6;
				break;
				case 30001 ... 40000:
					vel = 5;
				break;
				case 40001 ... 50000:
					vel = 4;
				break;
				case 50001 ... 100000:
					vel = 3;
				break;
				case 100001 ... 200000:
					vel = 2;
				break;
				default:
					vel = 1;
		}
		/* Call function based on current encoder focus */
		switch(encoder_focus){
		case none:
			break;
		case menu_on:
			// Menu is on display
			if (dir == 1){
				// Move to highlight the next menu item in the list
				int i = 0;
				while(Menu[i+1].name != NULL){
					if(Menu[i].expanded == 1){
						// This menu branch expanded, 
						// so iterate down it
						int j = 0;
						while(Menu[i].child[j+1]->name != NULL){
							if(Menu[i].highlight == 1){
								Menu[i].highlight = 0;
								Menu[i].child[j]->highlight = 1;
								break;
							}
							else if (Menu[i].child[j]->highlight == 1){
								Menu[i].child[j]->highlight = 0;
								Menu[i].child[j+1]->highlight = 1;
								break;
							}
							j++;
						}
					}
					else {
						if(Menu[i].highlight == 1) {
							Menu[i].highlight = 0;
							Menu[i+1].highlight = 1;
							break;
						}
					}
					i++;
				}
			}
			else {
				// Highlight previous item
				int i = 0;
				while(Menu[i].name != NULL){
					if(Menu[i].expanded == 1){
						// This menu branch expanded, 
						// so iterate up it
						int j = 0;
						while(Menu[i].child[j]->name != NULL){
							if(Menu[i].child[j]->highlight == 1){
								Menu[i].child[j]->highlight = 0;
								if(j > 0) {
									Menu[i].child[j-1]->highlight = 1;	
								}
								else {
									Menu[i].highlight = 1;	
								}
								break;
							}
							j++;
						}
					}
					else {
						if((Menu[i].highlight == 1) && (i > 0)) {
							Menu[i].highlight = 0;
							Menu[i-1].highlight = 1; 
							break;
						}
					}
					i++;
				}
			}
			break;
		case track_select:
			if(dir == 1){
				int track = 0;
				int prev_selected;
				int found_new = FALSE;
				while(track < MAX_TRACKS){
					if(Europi.tracks[track].selected == TRUE){
						// deselect this track
						prev_selected = track;
						Europi.tracks[track].selected = FALSE;
						GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x00);
						// select the next enabled one
						if(track < MAX_TRACKS - 1) track++;
						while(track < MAX_TRACKS){
							if(Europi.tracks[track].channels[CV_OUT].enabled == TRUE){
								Europi.tracks[track].selected = TRUE;
								found_new = TRUE;
								GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x01);
								break;
							}
							track++;
						}
					}
					track++;
				}
				// Didn't find a new one to select, so re-select the previous one
				if (found_new == FALSE){
					Europi.tracks[prev_selected].selected = TRUE;
				}
			}
			else {
				int track = MAX_TRACKS - 1;
				int prev_selected;
				int found_new = FALSE;
				while(track >= 0){
					if(Europi.tracks[track].selected == TRUE){
						// deselect this track
						prev_selected = track;
						Europi.tracks[track].selected = FALSE;
						GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x00);
						// select the next enabled one
						if(track > 0) track--;
						while(track >= 0){
							if(Europi.tracks[track].channels[CV_OUT].enabled == TRUE){
								Europi.tracks[track].selected = TRUE;
								found_new = TRUE;
								GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x01);
								break;
							}
							track--;
						}
					}
					track--;
				}
				// Didn't find a new one to select, so re-select the previous one
				if (found_new == FALSE){
					Europi.tracks[prev_selected].selected = TRUE;
				}
			}
			break;
		case step_select:
		{
			int track = 0;
			while (track < MAX_TRACKS){
				if(Europi.tracks[track].selected == TRUE){
					if (dir == 1) {
						if (Europi.tracks[track].current_step < (Europi.tracks[track].last_step -1)) Europi.tracks[track].current_step++;
					}
					else {
						if (Europi.tracks[track].current_step > 0) Europi.tracks[track].current_step--;
					}
					break;
				}
				track++;
			}
			/* Output the current value for this track / step, so we can hear what's going on */
			//float output_scaling;
			//uint16_t quantized;
			//output_scaling = ((float)Europi.tracks[track].channels[CV_OUT].scale_max - (float)Europi.tracks[track].channels[CV_OUT].scale_zero) / (float)60000;
			//quantized = quantize(Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value, Europi.tracks[track].channels[CV_OUT].quantise);
			//Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value = (uint16_t)(output_scaling * quantized) + Europi.tracks[track].channels[CV_OUT].scale_zero;
			DACSingleChannelWrite(Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value);
			// And turn the Gate output on
			GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x01);
			break;
		}
		case set_zerolevel:
				if(dir == 1){
					int track=0;
					while(track < MAX_TRACKS){
						if(Europi.tracks[track].selected == TRUE){
							if(Europi.tracks[track].channels[CV_OUT].scale_zero <= 65535-vel){
							Europi.tracks[track].channels[CV_OUT].scale_zero += vel;
							DACSingleChannelWrite(Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].scale_zero);
							}
							break;
						}
						track++;
					}
				}
				else {
					int track=0;
					while(track < MAX_TRACKS){
						if(Europi.tracks[track].selected == TRUE){
							if(Europi.tracks[track].channels[CV_OUT].scale_zero >= vel){
							Europi.tracks[track].channels[CV_OUT].scale_zero -= vel;
							DACSingleChannelWrite(Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].scale_zero);
							}
							break;
						}
						track++;
					}
					
				}
			break;
		case set_maxlevel:
				if(dir == 1){
					int track=0;
					while(track < MAX_TRACKS){
						if(Europi.tracks[track].selected == TRUE){
							if(Europi.tracks[track].channels[CV_OUT].scale_max <= 65535-vel){
							Europi.tracks[track].channels[CV_OUT].scale_max += vel;
							DACSingleChannelWrite(Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].scale_max);
							}
							break;
						}
						track++;
					}
				}
				else {
					int track=0;
					while(track < MAX_TRACKS){
						if(Europi.tracks[track].selected == TRUE){
							if(Europi.tracks[track].channels[CV_OUT].scale_max >= vel){
							Europi.tracks[track].channels[CV_OUT].scale_max -= vel;
							DACSingleChannelWrite(Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].scale_max);
							}
							break;
						}
						track++;
					}
					
				}
			break;
		case set_loop:
				if(dir == 1){
					int track=0;
					while(track < MAX_TRACKS){
						if(Europi.tracks[track].selected == TRUE){
							if(Europi.tracks[track].last_step < MAX_STEPS){
								Europi.tracks[track].last_step++;
							}
							break;
						}
						track++;
					}
				}
				else {
					int track=0;
					while(track < MAX_TRACKS){
						if(Europi.tracks[track].selected == TRUE){
							if(Europi.tracks[track].last_step > 1){
								Europi.tracks[track].last_step--;
							}
							break;
						}
						track++;
					}
					
				}
			break;
		case set_pitch:
		{
			int track = 0;
			while (track < MAX_TRACKS){
				if(Europi.tracks[track].selected == TRUE){
					if(vel > 3) vel *= 10;
					if (dir == 1) {
						if (Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value <= (60000 - vel)) Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value += vel;
					}
					else {
						if (Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value >= vel) Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value -= vel;
					}
					// Quantise this and output it
					//float output_scaling;
					//uint16_t quantized;
					//output_scaling = ((float)Europi.tracks[track].channels[CV_OUT].scale_max - (float)Europi.tracks[track].channels[CV_OUT].scale_zero) / (float)60000;
					//quantized = quantize(Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value, Europi.tracks[track].channels[CV_OUT].quantise);
					//Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value = (uint16_t)(output_scaling * quantized) + Europi.tracks[track].channels[CV_OUT].scale_zero;
					Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value = Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value; //= (uint16_t)(output_scaling * quantized) + Europi.tracks[track].channels[CV_OUT].scale_zero;
					DACSingleChannelWrite(Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value);
					break;
				}
				track++;
			}
			
			break;
		}
		case set_quantise:
		{
			int track = 0;
			while (track < MAX_TRACKS){
				if(Europi.tracks[track].selected == TRUE){
					if (dir == 1) {
						if (Europi.tracks[track].channels[CV_OUT].quantise < 47) Europi.tracks[track].channels[CV_OUT].quantise++;
					}
					else {
						if (Europi.tracks[track].channels[CV_OUT].quantise > 0) Europi.tracks[track].channels[CV_OUT].quantise--;
					}
					// Re-Quantise track
					quantize_track(track,Europi.tracks[track].channels[CV_OUT].quantise);
					break;
				}
				track++;
			}
			
			break;
		}
		case pitch_cv:
			break;
		case slew_type:
			slew_adjust(dir, vel);
			break;
		case gate_on_off:
			break;
		case repeat:
			step_repeat(dir,vel);
			break;
		case quantise:
			break;
		}
		
	}
}


/* Rotary encoder button pressed */
void encoder_button(int gpio, int level, uint32_t tick)
{
	int i = 0;
	if(level == 0){
		switch(encoder_focus){
		case none:

			break;
		case menu_on:
			//Expand / contract sub-menu or execute callback
            toggle_menu();
			break;
		case track_select:
				if(ScreenOverlays.SetZero == 1)	encoder_focus = set_zerolevel;
				else if (ScreenOverlays.SetTen == 1) encoder_focus = set_maxlevel;
				else if (ScreenOverlays.SetLoop == 1) encoder_focus = set_loop;
				else if (ScreenOverlays.SetPitch == 1) encoder_focus = step_select;
				else if (ScreenOverlays.SetQuantise == 1) encoder_focus = set_quantise;
			break;
		case step_select:
				encoder_focus = set_pitch;
			break;
		case set_pitch:
				encoder_focus = track_select;
			break;
		case set_zerolevel:
				encoder_focus = track_select;
			break;
		case set_maxlevel:
				encoder_focus = track_select;
			break;
		case set_loop:
				encoder_focus = track_select;
			break;
		case set_quantise:
				encoder_focus = track_select;
			break;
		case pitch_cv:
			encoder_focus = slew_type;
			break;
		case slew_type:
			encoder_focus = gate_on_off;
			break;
		case gate_on_off:
			encoder_focus = repeat;
			break;
		case repeat:
			encoder_focus = quantise;
			break;
		case quantise:
			encoder_focus = none;
			break;
		default:
			encoder_focus = none;
		}
	}
}

/* toggle_menu */
void toggle_menu(void){
    int i = 0;
    while(Menu[i].name != NULL){
        if((Menu[i].highlight == 1) && (Menu[i].child[0]->name != NULL)){
            Menu[i].expanded ^= 1;
        }
        else if ((Menu[i].expanded == 1) && (Menu[i].highlight == 0)){
            int j = 0;
            while(Menu[i].child[j]->name != NULL){
                if((Menu[i].child[j]->highlight == 1) && (Menu[i].child[j]->funcPtr != NULL)) {
                    Menu[i].child[j]->funcPtr();
                }
                j++;
            }
        }
        i++;
    }
}

/* clear_menu 
 * runs through the menu structure and 
 * ensures that no sub menus are expanded
 * and that the only element highlighted is 
 * the first menu item
 */
void ClearMenus(void){
    int i = 0;
    while(Menu[i].name != NULL){
        Menu[i].highlight = 0;
        Menu[i].expanded = 0;
        int j = 0;
        while(Menu[i].child[j]->name != NULL){
            Menu[i].child[j]->highlight = 0;
            j++;
        }
        i++;
    }
}

/* MenuSelectItem takes the passed parent and child
 * branches and clears all but those
 */
void MenuSelectItem(int Parent, int Child){
    ClearMenus();
    if(Child == 0) {
        Menu[Parent].highlight = 1;   
    }
    else {
        Menu[Parent].expanded = 1;
        Menu[Parent].child[Child]->highlight = 1;   
    }
}

/* Button 1 pressed */
void button_1(int gpio, int level, uint32_t tick)
{
	if (level == 1) {
		// Temp:Controlled shutdown
		prog_running = 0;
	}

}

/* Button 2 pressed */
void button_2(int gpio, int level, uint32_t tick)
{
	if (level == 1) ScreenOverlays.MainMenu ^= 1;
	if(ScreenOverlays.MainMenu == 1){
		encoder_focus = menu_on;
	}
    else {
        ClearMenus();
        MenuSelectItem(0,0);    // Select just the first item of the first branch
    }
}
/* Button 3 pressed */
void button_3(int gpio, int level, uint32_t tick)
{
	float bpm;
	if (level == 1) {
		clock_freq -= 10;
		if (clock_freq < 1) clock_freq = 1;
		gpioHardwarePWM(MASTER_CLK,clock_freq,500000);
		bpm = ((((float)clock_freq)/48) * 60);
		log_msg("BPM: %f\n",bpm);
	}
}
	
/* Button 4 pressed */
void button_4(int gpio, int level, uint32_t tick)
{
	float bpm;
	if (level == 1) {
		clock_freq += 10;
		gpioHardwarePWM(MASTER_CLK,clock_freq,500000);
		bpm = ((((float)clock_freq)/48) * 60);
		log_msg("BPM: %f\n",bpm);
	}
}

/*
 * Specifically looks for a PCF8574 on the default
 * address of 0x20 this should be unique to the Main
 * Europi module, therefore it should be safe to 
 * return a handle to the DAC8574 on the anticipated
 * address of 0x08
 */
int EuropiFinder()
{
	int retval;
	int DACHandle;
	unsigned pcf_addr = PCF_BASE_ADDR;
	unsigned pcf_handle = i2cOpen(1,pcf_addr,0);
	if(pcf_handle < 0)return -1;
	// Unfortunately, this tends to retun a valid handle even though the
	// device doesn't exist. The only way to really check it is there 
	// to try writing to it, which will fail if it doesn't exist.
	retval = i2cWriteByte(pcf_handle, (unsigned)(0xF0));
	// either that worked, or it didn't. Either way we
	// need to close the handle to the device for the
	// time being
	i2cClose(pcf_handle);
	if(retval < 0) return -1;	
	// pass back a handle to the DAC8574, 
	// which will be on i2c Address 0x4C
	DACHandle = i2cOpen(1,DAC_BASE_ADDR,0);
	return DACHandle;
}

/* 
 * Looks for an SC16IS750 on the passed address. If one
 * exists, then it should be safe to assume that this is 
 * a MidiMinion, in which case it is safe to pass back a handle
 * to the DAC8574
 */
int MidiMinionFinder(unsigned address)
{
	int handle;
	int retval;
	unsigned i2cAddr;
	int mid_handle;
	if((address > 8) || (address < 0)) return -1;
	i2cAddr = MID_BASE_ADDR | (address & 0x7);
	mid_handle = i2cOpen(1,i2cAddr,0);
	if (handle < 0) return -1;
	return handle;
}

/* 
 * Looks for an MCP23008 on the passed address. If one
 * exists, then it should be safe to assume that this is 
 * a Minion, in which case it is safe to pass back a handle
 * to the DAC8574
 */
int MinionFinder(unsigned address)
{
	int handle;
	int mcp_handle;
	int retval;
	unsigned i2cAddr;
	if((address > 8) || (address < 0)) return -1;
	i2cAddr = MCP_BASE_ADDR | (address & 0x7);
	mcp_handle = i2cOpen(1,i2cAddr,0);
	if (mcp_handle < 0) return -1;
	/* 
	 * we have a valid handle, however whether there is actually
	 * a device on this address can seemingly only be determined 
	 * by attempting to write to it.
	 */
	 retval = i2cWriteWordData(mcp_handle, 0x00, (unsigned)(0x0));
	 // close the handle to the PCF8574 (it will be re-opened shortly)
	 i2cClose(mcp_handle);
	 if(retval < 0) return -1;
	 i2cAddr = DAC_BASE_ADDR | (address & 0x3);
	 handle = i2cOpen(1,i2cAddr,0);
	 return handle;
}

/* 
 * DAC8574 16-Bit DAC single channel write 
 * Channel, in this context is the full 6-Bit address
 * of the channel - ie [A3][A2][A1][A0][C1][C0]
 * [A1][A0] are ignored, because we already have a 
 * handle to a device on that address. 
 * [A3][A2] are significant, as they need to match
 * the state of the address lines on the DAC
 * The ctrl_reg needs to look like this:
 * [A3][A2][0][1][x][C1][C0][0]
 */
void DACSingleChannelWrite(unsigned handle, uint8_t address, uint8_t channel, uint16_t voltage){
	uint16_t v_out;
	uint16_t v_sav;
	uint8_t ctrl_reg;
	int retval;
	if(impersonate_hw == TRUE) return;
	//log_msg("%d, %d, %d, %d\n",handle,address,channel,voltage);
	ctrl_reg = (((address & 0xC) << 4) | 0x10) | ((channel << 1) & 0x06);
	//log_msg("handle: %0x, address: %0x, channel: %d, ctrl_reg: %02x, Voltage: %d\n",handle, address, channel,ctrl_reg,voltage);
	//swap MSB & LSB because i2cWriteWordData sends LSB first, but the DAC expects MSB first
	v_sav = voltage;
	v_out = ((voltage >> 8) & 0x00FF) | ((voltage << 8) & 0xFF00);
	//log_msg("hdle: %0x, addr: %0x, chan: %d, ctrl: %02x, Volt: %d Vshift: %0x\n",handle, address, channel,ctrl_reg,v_sav,v_out);
	retval = i2cWriteWordData(handle,ctrl_reg,v_out);
	//log_msg("Ret: %d\n",retval);
}
/* 
 * GATEMultiOutput
 * 
 * Writes the passed 8-Bit value straight to the GPIO Extender
 * the idea being that this is a quicker way of turning all
 * gates off on a Minion at the end of a Gate Pulse
 * 
 * Note that the LS 4 bits are the Gate, and the MS 4 bits are
 * the LEDs, so the lower 4 bits should match the upper 4 bits
 */
void GATEMultiOutput(unsigned handle, uint8_t value)
{
	if(impersonate_hw == TRUE) return;
	i2cWriteByteData(handle, 0x09,value);	
}
/*
 * Outputs the passed value to the GATE output identified
 * by the Handle to the Open device, and the channel (0-3)
 * As this is just a single bit, this function has to 
 * read the current state of the other gates on the GPIO
 * extender, incorporate this value, then write everything
 * back.
 * Note that, on the Minion, the Gates are on GPIO Ports 0 to 3, 
 * though the gate indicator LEDs are on GPIO Ports 4 to 7, so
 * the output values from ports 0 to 3 need to be mirrored
 * to ports 4 - 7
 * The i2c Protocols are different for the PCF8574 used on the 
 * Europi, and the MCP23008 used on the Minions, and the MCP23008
 * will drive an LED from its High output, but the PCF8574 will
 * only pull it low!
 * A Mutex protects the read-update-write cycle for the MCP23008
 * as race conditions can exist due to the fact that multiple
 * threads can be attempting the same opersation at the same time
 */ 
void GATESingleOutput(unsigned handle, uint8_t channel,int Device,int Value)
{
	//log_msg("handle: %d, channel: %d, Device: %d, Value: %d\n",handle,channel,Device,Value);
	uint8_t curr_val;
	if(impersonate_hw == TRUE) return;
	if(Device == DEV_MCP23008){
		if (Value > 0){
			// Set corresponding bit high
			mcp23008_state[handle] |= (0x01 << channel);
			mcp23008_state[handle] |= (0x01 << (channel + 4));
		}
		else {
			// Set corresponding bit low
			mcp23008_state[handle] &= ~(0x01 << channel);
			mcp23008_state[handle] &= ~(0x01 << (channel + 4));
		}
		i2cWriteByteData(handle, 0x09,mcp23008_state[handle]);
		
		/*
		pthread_mutex_lock(&mcp23008_lock);
		curr_val = i2cReadByteData(handle, 0x09);
		if (Value > 0){
			// Set corresponding bit high
			curr_val |= (0x01 << channel);
			curr_val |= (0x01 << (channel + 4));
		}
		else {
			// Set corresponding bit low
			curr_val &= ~(0x01 << channel);
			curr_val &= ~(0x01 << (channel + 4));
		}
		i2cWriteByteData(handle, 0x09,curr_val);
		pthread_mutex_unlock(&mcp23008_lock);
		*/
	}
	else if (Device == DEV_PCF8574){
		/* The PCF8574 will only turn an LED on
		 * when its output is LOW, so the upper
		 * 4 bits need to be the inverse of the
		 * lower 4 bits. Also, we cannot use the 
		 * trick of reading the current state of
		 * the port, as it's bi-directonal, but 
		 * non-latching. Therefore we have to keep
		 * the current state in a global variable
		 */
		pthread_mutex_lock(&pcf8574_lock);
		if (Value > 0){
			// Set corresponding bit high
			PCF8574_state |= (0x01 << channel);
			// the equivalent in the MS Nibble needs to be low to turn the LED on
			PCF8574_state &= ~(0x01 << (channel+4));
		}
		else {
			// Set corresponding bit low
			PCF8574_state &= ~(0x01 << channel);
			// the equivalent in the MS Nibble needs to be high to turn the LED off
			PCF8574_state |= (0x01 << (channel+4));
		}
		i2cWriteByte(handle,PCF8574_state);
		pthread_mutex_unlock(&pcf8574_lock);
	}
}
/*
 * Initialises all the hardware ports - scanning for connected
 * Minions etc.
 */
void hardware_init(void)
{
	unsigned track = 0;
	unsigned address;
	unsigned mcp_addr;
	unsigned pcf_addr;
	uint8_t chnl;
	int handle;
	int gpio_handle;
	int pcf_handle;
	/* Before we start, make sure all Tracks / Channels are disabled */
	for (track = 0; track < MAX_TRACKS;track++){
		Europi.tracks[track].selected = FALSE;
		Europi.tracks[track].channels[CV_OUT].enabled = FALSE;
		Europi.tracks[track].channels[GATE_OUT].enabled = FALSE;
	}
	/*
	 * If impersonate_hw is set to TRUE, then this bypasses the 
	 * hardware checks, and pretends all hardware is present - this
	 * is useful when testing the software without the full hardware
	 * present
	 */
	 if (impersonate_hw == TRUE){
		 is_europi = TRUE;
		 for (track = 0; track < MAX_TRACKS;track++){
			/* These are just dummy values to fool the software
			 * into thinking it has the full hardware present
			 */
			Europi.tracks[track].channels[CV_OUT].enabled = TRUE;
			Europi.tracks[track].channels[CV_OUT].type = CHNL_TYPE_CV;
			Europi.tracks[track].channels[CV_OUT].quantise = 0;			/* Quantization off by default */
			Europi.tracks[track].channels[CV_OUT].i2c_handle = track;			
			Europi.tracks[track].channels[CV_OUT].i2c_device = DEV_DAC8574;
			Europi.tracks[track].channels[CV_OUT].i2c_address = 0x0;
			Europi.tracks[track].channels[CV_OUT].i2c_channel = 0;		
			Europi.tracks[track].channels[CV_OUT].scale_zero = 280;		/* Value required to generate zero volt output */
			Europi.tracks[track].channels[CV_OUT].scale_max = 63000;		/* Value required to generate maximum output voltage */
			Europi.tracks[track].channels[CV_OUT].transpose = 0;			/* fixed (transpose) voltage offset applied to this channel */
			Europi.tracks[track].channels[CV_OUT].octaves = 10;			/* How many octaves are covered from scale_zero to scale_max */
			Europi.tracks[track].channels[CV_OUT].vc_type = VOCT;
			Europi.tracks[track].channels[GATE_OUT].enabled = TRUE;
			Europi.tracks[track].channels[GATE_OUT].type = CHNL_TYPE_GATE;
			Europi.tracks[track].channels[GATE_OUT].i2c_handle = track;			
			Europi.tracks[track].channels[GATE_OUT].i2c_device = DEV_PCF8574;
			Europi.tracks[track].channels[GATE_OUT].i2c_channel = 0;
		 }
		 return;
	 }
	/* 
	 * Specifically look for a PCF8574 on address 0x38
	 * if one exists, then it's on the Europi, so the first
	 * two Tracks will be allocated to the Europi
	 */
	track = 0;
	address = 0x08;
	handle = EuropiFinder();
	if (handle >=0){
		/* we have a Europi - it supports 2 Tracks each with 2 channels (CV + GATE) */
		log_msg("Europi Found on i2cAddress %d handle = %d\n",address, handle);
		is_europi = TRUE;
		/* As this is a Europi, then there should be a PCF8574 GPIO Expander on address 0x38 */
		pcf_addr = PCF_BASE_ADDR;
		pcf_handle = i2cOpen(1,pcf_addr,0);
		if(pcf_handle < 0){log_msg("failed to open PCF8574 associated with DAC8574 on Addr: 0x08");}
		if(pcf_handle >= 0) {
			/* Gates off, LEDs off */
			i2cWriteByte(pcf_handle, (unsigned)(0xF0));
		}
		Europi.tracks[track].channels[CV_OUT].enabled = TRUE;
		Europi.tracks[track].channels[CV_OUT].type = CHNL_TYPE_CV;
		Europi.tracks[track].channels[CV_OUT].quantise = 0;			/* Quantization off by default */
		Europi.tracks[track].channels[CV_OUT].i2c_handle = handle;			
		Europi.tracks[track].channels[CV_OUT].i2c_device = DEV_DAC8574;
		Europi.tracks[track].channels[CV_OUT].i2c_address = 0x08;
		Europi.tracks[track].channels[CV_OUT].i2c_channel = 0;		
		Europi.tracks[track].channels[CV_OUT].scale_zero = 280;		/* Value required to generate zero volt output */
		Europi.tracks[track].channels[CV_OUT].scale_max = 63000;		/* Value required to generate maximum output voltage */
		Europi.tracks[track].channels[CV_OUT].transpose = 0;			/* fixed (transpose) voltage offset applied to this channel */
		Europi.tracks[track].channels[CV_OUT].octaves = 10;			/* How many octaves are covered from scale_zero to scale_max */
		Europi.tracks[track].channels[CV_OUT].vc_type = VOCT;
		/* Track 0 channel 1 = Gate Output */
		Europi.tracks[track].channels[GATE_OUT].enabled = TRUE;
		Europi.tracks[track].channels[GATE_OUT].type = CHNL_TYPE_GATE;
		Europi.tracks[track].channels[GATE_OUT].i2c_handle = pcf_handle;			
		Europi.tracks[track].channels[GATE_OUT].i2c_device = DEV_PCF8574;
		Europi.tracks[track].channels[GATE_OUT].i2c_channel = 0;
		/* Track 1 channel 0 = CV */
		track++;
		Europi.tracks[track].channels[CV_OUT].enabled = TRUE;
		Europi.tracks[track].channels[CV_OUT].type = CHNL_TYPE_CV;
		Europi.tracks[track].channels[CV_OUT].quantise = 0;		
		Europi.tracks[track].channels[CV_OUT].i2c_handle = handle;			
		Europi.tracks[track].channels[CV_OUT].i2c_device = DEV_DAC8574;
		Europi.tracks[track].channels[CV_OUT].i2c_address = 0x08;
		Europi.tracks[track].channels[CV_OUT].i2c_channel = 1;		
		Europi.tracks[track].channels[CV_OUT].scale_zero = 280;		/* Value required to generate zero volt output */
		Europi.tracks[track].channels[CV_OUT].scale_max = 63000;		/* Value required to generate maximum output voltage */
		Europi.tracks[track].channels[CV_OUT].transpose = 0;				/* fixed (transpose) voltage offset applied to this channel */
		Europi.tracks[track].channels[CV_OUT].octaves = 10;			/* How many octaves are covered from scale_zero to scale_max */
		Europi.tracks[track].channels[CV_OUT].vc_type = VOCT;
		/* Track 1 channel 1 = Gate*/
		Europi.tracks[track].channels[GATE_OUT].enabled = TRUE;
		Europi.tracks[track].channels[GATE_OUT].type = CHNL_TYPE_GATE;
		Europi.tracks[track].channels[GATE_OUT].i2c_handle = pcf_handle;			
		Europi.tracks[track].channels[GATE_OUT].i2c_device = DEV_PCF8574;
		Europi.tracks[track].channels[GATE_OUT].i2c_channel = 1;		
		/* Channels 3 & 4 of the PCF8574 are the Clock and Step 1 out 
		 * no need to set them to anything specific, and we don't really
		 * want them appearing as additional tracks*/
		track++;	/* Minion tracks will therefore start from Track 2 */
	}
	/* 
	 * scan for Minions - these will be on addresses
	 * 0 - 7. Each Minion supports 4 Tracks
	 */
	for (address=0;address<=7;address++){
		handle = MinionFinder(address);
		if(handle >= 0){
			log_msg("Minion Found on Address %d handle = %d\n",address, handle);
			/* Get a handle to the associated MCP23008 */
			mcp_addr = MCP_BASE_ADDR | (address & 0x7);	
			gpio_handle = i2cOpen(1,mcp_addr,0);
			//log_msg("gpio_handle: %d\n",gpio_handle);
			if(gpio_handle < 0){log_msg("failed to open MCP23008 associated with DAC8574 on Addr: %0x\n",address);}
			/* Set MCP23008 IO direction to Output, and turn all Gates OFF */
			if(gpio_handle >= 0) {
				i2cWriteWordData(gpio_handle, 0x00, (unsigned)(0x0));
				i2cWriteByteData(gpio_handle, 0x09, 0x0);
				}
			int i;
			for(i=0;i<4;i++){
				//log_msg("Track: %d, channel: %d\n",track,i);
				/* Channel 0 = CV Output */
				Europi.tracks[track].channels[CV_OUT].enabled = TRUE;
				Europi.tracks[track].channels[CV_OUT].type = CHNL_TYPE_CV;
				Europi.tracks[track].channels[CV_OUT].quantise = 0;			/* Quantization Off by default */
				Europi.tracks[track].channels[CV_OUT].i2c_handle = handle;			
				Europi.tracks[track].channels[CV_OUT].i2c_device = DEV_DAC8574;
				Europi.tracks[track].channels[CV_OUT].i2c_address = address;
				Europi.tracks[track].channels[CV_OUT].i2c_channel = i;		
				Europi.tracks[track].channels[CV_OUT].scale_zero = 0X0000;	/* Value required to generate zero volt output */
				Europi.tracks[track].channels[CV_OUT].scale_max = 63000;		/* Value required to generate maximum output voltage NOTE: This needs to be overwritten during configuration*/
				Europi.tracks[track].channels[CV_OUT].transpose = 0;				/* fixed (transpose) voltage offset applied to this channel */
				Europi.tracks[track].channels[CV_OUT].octaves = 10;			/* How many octaves are covered from scale_zero to scale_max */
				Europi.tracks[track].channels[CV_OUT].vc_type = VOCT;
				/* Channel 1 = Gate Output*/
				if (gpio_handle >= 0) {
					Europi.tracks[track].channels[GATE_OUT].enabled = TRUE;
					Europi.tracks[track].channels[GATE_OUT].type = CHNL_TYPE_GATE;
					Europi.tracks[track].channels[GATE_OUT].i2c_handle = gpio_handle;			
					Europi.tracks[track].channels[GATE_OUT].i2c_device = DEV_MCP23008;
					Europi.tracks[track].channels[GATE_OUT].i2c_channel = i; 		
				}
				else {
					/* for some reason we couldn't get a handle to the MCP23008, so disable the Gate channel */
					Europi.tracks[track].channels[GATE_OUT].enabled = FALSE;
				}
				track++;
			}	
		}
	}
	/* All hardware identified - run through flashing each Gate just for fun */
	if (is_europi == TRUE){
		/* Track 0 Channel 1 will have the GPIO Handle for the PCF8574 channel 2 is Clock Out*/
		GATESingleOutput(Europi.tracks[0].channels[1].i2c_handle,CLOCK_OUT,DEV_PCF8574,HIGH);
		usleep(50000);
		GATESingleOutput(Europi.tracks[0].channels[1].i2c_handle,CLOCK_OUT,DEV_PCF8574,LOW);
		GATESingleOutput(Europi.tracks[0].channels[1].i2c_handle,STEP1_OUT,DEV_PCF8574,HIGH);
		usleep(50000);
		GATESingleOutput(Europi.tracks[0].channels[1].i2c_handle,STEP1_OUT,DEV_PCF8574,LOW);
	}

	for (track = 0;track < MAX_TRACKS; track++){
		for (chnl = 0; chnl < MAX_CHANNELS; chnl++){
			if (Europi.tracks[track].channels[chnl].enabled == TRUE){
				if (Europi.tracks[track].channels[chnl].type == CHNL_TYPE_GATE){
					GATESingleOutput(Europi.tracks[track].channels[chnl].i2c_handle, Europi.tracks[track].channels[chnl].i2c_channel,Europi.tracks[track].channels[chnl].i2c_device,1);	
					usleep(50000);
					GATESingleOutput(Europi.tracks[track].channels[chnl].i2c_handle, Europi.tracks[track].channels[chnl].i2c_channel,Europi.tracks[track].channels[chnl].i2c_device,0);	
				}
			}
		}
	}
}

/*
 * QUANTIZE
 * 
 * Takes a raw value between 0 and 60000 and returns
 * an absolute value quantized to a particular scale
 * 
 * It assumes an internal resolution of 6000 per octave
 */
int quantize(int raw, int scale){
	int octave = 0;
	int i;
	if(scale == 0) return raw;	// Scale = 0 => Quantization OFF
	if(raw > 60000) return 60000;
	while(raw > 6000){
		raw = raw - 6000;
		octave++;
	}
	for(i=0;i<=12;i++){
		if(raw >= lower_boundary[scale][i] && raw <= upper_boundary[scale][i]) return (scale_values[scale][i] + octave*6000);
	}
};

