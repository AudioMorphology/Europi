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

//#include <linux/input.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <pigpio.h>
#include <signal.h>

#include "europi.h"
//#include "front_panel.c"
//#include "touch.h"
//#include "touch.c"
//#include "quantizer_scales.h"
#include "../raylib/src/raylib.h"

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
extern char debug_messages[10][80];
extern char modal_dialog_txt1[];
extern char modal_dialog_txt2[];
extern char modal_dialog_txt3[];
extern char modal_dialog_txt4[];
extern int next_debug_slot;
extern char input_txt[];  
extern int kbfd;
extern int ThreadEnd;
extern int prog_running;
extern int run_stop; 
extern int save_run_stop;
extern int is_europi; 
extern int midi_clock_counter;
extern int midi_clock_divisor;
extern int retrig_counter;
extern int extclk_counter;
extern int extclk_level;
extern int clock_counter;
extern int clock_level;
extern int clock_freq;
extern int clock_source;
extern int TuningOn;
extern uint16_t TuningVoltage; 
extern uint8_t PCF8574_state;
extern int led_on;
extern int print_messages;
//extern unsigned int sequence[6][32][3];
//extern int current_step;
//extern int last_step;
extern int last_track;
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
extern int VerticalScrollPercent;
extern int sample;
extern int touched;
extern int encoder_level_A;
extern int encoder_level_B;
extern int encoder_lastGpio;
extern int encoder_pos;
extern int encoder_vel;
extern uint32_t encoder_tick;
extern enum encoder_focus_t encoder_focus;
extern enum btnA_func_t btnA_func;
extern enum btnB_func_t btnB_func;
extern enum btnC_func_t btnC_func;
extern enum btnD_func_t btnD_func;
extern int btnA_state;
extern int btnB_state;
extern int btnC_state;
extern int btnD_state;
extern struct europi Europi;
extern struct europi_hw Europi_hw;
extern enum display_page_t DisplayPage;
//extern struct screen_overlays ScreenOverlays;
extern uint32_t ActiveOverlays;
extern int kbd_char_selected;
extern struct MENU Menu[];
extern pthread_attr_t detached_attr;		
extern pthread_mutex_t mcp23008_lock;
extern pthread_mutex_t pcf8574_lock;
extern pthread_t midiThreadId[]; 
extern int midiThreadLaunched[];
extern uint8_t mcp23008_state[16];
extern int test_v;
pthread_t ThreadId; 		// Pointer to detatched Thread Ids (re-used by each/every detatched thread)
extern Font font1;
extern Texture2D Splash;
extern Texture2D KeyboardTexture;
extern Texture2D DialogTexture;
extern Texture2D SmallDialogTexture;
extern Texture2D TextInputTexture;
extern Texture2D Text2chTexture;
extern Texture2D Text5chTexture;
extern Texture2D Text10chTexture;
extern Texture2D MainScreenTexture;
extern Texture2D TopBarTexture;
extern Texture2D ButtonBarTexture; 
extern Texture2D VerticalScrollBarTexture;
extern Texture2D VerticalScrollBarShortTexture;
extern Texture2D ScrollHandleTexture;
extern int disp_menu;	
extern char **files;
extern char *kbd_chars[4][11];
extern size_t file_count;                      
extern int file_selected;
extern int first_file;


/* Internal Clock
 * 
 * This is the main timing loop for the 
 * whole programme, and which runs continuously
 * from program start to end.
 * 
 * The Master Clock runs at 96 * The BPM step
 * frequency
 *
 * This drives PIN 10 (GPIO 15), which loops back 
 * via the Clock In jack 
 */
void master_clock(int gpio, int level, uint32_t tick)
{
	if (run_stop == RUN){
		if (clock_counter++ > 95) {
			clock_counter = 0;
			gpioWrite(INT_CLK_OUT, 1);
		}
		if (clock_counter == 48) gpioWrite(INT_CLK_OUT,0);
	}
}

/* Main Clock - this is the function that advances the 
* sequnce on to the next step. The master clock is normalled
* back into this input via the external clock jack, thus
* inserting a jack into the Clock In input cuts out the
* internal clock and replaces it with the external clock
* signal
 */
void main_clock(int gpio, int level, uint32_t tick)
{
	if (run_stop == RUN){
		// Copy the clock signal to the Clock Out port
		GATESingleOutput(Europi.tracks[0].channels[GATE_OUT].i2c_handle,CLOCK_OUT,DEV_PCF8574,level);
		//GATESingleOutput(1,CLOCK_OUT,DEV_PCF8574,level);
		if (level == 1) next_step();
	}
}

/* run/stop switch input
 * called whenever the switch changes state
 */
void runstop_input(int gpio, int level, uint32_t tick)
{
	run_stop = !level;
}
/* Reset input
 * Rising edge causes next step to be Step 1
 */
void reset_input(int gpio, int level, uint32_t tick)
{
	if (level == 1)	step_one = TRUE;
}
/* Hold input
 * nOT QUITE SURE WHAT TO DO WITHH THIS YET!!
 */
void hold_input(int gpio, int level, uint32_t tick)
{
	if(level == 1){
	}

}
/*
* Manually force Step One to tbe the next step 
*/
void set_step_one(void)
{
	step_one = TRUE;	
}
/* Function called to advance the sequence on to the next step */
void next_step(void)
{
	uint32_t current_tick = gpioTick();
	// first ever time it's run, there will be
	// no value for step_tick, to the length of
	// the first step will be indeterminate. So,
	// set it to 200ms.
	if (step_tick == 0) step_ticks = 250000; else step_ticks = current_tick - step_tick;
	//log_msg("Step Ticks: %d\n",step_ticks);
	step_tick = current_tick;
	int previous_step, channel, track;
	/* look for something to do */
	//for (track = 0;track < MAX_TRACKS; track++){
	for (track = 0;track < last_track; track++){
		/* if this track is busy doing something else, then it won't advance to the next step */
		if (Europi.tracks[track].track_busy == FALSE){
			// Each track can be running at a Sub-Division of the main clock 
			// So, if we haven't reached the required repetitions for this track, don't move on to the next step
			if (++Europi.tracks[track].clock_divisor_counter >= Europi.tracks[track].clock_divisor){
				Europi.tracks[track].clock_divisor_counter = 0;
				/* Each Track has its own end point */
				previous_step = Europi.tracks[track].current_step;
				/* Switch behaviour depending on the direction this track moves in */
				switch (Europi.tracks[track].direction){
					case Forwards:
					default:
						if(++Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repeat_counter >= Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repetitions || step_one == TRUE){
						Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repeat_counter = 0;
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
									sGate.gate_type = Trigger;
									sGate.ratchets = 1;
									sGate.fill = 1;
									struct gate *pGate = malloc(sizeof(struct gate));
									memcpy(pGate, &sGate, sizeof(struct gate));
									if(pthread_create(&ThreadId, &detached_attr, &GateThread, pGate)){
										log_msg("Gate thread creation error\n");
									}
								}
							}
						}                
						break;
					case Backwards:
						if(++Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repeat_counter >= Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repetitions || step_one == TRUE){
						Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repeat_counter = 0;
							if(Europi.tracks[track].current_step == 0 && step_one == FALSE){
								Europi.tracks[track].current_step = Europi.tracks[track].last_step -1;
							}
							else if (Europi.tracks[track].current_step == 1 || step_one == TRUE){
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
									sGate.gate_type = Trigger;
									sGate.ratchets = 1;
									sGate.fill = 1;
									struct gate *pGate = malloc(sizeof(struct gate));
									memcpy(pGate, &sGate, sizeof(struct gate));
									if(pthread_create(&ThreadId, &detached_attr, &GateThread, pGate)){
										log_msg("Gate thread creation error\n");
									}
								}
							}
							else Europi.tracks[track].current_step--;
						}
					break;
					case Pendulum_F:
						/* Advances forwards until it reaches the last step */
						if(++Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repeat_counter >= Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repetitions || step_one == TRUE){
						Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repeat_counter = 0;
							if (++Europi.tracks[track].current_step >= Europi.tracks[track].last_step && step_one == FALSE){
								Europi.tracks[track].current_step--;
								Europi.tracks[track].direction = Pendulum_B;
							}
							else if (step_one == TRUE) {
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
									sGate.gate_type = Trigger;
									sGate.ratchets = 1;
									sGate.fill = 1;
									struct gate *pGate = malloc(sizeof(struct gate));
									memcpy(pGate, &sGate, sizeof(struct gate));
									if(pthread_create(&ThreadId, &detached_attr, &GateThread, pGate)){
										log_msg("Gate thread creation error\n");
									}
								}
							}
						}
					break;
					case Pendulum_B:
						if(++Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repeat_counter >= Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repetitions || step_one == TRUE){
						Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repeat_counter = 0;
							if(Europi.tracks[track].current_step == 0 || step_one == TRUE){
								Europi.tracks[track].direction = Pendulum_F;
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
									sGate.gate_type = Trigger;
									sGate.ratchets = 1;
									sGate.fill = 1;
									struct gate *pGate = malloc(sizeof(struct gate));
									memcpy(pGate, &sGate, sizeof(struct gate));
									if(pthread_create(&ThreadId, &detached_attr, &GateThread, pGate)){
										log_msg("Gate thread creation error\n");
									}
								}
							}
							else Europi.tracks[track].current_step--;
						}
					break;
					case Random:
						if(++Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repeat_counter >= Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repetitions || step_one == TRUE){
						Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repeat_counter = 0;
							Europi.tracks[track].current_step = rand() % Europi.tracks[track].last_step;
							if(Europi.tracks[track].current_step == 0 || step_one == TRUE){
								Europi.tracks[track].current_step = 0;
								if ((is_europi == TRUE) && (track == 0)){
									/* Track 0 Channel 1 will have the GPIO Handle for the PCF8574 channel 3 is Step 1 Out*/
									struct gate sGate;
									sGate.track = track;
									sGate.i2c_handle = Europi.tracks[0].channels[GATE_OUT].i2c_handle;
									sGate.i2c_address = Europi.tracks[0].channels[GATE_OUT].i2c_address;
									sGate.i2c_channel = STEP1_OUT;
									sGate.i2c_device = DEV_PCF8574;
									sGate.gate_type = Trigger;
									sGate.ratchets = 1;
									sGate.fill = 1;
									struct gate *pGate = malloc(sizeof(struct gate));
									memcpy(pGate, &sGate, sizeof(struct gate));
									if(pthread_create(&ThreadId, &detached_attr, &GateThread, pGate)){
										log_msg("Gate thread creation error\n");
									}
								}
							}
						}
					break;
				}
				/* Deal with the various different types of Analogue output
				* In General, this launches a thread to deal with anything
				* that isn't a simple static voltage, as this removes the
				* processing load from the main program loop
				*/
				if(Europi.tracks[track].channels[CV_OUT].enabled == TRUE) {
					//log_msg("CV Out, Trk: %d Chnl: %d, Val: %d\n",track,CV_OUT,Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value);
					switch(Europi.tracks[track].channels[CV_OUT].type){
						default:
						case CHNL_TYPE_CV:
							// CV Channels implement static CV & Slew
							if(Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type == Off){
								// No Slew - just set the output CV
								//log_msg("SnglChnlWr, Trk: %d Chnl: %d, i2c: %d Val: %d\n",track,CV_OUT,Europi.tracks[track].channels[CV_OUT].i2c_channel,Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value);
								DACSingleChannelWrite(track,Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value);
							}
							else {
								// Slew
								//log_msg("slew\n");
								struct slew sSlew;
								sSlew.track = track;
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
							}
						break;
						case CHNL_TYPE_MIDI:
							MIDISingleChannelWrite(Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_channel, 0x40, Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value);   
						break;
					}
								
				}
				// MOD Channel
				if(Europi.tracks[track].channels[MOD_OUT].enabled == TRUE) {
					switch(Europi.tracks[track].channels[MOD_OUT].type){
						default:
						case CHNL_TYPE_MOD:
							if(Europi.tracks[track].channels[MOD_OUT].steps[Europi.tracks[track].current_step].mod_shape == Mod_Off){
								// Just Output Zero - no need for the thread launch overhead
								DACSingleChannelWrite(track,Europi.tracks[track].channels[MOD_OUT].i2c_handle, Europi.tracks[track].channels[MOD_OUT].i2c_address, Europi.tracks[track].channels[MOD_OUT].i2c_channel, 0);
							}
							else{
								// Launch a modulation thread for this step
								struct modstep sMod;
								sMod.track = track;
								sMod.i2c_handle = Europi.tracks[track].channels[MOD_OUT].i2c_handle;
								sMod.i2c_address = Europi.tracks[track].channels[MOD_OUT].i2c_address;
								sMod.i2c_channel =  Europi.tracks[track].channels[MOD_OUT].i2c_channel;
								sMod.i2c_device = Europi.tracks[track].channels[MOD_OUT].i2c_device;
								sMod.mod_shape = Europi.tracks[track].channels[MOD_OUT].steps[Europi.tracks[track].current_step].mod_shape;
								sMod.min = Europi.tracks[track].channels[MOD_OUT].steps[Europi.tracks[track].current_step].min;
								sMod.max = Europi.tracks[track].channels[MOD_OUT].steps[Europi.tracks[track].current_step].max;
								sMod.duty_cycle = Europi.tracks[track].channels[MOD_OUT].steps[Europi.tracks[track].current_step].duty_cycle;
								struct modstep *pModstep = malloc(sizeof(struct modstep));
								memcpy(pModstep, &sMod, sizeof(struct modstep));
								if(pthread_create(&ThreadId, &detached_attr, &ModThread, pModstep)){
								}
							}
						break;
					}
								
				}
				// Gate Channel
				/* launch a thread to handle the gate function for each channel / step */
				if (Europi.tracks[track].channels[GATE_OUT].enabled == TRUE ){
					struct gate sGate;
					sGate.track = track;
					sGate.i2c_handle = Europi.tracks[track].channels[GATE_OUT].i2c_handle;
					sGate.i2c_address = Europi.tracks[track].channels[GATE_OUT].i2c_address;
					sGate.i2c_channel =  Europi.tracks[track].channels[GATE_OUT].i2c_channel;
					sGate.i2c_device = Europi.tracks[track].channels[GATE_OUT].i2c_device;
					sGate.ratchets = Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].ratchets;
					sGate.gate_type = Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].gate_type;
					sGate.fill = Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].fill;
					struct gate *pGate = malloc(sizeof(struct gate));
					memcpy(pGate, &sGate, sizeof(struct gate));
					if(pthread_create(&ThreadId, &detached_attr, &GateThread, pGate)){
						log_msg("Gate thread creation error\n");
					}
				}
			}
		}
	}
	/* anything that needed resetting back to step 1 will have done so */
	if (step_one == TRUE) step_one = FALSE;
}

/*
 * ADSR thread.
 * ADSR Profile ramp generator. Always starts and ends at Zero, and
 * the sustain level is expressed as a Percentage of the max Attack value
 */
void *AdsrThread(void *arg)
{
	struct adsr *pADSR = (struct adsr *)arg;
	uint16_t this_value;
	uint32_t start_tick = gpioTick();
    uint16_t sus_level;
	int step_size;
	int num_steps;
	int i;
    // Set Track Busy flag
    Europi.tracks[pADSR->track].track_busy = TRUE;
    sus_level = ((pADSR->a_end_value - pADSR->a_start_value) * pADSR->s_level)/100;
    // A-ramp
    this_value = pADSR->a_start_value;
    DACSingleChannelWrite(pADSR->track,pADSR->i2c_handle, pADSR->i2c_address, pADSR->i2c_channel, this_value);
    if(pADSR->a_length >= slew_interval){
        step_size = (pADSR->a_end_value - pADSR->a_start_value) / (pADSR->a_length / slew_interval);
        num_steps = (pADSR->a_end_value - pADSR->a_start_value) / step_size;
        if (step_size == 0) {step_size = 1; num_steps = (pADSR->a_end_value - pADSR->a_start_value);}
        for(i = 0;i < num_steps; i++){
            usleep(slew_interval / 2);
            // Rail clamp
            if((this_value += step_size) <= Europi.tracks[pADSR->track].channels[CV_OUT].scale_max){
                DACSingleChannelWrite(pADSR->track,pADSR->i2c_handle, pADSR->i2c_address, pADSR->i2c_channel, this_value);
            }
            else {
                DACSingleChannelWrite(pADSR->track,pADSR->i2c_handle, pADSR->i2c_address, pADSR->i2c_channel, Europi.tracks[pADSR->track].channels[CV_OUT].scale_max);
            }
        }
    }
    // D-ramp
    this_value = pADSR->a_end_value;
    DACSingleChannelWrite(pADSR->track,pADSR->i2c_handle, pADSR->i2c_address, pADSR->i2c_channel, this_value);
    if(pADSR->d_length >= slew_interval){
        step_size = (pADSR->a_end_value - sus_level) / (pADSR->d_length / slew_interval);
        num_steps = (pADSR->a_end_value - sus_level) / step_size;
        if (step_size == 0) {step_size = 1; num_steps = (pADSR->a_end_value - sus_level);}
        for(i = 0;i < num_steps; i++){
            usleep(slew_interval / 2);
            // Rail clamp
            if((this_value -= step_size) >= Europi.tracks[pADSR->track].channels[CV_OUT].scale_zero){
                DACSingleChannelWrite(pADSR->track,pADSR->i2c_handle, pADSR->i2c_address, pADSR->i2c_channel, this_value);
            }
            else {
                DACSingleChannelWrite(pADSR->track,pADSR->i2c_handle, pADSR->i2c_address, pADSR->i2c_channel, Europi.tracks[pADSR->track].channels[CV_OUT].scale_zero);
            }
        }
    }
    // Sustain time
    usleep(pADSR->s_length);
    
    // Release ramp
    this_value = sus_level;
    DACSingleChannelWrite(pADSR->track,pADSR->i2c_handle, pADSR->i2c_address, pADSR->i2c_channel, this_value);
    if(pADSR->r_length >= slew_interval){
        step_size = (sus_level - pADSR->r_end_value) / (pADSR->r_length / slew_interval);
        num_steps = (sus_level - pADSR->r_end_value) / step_size;
        if (step_size == 0) {step_size = 1; num_steps = (sus_level - pADSR->r_end_value);}
        for(i = 0;i < num_steps; i++){
            usleep(slew_interval / 2);
            // Rail clamp
            if((this_value -= step_size) >= Europi.tracks[pADSR->track].channels[CV_OUT].scale_zero){
                DACSingleChannelWrite(pADSR->track,pADSR->i2c_handle, pADSR->i2c_address, pADSR->i2c_channel, this_value);
            }
            else {
                DACSingleChannelWrite(pADSR->track,pADSR->i2c_handle, pADSR->i2c_address, pADSR->i2c_channel, Europi.tracks[pADSR->track].channels[CV_OUT].scale_zero);
            }
        }
    }
    
    DACSingleChannelWrite(pADSR->track,pADSR->i2c_handle, pADSR->i2c_address, pADSR->i2c_channel, pADSR->r_end_value);
    // Clear Track Busy flag
    Europi.tracks[pADSR->track].track_busy = FALSE;
	free(pADSR);
	return(0);
}

/*
 * Attack-Decay thread.
 * Simple AD ramp, which can be set to OneShot or Repeat
 */
void *AdThread(void *arg)
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
		DACSingleChannelWrite(pAD->track,pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, this_value);
		if(pAD->a_length >= slew_interval){
			step_size = (pAD->a_end_value - pAD->a_start_value) / (pAD->a_length / slew_interval);
			num_steps = (pAD->a_end_value - pAD->a_start_value) / step_size;
			if (step_size == 0) {step_size = 1; num_steps = (pAD->a_end_value - pAD->a_start_value);}
			for(i = 0;i < num_steps; i++){
				usleep(slew_interval / 2);
				// Rail clamp
				if((this_value += step_size) <= 60000){
					DACSingleChannelWrite(pAD->track,pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, this_value);
				}
				else {
					DACSingleChannelWrite(pAD->track,pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, 60000);
				}
			}
		}
		// D-ramp
		this_value = pAD->a_end_value;
		DACSingleChannelWrite(pAD->track,pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, this_value);
		if(pAD->d_length >= slew_interval){
			step_size = (pAD->a_end_value - pAD->d_end_value) / (pAD->d_length / slew_interval);
			num_steps = (pAD->a_end_value - pAD->d_end_value) / step_size;
			if (step_size == 0) {step_size = 1; num_steps = (pAD->a_end_value - pAD->d_end_value);}
			for(i = 0;i < num_steps; i++){
				usleep(slew_interval / 2);
				// Rail clamp
				if((this_value -= step_size) >= 0){
					DACSingleChannelWrite(pAD->track,pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, this_value);
				}
				else {
					DACSingleChannelWrite(pAD->track,pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, 0);
				}
			}
		}
		DACSingleChannelWrite(pAD->track,pAD->i2c_handle, pAD->i2c_address, pAD->i2c_channel, pAD->d_end_value);
		// Clear Track Busy flag
		Europi.tracks[pAD->track].track_busy = FALSE;
	}	
	free(pAD);
	return(0);
}

/*
 * Slew Thread - launched for each Track / Step that
 * has a slew value other than SLEW_OFF. This thread
 * function lives just to perform the slew, then
 * ends itself. It executes the slew by stepping through
 * a pre-calculated array for each slew shape (Linear, Exponential etc)
 */
void *SlewThread(void *arg)
{
	struct slew *pSlew = (struct slew *)arg;
	uint16_t this_value = pSlew->start_value;
	uint32_t start_tick = gpioTick();
	int step_size;
    int num_steps;
    float profile_offset;
    float profile_index;
    float pitch_jump;
    int slew_profile;

	if (pSlew->slew_length == 0) {
		// No slew length set, so just output this step and close the thread
		DACSingleChannelWrite(pSlew->track,pSlew->i2c_handle, pSlew->i2c_address, pSlew->i2c_channel, pSlew->end_value);
		free(pSlew);
		return(0);
	}
	
	if ((pSlew->end_value > pSlew->start_value) && ((pSlew->slew_shape == Rising) || (pSlew->slew_shape == Both))) {
		// Glide UP
        switch(pSlew->slew_type){
            case Exponential:
                slew_profile = 1;
                break;
            case RevExp:
                slew_profile = 2;
                break;
            case Linear:
            default:
                slew_profile = 0;
                break;
        }
        num_steps = pSlew->slew_length / slew_interval;
        profile_index = (float)100 / (float)num_steps;
		pitch_jump = pSlew->end_value - pSlew->start_value;
        profile_offset = 0;
        while((num_steps > 0) && (this_value <= pSlew->end_value)){
            this_value = pSlew->start_value + ((slew_profiles[0][slew_profile][(int)profile_offset] / (float)100) * pitch_jump);
            DACSingleChannelWrite(pSlew->track,pSlew->i2c_handle, pSlew->i2c_address, pSlew->i2c_channel, this_value);
			usleep(slew_interval / 2);		
            profile_offset += profile_index;
            num_steps--;
        }
        DACSingleChannelWrite(pSlew->track,pSlew->i2c_handle, pSlew->i2c_address, pSlew->i2c_channel, pSlew->end_value);
	}
	else if ((pSlew->end_value < pSlew->start_value) && ((pSlew->slew_shape == Falling) || (pSlew->slew_shape == Both))){
		// Glide Down
        switch(pSlew->slew_type){
            case Exponential:
                slew_profile = 4;
                break;
            case RevExp:
                slew_profile = 5;
                break;
            case Linear:
            default:
                slew_profile = 3;
                break;
        }
        num_steps = pSlew->slew_length / slew_interval;
        profile_index = (float)100 / (float)num_steps;
		pitch_jump = pSlew->start_value - pSlew->end_value;
        profile_offset = 0;
        while((num_steps > 0) && (this_value >= pSlew->end_value)){
            this_value = pSlew->end_value + ((slew_profiles[1][slew_profile][(int)profile_offset] / (float)100) * pitch_jump);
            DACSingleChannelWrite(pSlew->track,pSlew->i2c_handle, pSlew->i2c_address, pSlew->i2c_channel, this_value);
			usleep(slew_interval / 2);		
            profile_offset += profile_index;
            num_steps--;
        }
        DACSingleChannelWrite(pSlew->track,pSlew->i2c_handle, pSlew->i2c_address, pSlew->i2c_channel, pSlew->end_value);
	}
	else {
		// Slew set, but Rising or Falling are off, so just output the end value
		DACSingleChannelWrite(pSlew->track,pSlew->i2c_handle, pSlew->i2c_address, pSlew->i2c_channel, pSlew->end_value);
	}
    free(pSlew);
	return(0);
}

/*
 * Modulation thread
 * Is passed a structure containing everything it
 * needs to know
 */
void *ModThread(void *arg)
{
	struct modstep *pMod = (struct modstep *)arg;
	uint16_t this_value;
	int step_size;
	int num_steps;
	int a_length;
	int d_length;
	int i;
	switch (pMod->mod_shape) {
		default:
		case Mod_Off:
			// should never get here, but just in case, output the min value
			DACSingleChannelWrite(pMod->track,pMod->i2c_handle, pMod->i2c_address, pMod->i2c_channel, pMod->min);
		break;
		case Mod_Square:
			// Output the Max value, sleep according to Duty Cyle, outpu minimum
			DACSingleChannelWrite(pMod->track,pMod->i2c_handle, pMod->i2c_address, pMod->i2c_channel, pMod->max);
			//log_msg("Tr %d, Hn %d, Ad %d, Ch %d, Val %d\n",pMod->track,pMod->i2c_handle,pMod->i2c_address,pMod->i2c_channel,pMod->max);
			usleep((step_ticks * pMod->duty_cycle)/100);
			DACSingleChannelWrite(pMod->track,pMod->i2c_handle, pMod->i2c_address, pMod->i2c_channel, pMod->min);
			//log_msg("Tr %d, Hn %d, Ad %d, Ch %d, Val %d\n",pMod->track,pMod->i2c_handle,pMod->i2c_address,pMod->i2c_channel,pMod->min);
		break;
		case Mod_Triangle:
			// Attack & Decay ramp lengths depend on Duty cycle
			a_length = (step_ticks * pMod->duty_cycle)/100; 
			d_length = step_ticks - a_length;
			// A-ramp
			this_value = pMod->min;
			DACSingleChannelWrite(pMod->track,pMod->i2c_handle, pMod->i2c_address, pMod->i2c_channel, this_value);
			if(a_length >= slew_interval){
				step_size = (pMod->max - pMod->min) / (a_length / slew_interval);
				num_steps = (pMod->max - pMod->min) / step_size;
				if (step_size == 0) {step_size = 1; num_steps = (pMod->max - pMod->min);}
				for(i = 0;i < num_steps; i++){
					usleep(slew_interval / 2);
					// Rail clamp
					if((this_value += step_size) <= pMod->max){
						DACSingleChannelWrite(pMod->track,pMod->i2c_handle, pMod->i2c_address, pMod->i2c_channel, this_value);
					}
					else {
						DACSingleChannelWrite(pMod->track,pMod->i2c_handle, pMod->i2c_address, pMod->i2c_channel, pMod->max);
					}
				}
			}
			// D-ramp
			this_value = pMod->max;
			DACSingleChannelWrite(pMod->track,pMod->i2c_handle, pMod->i2c_address, pMod->i2c_channel, this_value);
			if(d_length >= slew_interval){
				step_size = (pMod->max - pMod->min) / (d_length / slew_interval);
				num_steps = (pMod->max - pMod->min) / step_size;
				if (step_size == 0) {step_size = 1; num_steps = (pMod->max - pMod->min);}
				for(i = 0;i < num_steps; i++){
					usleep(slew_interval / 2);
					// Rail clamp
					if((this_value -= step_size) >= pMod->min){
						DACSingleChannelWrite(pMod->track,pMod->i2c_handle, pMod->i2c_address, pMod->i2c_channel, this_value);
					}
					else {
						DACSingleChannelWrite(pMod->track,pMod->i2c_handle, pMod->i2c_address, pMod->i2c_channel, pMod->min);
					}
				}
			}
			DACSingleChannelWrite(pMod->track,pMod->i2c_handle, pMod->i2c_address, pMod->i2c_channel, pMod->min);
		break;
	}
	free(pMod);
	return(0);
}

/*
 * Gate Thread - launched for each Track / Step that
 * has a Gate/Trigger. For normal Gsates, it uses gate_type 
 * to determine the length of the pulse. 
 * If a ratchet value is set, then it will output a series of
 * pulses timed to fit within the known length of the Step. It
 * uses a Fill value to determine how many of these ratchets actually
 * sound. If Fill >= ratchets, then all will sound, otherwise it
 * looks up the fill sequence in a pre-calculated Euclidian fill
 * table, which gives quite musical rhythmic fills up to 32 ratchets 
 * per step. Above this, it just outputs 100% ratchets.
 * 
 * The thread lives just to perform the gate, then ends itself
 *
 * Track swing perceentage is an amount from 50% (no swing) to 90%
 * which delays the gate on Even-numbered steps by a percentage of
 * the overall step length
 */
void *GateThread(void *arg)
{
	struct gate *pGate = (struct gate *)arg;
	uint32_t step_ticks_local = step_ticks;	
    uint32_t swing_delay;
	// If global tuning is on, ignore all Gate info, just turn all the gates ON and quit
    if(TuningOn == TRUE){
        GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,1); 
        free(pGate);
        return(0);
    }
	if ((Europi.tracks[pGate->track].swing_percent > 50) && (Europi.tracks[pGate->track].swing_percent <= 90)) {
		//Track has a swing percent set
		if(Europi.tracks[pGate->track].current_step % 2 == 0){
			// Even-numbered step, with swing, so delay the onset of the Gate by the swing percentage
			swing_delay = (step_ticks_local * (Europi.tracks[pGate->track].swing_percent - 50)) / 100;
			// Reduce the remaiining step length by thhis amount (so that everything fits)
			step_ticks_local -= swing_delay;
			log_msg("Trk: %d, Pct: %d, STicks: %d, S_delay: %d",pGate->track,Europi.tracks[pGate->track].swing_percent,step_ticks,swing_delay);
			usleep(swing_delay);
		}
	}
	//log_msg("GateThread H: %d, Ch: %d, Dev: %d\n",pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device);
    if (pGate->ratchets <= 1){
        //Normal Gate
        switch(pGate->gate_type){
            case Gate_Off:
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,0);
            break;
            case Gate_On:
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,1);
            break;
            case Trigger:
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,1);
                usleep(10000);  //10ms Pulse
                /* Gate Off */
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,0);
            break;
            case Gate_25:
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,1);
                usleep((step_ticks_local * 25)/100);
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,0);
            break;
            case Gate_50:
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,1);
                usleep((step_ticks_local * 50)/100);
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,0);
            break;
            case Gate_75:
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,1);
                usleep((step_ticks_local * 75)/100);
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,0);
            break;
            case Gate_95:
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,1);
                usleep((step_ticks_local * 95)/100);
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,0);
            break;
        }
    }
    // Ratchetting Gate
    else {
        /* this step is to be re-triggered, so work out the sleep length between triggers 
         * The measured Function Calling overhead averages at around 10k to 20k ticks
         * whereas a typical Step length would be between 200k and, perhaps, 1m2, so taking
         * off 10k for the function calling overhead feels about right
         */
        int sleep_time = ((step_ticks_local - 10000) / pGate->ratchets)/2;

        int i;
        for (i = 0; i < pGate->ratchets; i++){
            // Ratchet is ON
            if(polyrhythm(pGate->ratchets,pGate->fill,i)){
                /* Gate On */
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,1);
                usleep(sleep_time);
                /* Gate Off */
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,0);
                usleep(sleep_time);
            }
            else {
                // Ratchet is OFF - make sure Gate is OFF just in case
                // an Off Ratchet follows an ON gate!
                GATESingleOutput(pGate->i2c_handle, pGate->i2c_channel,pGate->i2c_device,0);
                usleep(sleep_time * 2);
            }
        }
    }
    free(pGate);
	return(0);
}

/*
 * MIDI Thread - Joinable thread launched
 * for each MIDI Minion (ie up to 4 of these
 * could be running)
 */
void *MidiThread(void *arg)
{
    struct midiChnl *pMidiChnl = (struct midiChnl *)arg;
	int fd = pMidiChnl->i2c_handle;
    int ret_val;
    while (!ThreadEnd){
        if(i2cReadByteData(fd,SC16IS750_RXLVL) > 0) {
            ret_val = i2cReadByteData(fd,SC16IS750_RHR); 
        }
    }
    return NULL;
}
/*
 * Delay thread, which sleeps for the passed time
 * then applies the passed bit-mask to the ActiveOverlays Global
 * This is ued to Eg. turn off temporary small Dialogs
 * after an amount of time
 */
void *OvlTimerThread(void *arg){
	struct ovl_timer *pOvlTimer = (struct ovl_timer *)arg;
	usleep(pOvlTimer->sleeptime);
	ActiveOverlays &= pOvlTimer->overlays;
	return NULL;
}
/*
 * STARTUP
 * Main initialisation function, called once when the
 * prog starts.
 */
int startup(void)
{
	 // Initial state of the Screen Elements (Menus etc)
	 ClearScreenOverlays();
	 DisplayPage = GridView;
	 // Initialise the Deatched pThread attribute
	pthread_attr_init(&detached_attr);
	pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
	int i;
    // Make sure MIDI Listener threads aren't initialised
    // they will be initialised when MIDI Minion(s) are detected
    for(i=0;i<4;i++) {
            midiThreadId[i] = (pthread_t)NULL;
            midiThreadLaunched[i] = FALSE;
    }

    
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
	gpioSetMode(MASTER_CLK, PI_OUTPUT);
	gpioSetMode(INT_CLK_OUT, PI_OUTPUT);
	gpioSetMode(CLOCK_IN, PI_INPUT);
	//gpioGlitchFilter(CLOCK_IN,100);				/* EXT_CLK has to be at the new level for 100uS before it is registered */
	gpioSetMode(RUNSTOP_IN, PI_INPUT);
	gpioSetPullUpDown(RUNSTOP_IN, PI_PUD_UP);
	gpioGlitchFilter(RUNSTOP_IN,100);
	gpioSetMode(RESET_IN, PI_INPUT);
	gpioGlitchFilter(RESET_IN,100);
	gpioSetMode(HOLD_IN, PI_INPUT);
	gpioSetPullUpDown(HOLD_IN, PI_PUD_UP);
	// Register callback routines
	gpioSetAlertFunc(BUTTON1_IN, button_1);
	gpioSetAlertFunc(BUTTON2_IN, button_2);
	gpioSetAlertFunc(BUTTON3_IN, button_3);
	gpioSetAlertFunc(BUTTON4_IN, button_4);
	gpioSetAlertFunc(ENCODER_BTN, encoder_button);
	//gpioSetAlertFunc(TOUCH_INT, touch_interrupt);
	gpioSetAlertFunc(CLOCK_IN, main_clock);
	gpioSetAlertFunc(RUNSTOP_IN, runstop_input);
	gpioSetAlertFunc(RESET_IN, reset_input);
	gpioSetAlertFunc(HOLD_IN, hold_input);

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
    // Initial function of each soft-button
    btnA_func = btnA_quit;
    btnB_func = btnB_menu;
    btnC_func = btnC_bpm_dn;
    btnD_func = btnD_bpm_up;


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
	if (!impersonate_hw) {
		//DisableCursor();	// Cursor enabled when Hardware impersonation is ON
	}
	//font1 = LoadSpriteFont("resources/fonts/mecha.rbmf");
    KeyboardTexture = LoadTexture("resources/images/keyboard.png");
    DialogTexture = LoadTexture("resources/images/dialog.png");
	SmallDialogTexture = LoadTexture("resources/images/small_dialog.png");
    TextInputTexture = LoadTexture("resources/images/text_input.png");
    Text2chTexture = LoadTexture("resources/images/text_2ch.png");
    Text5chTexture = LoadTexture("resources/images/text_5ch.png");
	Text10chTexture = LoadTexture("resources/images/text_10ch.png");
    MainScreenTexture = LoadTexture("resources/images/main_screen.png");
    TopBarTexture = LoadTexture("resources/images/top_bar.png");
    ButtonBarTexture = LoadTexture("resources/images/button_bar.png");
    VerticalScrollBarTexture = LoadTexture("resources/images/vertical_scroll_bar.png");
    VerticalScrollBarShortTexture = LoadTexture("resources/images/vertical_scroll_bar_short.png");
    ScrollHandleTexture = LoadTexture("resources/images/scroll_handle.png");
	Splash = LoadTexture("resources/images/splash_screen.png");
	

	//Splash screen
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
	seq_new();
	//load_sequence("resources/sequences/123test");
	//reapply the current hardware config to loaded sequence
	//reapply_config();
	/* Start the internal sequencer clock */
	run_stop = STOP;		/* master clock is running, but step generator is halted */
	select_first_track();	// Default select the first enabled track
	// Start the main internal clock, with 50% duty cycle
	// and register a callback against its state change
	gpioHardwarePWM(MASTER_CLK,clock_freq,500000);
	gpioSetAlertFunc(MASTER_CLK, master_clock);
	prog_running = 1;
	
    return(0);
}

/*
 * ReapplyConfig
 * the problem with loadcing a sequence direct from disk is that this also
 * saved the hardware configuration at that point in time. So, if a sequence 
 * is re-loaded from disk, we need to re-apply the CURRENT hardware 
 * configuration in order to ensure compatibility with the current hardware
 * config. In practice, this just involves enabling or disabling channels
 */
void reapply_config(void) {
	int track;
   for(track=0;track<MAX_TRACKS;track++){
		Europi.tracks[track].channels[CV_OUT].enabled = Europi_hw.hw_tracks[track].hw_channels[CV_OUT].enabled;
		Europi.tracks[track].channels[GATE_OUT].enabled = Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].enabled;
   }
}
/* SHUTDOWN
 * Main program shutdown routine - called 
 * once when the prog closes. Aims to shut 
 * all gates down, unload all textures, the
 * PIGPIO library etc.
 * Things to do as the prog closess */
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
			DACSingleChannelWrite(track,Europi.tracks[track].channels[0].i2c_handle, Europi.tracks[track].channels[0].i2c_address, Europi.tracks[track].channels[0].i2c_channel, Europi.tracks[track].channels[0].scale_zero);
		}
		/* set the Gate State for each channel to OFF*/
		if (Europi.tracks[track].channels[GATE_OUT].enabled == TRUE ){
			GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x00);
		}
        /* Note the current scale_zero and scale_max values in the Hardware config structure */
       Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_zero = Europi.tracks[track].channels[CV_OUT].scale_zero;
       Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_max = Europi.tracks[track].channels[CV_OUT].scale_max;
	}
    /* Save the current Hardware config */
    FILE * file = fopen("resources/hardware.conf","wb");
    if (file != NULL) {
        fwrite(&Europi_hw,sizeof(struct europi_hw),1,file);
        fclose(file);
    }
	/* Raylib de-initialisation */
	UnloadTexture(Splash);
    UnloadTexture(ScrollHandleTexture);
	UnloadTexture(VerticalScrollBarShortTexture);
    UnloadTexture(VerticalScrollBarTexture);
    UnloadTexture(ButtonBarTexture);
    UnloadTexture(TopBarTexture);
    UnloadTexture(MainScreenTexture);
    UnloadTexture(Text10chTexture);
    UnloadTexture(Text5chTexture);
    UnloadTexture(Text2chTexture);
    UnloadTexture(TextInputTexture);
	UnloadTexture(SmallDialogTexture);
    UnloadTexture(DialogTexture);
    UnloadTexture(KeyboardTexture);
	//UnloadSpriteFont(font1);
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
   char buf[50];
	if (print_messages == TRUE){
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	fprintf(stderr, "%s", buf);
    // Also logs messages to a circular buffer
    // which holds the most recent 10 messages
    snprintf(debug_messages[next_debug_slot],50,"%s",buf);
    next_debug_slot++;
    if(next_debug_slot >= 10) next_debug_slot = 0;
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
            select_next_track(dir);
			break;
		case step_select:
            select_next_step(dir);
            break;
		case set_zerolevel:
				if(dir == 1){
					int track=0;
					while(track < MAX_TRACKS){
						if(Europi.tracks[track].selected == TRUE){
							if(Europi.tracks[track].channels[CV_OUT].scale_zero <= 65535-vel){
							Europi.tracks[track].channels[CV_OUT].scale_zero += vel;
							DACSingleChannelWrite(track,Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].scale_zero);
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
							DACSingleChannelWrite(track,Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].scale_zero);
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
							DACSingleChannelWrite(track,Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].scale_max);
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
							DACSingleChannelWrite(track,Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].scale_max);
							}
							break;
						}
						track++;
					}
					
				}
			break;
		case set_loop:
            set_loop_point(dir);
			break;
		case set_pitch:
            set_step_pitch(dir,vel);
            break;
		case set_quantise:
            select_next_quantisation(dir);
            break;
		case set_direction:
            select_next_direction(dir);
            break;
		case keyboard_input:
		{
            if ((dir == 1) && (kbd_char_selected < KBD_ROWS * KBD_COLS)) {
                kbd_char_selected++;
			}
            else {
                if(kbd_char_selected > 0){
                    kbd_char_selected--;
                }
            }
			
			break;
		}
		case file_open_focus:
		{
            if ((dir == 1) && (file_selected < (file_count-1))) {
                file_selected++;
                // If we have scrolled to the bottom of the list, but there 
                // are more files to display, scroll the list as we go
                if((file_selected - first_file) >= DLG_ROWS) first_file++;
			}
            else {
                if((dir == -1) && (file_selected > 0)){
                    file_selected--;
                    if(((file_selected - first_file) < 0) && (first_file > 0)) first_file--;
                }
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
				if(ActiveOverlays & ovl_SetZero) 	encoder_focus = set_zerolevel;
				else if (ActiveOverlays & ovl_SetTen)  encoder_focus = set_maxlevel;
				else if (ActiveOverlays & ovl_SetLoop) encoder_focus = set_loop;
				else if (ActiveOverlays & ovl_SetPitch) encoder_focus = step_select;
				else if (ActiveOverlays & ovl_SetQuantise) encoder_focus = set_quantise;
                else if (ActiveOverlays & ovl_SetDirection) encoder_focus = set_direction;
			break;
        case file_open_focus:{
            char filename[100];
            snprintf(filename, sizeof(filename), "resources/sequences/%s", files[file_selected]);
            load_sequence(filename);
            ClearScreenOverlays();
            buttonsDefault();
            ClearMenus();
            MenuSelectItem(0,0);
			VerticalScrollPercent = 0;
            encoder_focus = none;
            }
            break;
        case keyboard_input:{
            // Add this key to the input buffer
            int row,col;
                    row = kbd_char_selected / KBD_COLS;
                    col = kbd_char_selected % KBD_COLS;
                    //Add this to the input_txt buffer
                    //sprintf(input_txt,"%s%s", input_txt,kbd_chars[row][col]);
					strcat(input_txt,kbd_chars[row][col]);
            }
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
        case set_direction:
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
    btnA_state = level;
}

/* Button 2 pressed */
void button_2(int gpio, int level, uint32_t tick)
{
    btnB_state = level;
}
/* Button 3 pressed */
void button_3(int gpio, int level, uint32_t tick)
{
    btnC_state = level;
}
	
/* Button 4 pressed */
void button_4(int gpio, int level, uint32_t tick)
{
    btnD_state = level;
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
 * to the UART. The SC16IS750 UART has an internal scratchpad
 * register. By writing then checking a random value, we can 
 * be pretty certain we've got a MIDI Minion. If so, we can 
 * set the baud rate, clock divisor etc.
 */
int MidiMinionFinder(unsigned address)
{
	int retval;
    uint8_t rnd_val;
	uint8_t ret_val;
	unsigned i2cAddr;
	int mid_handle;
	i2cAddr = MID_BASE_ADDR | (address & 0x7);
	mid_handle = i2cOpen(1,i2cAddr,0);
    //log_msg("MINION Addr: %x, Handle: %d\n",i2cAddr, mid_handle);
	if (mid_handle < 0) return -1;
    /* just to make sure, write out something random
     * to one of the user registers, and check that
     * it is there, to prove we have an SC16IS750 present
     */
    rnd_val = rand() % 0xFFFF;
	if(i2cWriteByteData(mid_handle,SC16IS750_SPR,rnd_val) !=0) return -1;   // Return on write failure
    ret_val = i2cReadByteData(mid_handle,SC16IS750_SPR);
    if(ret_val != rnd_val) {
        i2cClose(mid_handle);   //for some reason this doesn't free up the handle for re-use but, hey ho.
        return -1;    
    }
    else {
        return mid_handle;  
    }	
}

/* 
 * Looks for an MCP23008 on the passed address. If one
 * exists, then it should be safe to assume that this is 
1 * a Minion, in which case it is safe to pass back a handle
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
 * MIDI Single Channel Out.
 * This takes a CV Value, Channel etc, same as a
 * CV Output, though quantises the Note voltage
 * to a MIDI Note value
 */
void MIDISingleChannelWrite(unsigned handle, uint8_t channel, uint8_t velocity, uint16_t voltage){
    uint8_t note;
	if(impersonate_hw == TRUE) return;
    note = pitch2midi(voltage);
    // log_msg("Handle: %d, Chnl: %d, Velocity: %d, MIDI Note: %d\n",handle,channel,velocity,note);
    // Note On
    i2cWriteByteData(handle,SC16IS750_IOSTATE,0x00);
    i2cWriteByteData(handle,SC16IS750_RHR,(0x90 | (channel & 0x0F)));
    i2cWriteByteData(handle,SC16IS750_RHR,note);
    i2cWriteByteData(handle,SC16IS750_RHR,velocity);
    i2cWriteByteData(handle,SC16IS750_IOSTATE,0xFF);
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
void DACSingleChannelWrite(int track, unsigned handle, uint8_t address, uint8_t channel, uint16_t voltage){
	uint16_t v_out;
	uint8_t ctrl_reg;
	int retval;
	if(impersonate_hw == TRUE) return;
    if(TuningOn == TRUE) {
        //Output the Global tuning voltage scaled by this Channel's scale factor
        voltage = scale_value(track,TuningVoltage);
    }
	//log_msg("%d, %d, %d, %d\n",handle,address,channel,voltage);
	ctrl_reg = (((address & 0xC) << 4) | 0x10) | ((channel << 1) & 0x06);
	//log_msg("handle: %0x, address: %0x, channel: %d, ctrl_reg: %02x, Voltage: %d\n",handle, address, channel,ctrl_reg,voltage);
	//swap MSB & LSB because i2cWriteWordData sends LSB first, but the DAC expects MSB first
	v_out = ((voltage >> 8) & 0x00FF) | ((voltage << 8) & 0xFF00);
	i2cWriteWordData(handle,ctrl_reg,v_out);
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
	//log_msg("GateSingleOutput H: %d, Chnl: %d, Dev: %d, Val: %d\n",handle,channel,Device,Value);
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
			// due to a cockup in the V2 Circuit, I wired the LEDs
			// in the reverse order for some reason, so the bit order 
			// also needs to be reversed
			//PCF8574_state &= ~(0x01 << (channel+4));
			PCF8574_state &= ~(0x80 >> channel);
		}
		else {
			// Set corresponding bit low
			PCF8574_state &= ~(0x01 << channel);
			// the equivalent in the MS Nibble needs to be high to turn the LED off
			//PCF8574_state |= (0x01 << (channel+4));
			PCF8574_state |= (0x80 >> channel);
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
	int error;
	struct modChnl sModChnl; 
	struct modChnl *pModChnl = malloc(sizeof(struct modChnl));

	/* Before we start, make sure all Tracks / Channels are disabled */
	for (track = 0; track < MAX_TRACKS;track++){
		Europi.tracks[track].selected = FALSE;
        Europi.tracks[track].direction = Forwards;
		Europi.tracks[track].channels[CV_OUT].enabled = FALSE;
		Europi.tracks[track].channels[MOD_OUT].enabled = FALSE;
		Europi.tracks[track].channels[GATE_OUT].enabled = FALSE;
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].enabled = FALSE;
        Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].enabled = FALSE;
	}
    last_track = 0;
	/*
	 * If impersonate_hw is set to TRUE, then this bypasses the 
	 * hardware checks, and pretends all hardware is present - this
	 * is useful when testing the software without the full hardware
	 * present
	 */
	 if (impersonate_hw == TRUE){
		 is_europi = TRUE;
		 log_msg("Impersonating Europi Hardware\n");
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
			Europi.tracks[track].channels[MOD_OUT].enabled = TRUE;
			Europi.tracks[track].channels[MOD_OUT].type = CHNL_TYPE_CV;
			Europi.tracks[track].channels[MOD_OUT].quantise = 0;			/* Quantization off by default */
			Europi.tracks[track].channels[MOD_OUT].i2c_handle = track;			
			Europi.tracks[track].channels[MOD_OUT].i2c_device = DEV_DAC8574;
			Europi.tracks[track].channels[MOD_OUT].i2c_address = 0x0;
			Europi.tracks[track].channels[MOD_OUT].i2c_channel = 0;		
			Europi.tracks[track].channels[MOD_OUT].scale_zero = 280;		/* Value required to generate zero volt output */
			Europi.tracks[track].channels[MOD_OUT].scale_max = 63000;		/* Value required to generate maximum output voltage */
			Europi.tracks[track].channels[MOD_OUT].transpose = 0;			/* fixed (transpose) voltage offset applied to this channel */
			Europi.tracks[track].channels[MOD_OUT].octaves = 10;			/* How many octaves are covered from scale_zero to scale_max */
			Europi.tracks[track].channels[MOD_OUT].vc_type = VOCT;
            Europi.tracks[track].channels[GATE_OUT].enabled = TRUE;
			Europi.tracks[track].channels[GATE_OUT].type = CHNL_TYPE_GATE;
			Europi.tracks[track].channels[GATE_OUT].i2c_handle = track;			
			Europi.tracks[track].channels[GATE_OUT].i2c_device = DEV_PCF8574;
			Europi.tracks[track].channels[GATE_OUT].i2c_channel = 0;
            
            /* note the equivalent in the Hardware structure */
			Europi_hw.hw_tracks[track].hw_channels[CV_OUT].enabled = TRUE;
			Europi_hw.hw_tracks[track].hw_channels[CV_OUT].type = CHNL_TYPE_CV;
			Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_handle = track;			
			Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_device = DEV_DAC8574;
			Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_address = 0x0;
			Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_channel = 0;		
			Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_zero = 280;		
            Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_max = 63000;		
			Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].enabled = TRUE;
			Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].type = CHNL_TYPE_MOD;
			Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].i2c_handle = track;			
			Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].i2c_device = DEV_DAC8574;
			Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].i2c_address = 0x0;
			Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].i2c_channel = 1;		
			Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].scale_zero = 280;		
            Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].scale_max = 63000;		
            Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].enabled = TRUE;
			Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].type = CHNL_TYPE_GATE;
			Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_handle = track;			
			Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_device = DEV_PCF8574;
			Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_channel = 0;

		 }
		 last_track = MAX_TRACKS;
		 select_track(0);
		 return;
	 }
	/* 
	 * Specifically look for a PCF8574 on address 0x38/0x20
	 * if one exists, then it's on the Europi, so the first
	 * two Tracks will be allocated to the Europi
	 */
	track = 0;
	address = 0x08;
	handle = EuropiFinder();
	if (handle >=0){
		/* we have a Europi - it supports 2 Tracks each with 3 channels (CV + MOD + GATE) */
        log_msg("Europi found on Address 08\n");
		is_europi = TRUE;
		/* As this is a Europi, then there should be a PCF8574 GPIO Expander on address 0x20 or 0x38 */
		pcf_addr = PCF_BASE_ADDR;
		pcf_handle = i2cOpen(1,pcf_addr,0);
		if(pcf_handle < 0){log_msg("failed to open PCF8574 associated with DAC8574 on Addr: 0x08");}
		if(pcf_handle >= 0) {
			/* Gates off, LEDs off */
			i2cWriteByte(pcf_handle, (unsigned)(0xF0));
            log_msg("DAC8574 Ad:%0x, Hndl:%0x, PCF8574 Ad:%0x Hndl:%0x\n",DAC_BASE_ADDR,handle,PCF_BASE_ADDR,pcf_handle);
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

		/* Track 0 Channel 1 = Mod Output */
		Europi.tracks[track].channels[MOD_OUT].enabled = TRUE;
		Europi.tracks[track].channels[MOD_OUT].type = CHNL_TYPE_MOD;
		Europi.tracks[track].channels[MOD_OUT].quantise = 0;		
		Europi.tracks[track].channels[MOD_OUT].i2c_handle = handle;			
		Europi.tracks[track].channels[MOD_OUT].i2c_device = DEV_DAC8574;
		Europi.tracks[track].channels[MOD_OUT].i2c_address = 0x08;
		Europi.tracks[track].channels[MOD_OUT].i2c_channel = 1;		
		Europi.tracks[track].channels[MOD_OUT].scale_zero = 280;		/* Value required to generate zero volt output */
		Europi.tracks[track].channels[MOD_OUT].scale_max = 63000;		/* Value required to generate maximum output voltage */
		Europi.tracks[track].channels[MOD_OUT].transpose = 0;			/* fixed (transpose) voltage offset applied to this channel */
		Europi.tracks[track].channels[MOD_OUT].octaves = 10;			/* How many octaves are covered from scale_zero to scale_max */
		Europi.tracks[track].channels[MOD_OUT].vc_type = VOCT;

		/* Track 0 channel 2 = Gate Output */
		Europi.tracks[track].channels[GATE_OUT].enabled = TRUE;
		Europi.tracks[track].channels[GATE_OUT].type = CHNL_TYPE_GATE;
		Europi.tracks[track].channels[GATE_OUT].i2c_handle = pcf_handle;			
		Europi.tracks[track].channels[GATE_OUT].i2c_device = DEV_PCF8574;
		Europi.tracks[track].channels[GATE_OUT].i2c_channel = 0;

        
        /* note the equivalent in the Hardware structure */
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].enabled = TRUE;
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].type = CHNL_TYPE_CV;
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_handle = handle;			
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_device = DEV_DAC8574;
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_address = 0x08;
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_channel = 0;		
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_zero = 280;		
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_max = 63000;		
		Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].enabled = TRUE;
        Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].type = CHNL_TYPE_CV;
        Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].i2c_handle = handle;			
        Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].i2c_device = DEV_DAC8574;
        Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].i2c_address = 0x08;
        Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].i2c_channel = 0;		
        Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].scale_zero = 280;		
        Europi_hw.hw_tracks[track].hw_channels[MOD_OUT].scale_max = 63000;	
        Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].enabled = TRUE;
        Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].type = CHNL_TYPE_GATE;
        Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_handle = pcf_handle;			
        Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_device = DEV_PCF8574;
        Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_channel = 0;

		/* Track 1 channel 0 = CV */
		track++;
		Europi.tracks[track].channels[CV_OUT].enabled = TRUE;
		Europi.tracks[track].channels[CV_OUT].type = CHNL_TYPE_CV;
		Europi.tracks[track].channels[CV_OUT].quantise = 0;		
		Europi.tracks[track].channels[CV_OUT].i2c_handle = handle;			
		Europi.tracks[track].channels[CV_OUT].i2c_device = DEV_DAC8574;
		Europi.tracks[track].channels[CV_OUT].i2c_address = 0x08;
		Europi.tracks[track].channels[CV_OUT].i2c_channel = 2;		
		Europi.tracks[track].channels[CV_OUT].scale_zero = 280;		/* Value required to generate zero volt output */
		Europi.tracks[track].channels[CV_OUT].scale_max = 63000;		/* Value required to generate maximum output voltage */
		Europi.tracks[track].channels[CV_OUT].transpose = 0;				/* fixed (transpose) voltage offset applied to this channel */
		Europi.tracks[track].channels[CV_OUT].octaves = 10;			/* How many octaves are covered from scale_zero to scale_max */
		Europi.tracks[track].channels[CV_OUT].vc_type = VOCT;
		/* Track 1 cchannel 1 = Mod Output */
		Europi.tracks[track].channels[MOD_OUT].enabled = TRUE;
		Europi.tracks[track].channels[MOD_OUT].type = CHNL_TYPE_MOD;
		Europi.tracks[track].channels[MOD_OUT].quantise = 0;		
		Europi.tracks[track].channels[MOD_OUT].i2c_handle = handle;			
		Europi.tracks[track].channels[MOD_OUT].i2c_device = DEV_DAC8574;
		Europi.tracks[track].channels[MOD_OUT].i2c_address = 0x08;
		Europi.tracks[track].channels[MOD_OUT].i2c_channel = 3;		
		Europi.tracks[track].channels[MOD_OUT].scale_zero = 280;		/* Value required to generate zero volt output */
		Europi.tracks[track].channels[MOD_OUT].scale_max = 63000;		/* Value required to generate maximum output voltage */
		Europi.tracks[track].channels[MOD_OUT].transpose = 0;				/* fixed (transpose) voltage offset applied to this channel */
		Europi.tracks[track].channels[MOD_OUT].octaves = 10;			/* How many octaves are covered from scale_zero to scale_max */
		Europi.tracks[track].channels[MOD_OUT].vc_type = VOCT;
		/* Track 1 channel 2 = Gate*/
		Europi.tracks[track].channels[GATE_OUT].enabled = TRUE;
		Europi.tracks[track].channels[GATE_OUT].type = CHNL_TYPE_GATE;
		Europi.tracks[track].channels[GATE_OUT].i2c_handle = pcf_handle;			
		Europi.tracks[track].channels[GATE_OUT].i2c_device = DEV_PCF8574;
		Europi.tracks[track].channels[GATE_OUT].i2c_channel = 1;		

        /* note the equivalent in the Hardware structure */
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].enabled = TRUE;
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].type = CHNL_TYPE_CV;
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_handle = handle;			
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_device = DEV_DAC8574;
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_address = 0x08;
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_channel = 1;		
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_zero = 280;		
        Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_max = 63000;		
        Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].enabled = TRUE;
        Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].type = CHNL_TYPE_GATE;
        Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_handle = pcf_handle;			
        Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_device = DEV_PCF8574;
        Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_channel = 1;
		
		/* Channels 3 & 4 of the PCF8574 are the Clock and Step 1 out 
		 * no need to set them to anything specific, and we don't really
		 * want them appearing as additional tracks*/
		track++;	/* Minion tracks will therefore start from Track 2 */
	}
	else {
		log_msg("No Europi hardware found\n");
	}
	/* 
	 * scan for Minions - these will be on addresses
	 * 0 - 7. Each Minion supports 4 Tracks
	 */
	for (address=0;address<=7;address++){
		handle = MinionFinder(address);
		if(handle >= 0){
			log_msg("Minion Found on Address %d\n",address);
			/* Get a handle to the associated MCP23008 */
			mcp_addr = MCP_BASE_ADDR | (address & 0x7);	
			gpio_handle = i2cOpen(1,mcp_addr,0);
            log_msg("Minion DAC8574 Addr:%0x, Hndl:%0x, MCP23008 Addr:%0x Hndl:%0x\n",DAC_BASE_ADDR | (address & 0x3),handle,mcp_addr,gpio_handle);
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
				Europi.tracks[track].channels[CV_OUT].scale_zero = 0x0000;	/* Value required to generate zero volt output */
				Europi.tracks[track].channels[CV_OUT].scale_max = 63000;		/* Value required to generate maximum output voltage NOTE: This needs to be overwritten during configuration*/
				Europi.tracks[track].channels[CV_OUT].transpose = 0;				/* fixed (transpose) voltage offset applied to this channel */
				Europi.tracks[track].channels[CV_OUT].octaves = 10;			/* How many octaves are covered from scale_zero to scale_max */
				Europi.tracks[track].channels[CV_OUT].vc_type = VOCT;

                /* note the equivalent in the Hardware structure */
                Europi_hw.hw_tracks[track].hw_channels[CV_OUT].enabled = TRUE;
                Europi_hw.hw_tracks[track].hw_channels[CV_OUT].type = CHNL_TYPE_CV;
                Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_handle = handle;			
                Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_device = DEV_DAC8574;
                Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_address = address;
                Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_channel = i;		
                Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_zero = 0x0000;		
                Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_max = 63000;		

				/* Channel 1 = Gate Output*/
				if (gpio_handle >= 0) {
					Europi.tracks[track].channels[GATE_OUT].enabled = TRUE;
					Europi.tracks[track].channels[GATE_OUT].type = CHNL_TYPE_GATE;
					Europi.tracks[track].channels[GATE_OUT].i2c_handle = gpio_handle;			
					Europi.tracks[track].channels[GATE_OUT].i2c_device = DEV_MCP23008;
					Europi.tracks[track].channels[GATE_OUT].i2c_channel = i; 		
                    /* note the equivalent in the Hardware structure */
                    Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].enabled = TRUE;
                    Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].type = CHNL_TYPE_GATE;
                    Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_handle = gpio_handle;			
                    Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_device = DEV_MCP23008;
                    Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_channel = i;
                    
				}
				else {
					/* for some reason we couldn't get a handle to the MCP23008, so disable the Gate channel */
					Europi.tracks[track].channels[GATE_OUT].enabled = FALSE;
					Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].enabled = FALSE;
				}
				track++;
			}	
		}
	}
    /* 
     * Scan for MIDI Minions. There are 4 possible i2c addresses that 
     * might have MIDI Minions on them: 52,
     */
    int i=0;
    for (i=0;i<=3;i++){
        switch(i){
            case 0:
                address=0x50;
                break;
            case 1:
                address=0x51;
                break;
            case 2:
                address=0x54;
                break;
            case 3:
                address=0x55;
                break;            
        }
        handle = MidiMinionFinder(address);
		if(handle >= 0){
			log_msg("MIDI Minion Found on Address %d\n",i);
            log_msg("MIDI Minion SC16IS75 Addr:%0x, Handle:%d\n",address,handle);
            // Set the Baud Rate divisor = 4
            // Prescaler is set to 1, so Divisor = 2,000,000 / Baudrate * 16
            // MIDI Baud Rate = 31,250 so Divisor = 4
            i2cWriteByteData(handle,SC16IS750_MCR,0x00);    // Prescaler = 1
            if(i2cWriteByteData(handle,SC16IS750_LCR,0x83) !=0) log_msg("UART Write Failure\n");	//Line Control with Divisor Latch enabled
            i2cWriteByteData(handle,SC16IS750_DLH,0x00);
            i2cWriteByteData(handle,SC16IS750_DLL,0x04);
            i2cWriteByteData(handle,SC16IS750_LCR,0x03); 	// Clear Divisor Latch. 8,1,none
            i2cWriteByteData(handle,SC16IS750_FCR,0x01);    // Enable TX & Rx FIFO
            // IO Control - gpio[7:4] set to behave as IO pins. All inputs non-latching
            i2cWriteByteData(handle,SC16IS750_IOCONTROL,0x00);
            // Set IO Direction (all Output)
            i2cWriteByteData(handle,SC16IS750_IODIR,0xFF);
            // finally, set up the Track object for this MIDI channel
            Europi.tracks[track].channels[CV_OUT].enabled = TRUE;
            Europi.tracks[track].channels[CV_OUT].type = CHNL_TYPE_MIDI;
            Europi.tracks[track].channels[CV_OUT].quantise = 0;			/* Quantization off by default */
            Europi.tracks[track].channels[CV_OUT].i2c_handle = handle;			
            Europi.tracks[track].channels[CV_OUT].i2c_device = DEV_SC16IS750;
            Europi.tracks[track].channels[CV_OUT].i2c_address = address;
            Europi.tracks[track].channels[CV_OUT].i2c_channel = 0;		
            Europi.tracks[track].channels[CV_OUT].scale_zero = 280;		/* Value required to generate zero volt output */
            Europi.tracks[track].channels[CV_OUT].scale_max = 63000;		/* Value required to generate maximum output voltage */
            Europi.tracks[track].channels[CV_OUT].transpose = 0;			/* fixed (transpose) voltage offset applied to this channel */
            Europi.tracks[track].channels[CV_OUT].octaves = 10;			/* How many octaves are covered from scale_zero to scale_max */
            Europi.tracks[track].channels[CV_OUT].vc_type = VOCT;
            
            /* note the equivalent in the Hardware structure */
            Europi_hw.hw_tracks[track].hw_channels[CV_OUT].enabled = TRUE;
            Europi_hw.hw_tracks[track].hw_channels[CV_OUT].type = CHNL_TYPE_MIDI;
            Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_handle = handle;			
            Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_device = DEV_SC16IS750;
            Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_address = address;
            Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_channel = 0;		
            Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_zero = 280;		
            Europi_hw.hw_tracks[track].hw_channels[CV_OUT].scale_max = 63000;		
            
            // No Gate channel on a MIDI Minion:
            Europi.tracks[track].channels[GATE_OUT].enabled = FALSE;
            Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].enabled = FALSE;
            // Launch a listening Thread
            struct midiChnl sMidiChnl; 
            sMidiChnl.i2c_handle = handle;
            struct midiChnl *pMidiChnl = malloc(sizeof(struct midiChnl));
            memcpy(pMidiChnl, &sMidiChnl, sizeof(struct midiChnl));
            int error = pthread_create(&midiThreadId[i], NULL, MidiThread, pMidiChnl);
            if (error != 0) log_msg("Error creating MIDI Listener thread\n");
            else {
                    log_msg("Midi Listener thread created\n"); 
                    midiThreadLaunched[i] = TRUE;
            }
            track++;
        }
    }
    /* The last_track global can be used instead of MAX_TRACKS to reduce the size of loops*/
    last_track = track;
    log_msg("Last Track: %d\n",last_track);
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
				if (Europi.tracks[track].channels[chnl].type == CHNL_TYPE_MIDI){
                    // This just flashes the MIDI Out LED on a MIDI Minion
                    i2cWriteByteData(Europi.tracks[track].channels[chnl].i2c_handle,SC16IS750_IOSTATE,0x00);
                    usleep(50000);
                    i2cWriteByteData(Europi.tracks[track].channels[chnl].i2c_handle,SC16IS750_IOSTATE,0xFF);
				}
			}
		}
	}
    /* Scan for Hardware Changes */
    hardware_config();
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
	while(raw >= 6000){
		raw = raw - 6000;
		octave++;
	}
	for(i=0;i<=12;i++){
		if(raw >= lower_boundary[scale][i] && raw < upper_boundary[scale][i]) return (scale_values[scale][i] + octave*6000);
	}
    return raw; //Default if it fails to quantise
};

/*
 * PITCH2MIDI
 * 
 * Takes a Note Voltage between 0 and 60000 and returns a MIDI Note number
 * between 0 and 120 - NOTE that MIDI Note numbers should go as high as 127
 * but, using 500/Semi-tone means 127 would be represented by 63,500 which
 * is off the top of our scale
 */
int pitch2midi(uint16_t voltage){
    if(voltage >= 60000) return(120);
    return(voltage/500);
}

